#include "can_comm.h"
#include "switch_mode.h"
#include "cmsis_os.h"
#include <string.h>

#define CAN_RECOVERY_INTERVAL_MS  2000

uint8_t  can_offline = 0;     /* can 연결 상태를 확인 */
static uint32_t can_retry_tick = 0;

/* 최근 수신 패킷 저장소 (ISR에서 갱신되므로 volatile) */
volatile GSensorPacket_t latest_gPkt = {0};
volatile FlexPacket_t    latest_fPkt = {0};

/* CAN에서 마지막으로 수신한 원본 (채널별 구분용, Live Expression 확인용) */
volatile GSensorPacket_t g_canRxGSensorPkt = {0};
volatile FlexPacket_t    g_canRxFlexPkt    = {0};

/* Hand로부터 마지막으로 수신한 상태(0x102) — comm_mode와 무관하게 항상 갱신 */
volatile HandStatusPacket_t g_canRxHandStatusPkt = {0};

/* Robot 자체 상태 (lcd.c/servo.c가 필드를 나눠서 채움, Callback_1000ms가 0x201로 주기 송신) */
RobotStatusPacket_t g_robotStatusPkt = { HW_STATUS_CONNECTED, 0, 0, 0, {0}, 0 };

/* 통신 연결 플래그 (COMM_CONNECTED_NONE/CAN/BT) */
volatile uint8_t comm_connected = COMM_CONNECTED_NONE;


/* 공통 TX 헤더 생성 (Hand와 동일 패턴, 호출자마다 지역변수라 여러 곳에서 동시에 불러도 안전) */
static FDCAN_TxHeaderTypeDef CAN_MakeTxHeader(uint32_t id)
{
    FDCAN_TxHeaderTypeDef header = {
        .Identifier          = id,
        .IdType              = FDCAN_STANDARD_ID,
        .TxFrameType         = FDCAN_DATA_FRAME,
        .DataLength          = FDCAN_DLC_BYTES_8,
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch       = FDCAN_BRS_OFF,
        .FDFormat            = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
        .MessageMarker       = 0,
    };
    return header;
}


/* CAN 초기화: 필터 없이 모든 Standard ID를 FIFO0으로 받고, 소프트웨어 switch-case로 ID 구분 */
void CAN_Comm_Init(void)
{
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0,
                                  FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_BUS_OFF, 0);
    HAL_FDCAN_Start(&hfdcan1);
}


/* 프레임 1개 송신 후 에러 카운터 확인 (Hand와 동일 패턴) */
static uint8_t CAN_SendFrame(uint32_t id, const uint8_t *data)
{
    FDCAN_TxHeaderTypeDef tx_header = CAN_MakeTxHeader(id);
    FDCAN_ProtocolStatusTypeDef status;

    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, (uint8_t *)data);
    HAL_FDCAN_GetProtocolStatus(&hfdcan1, &status);

    if (status.ErrorPassive == 1)
    {
        HAL_FDCAN_Stop(&hfdcan1);   /* TEC 즉시 리셋 */
        can_offline = 1;
        return 0;
    }
    return 1;
}


/* ID별 CAN 송신 (앞으로 ID 추가하기 쉽게 switch-case 유지) */
void CAN_Send(uint32_t id, const uint8_t *data)
{
    if (can_offline) return;

    switch (id)
    {
        case CAN_MSG_SERVO_DBG:
        case CAN_MSG_ERROR_ROBOT:
            CAN_SendFrame(id, data);
            break;

        default:
            /* 정의되지 않은 ID, 무시 */
            break;
    }
}


/* Robot 자체 상태(g_robotStatusPkt)를 CAN(0x201)으로 송신 (Callback_1000ms가 호출) */
void RobotStatus_Send(void)
{
    g_robotStatusPkt.checksum = CALC_CHECKSUM(&g_robotStatusPkt);
    CAN_Send(CAN_MSG_ERROR_ROBOT, (const uint8_t *)&g_robotStatusPkt);
}


/* 2초마다 CAN 복구 시도 (Hand와 동일 패턴, 프로브로 현재 g_robotStatusPkt 재사용) */
void CAN_Recovery(void)
{
    if (!can_offline) return;

    if ((HAL_GetTick() - can_retry_tick) < CAN_RECOVERY_INTERVAL_MS) return;
    can_retry_tick = HAL_GetTick();

    HAL_FDCAN_Start(&hfdcan1);

    FDCAN_TxHeaderTypeDef tx_header = CAN_MakeTxHeader(CAN_MSG_ERROR_ROBOT);
    FDCAN_ErrorCountersTypeDef counters;
    g_robotStatusPkt.checksum = CALC_CHECKSUM(&g_robotStatusPkt);
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, (uint8_t *)&g_robotStatusPkt);
    osDelay(5);
    HAL_FDCAN_GetErrorCounters(&hfdcan1, &counters);

    if (counters.TxErrorCnt == 0)
    {
        can_offline = 0;    /* 복구 성공 */
    }
    else
    {
        HAL_FDCAN_Stop(&hfdcan1);   /* 아직 끊김, 다시 대기 */
    }
}


/* Bus-Off 인터럽트 콜백 (Hand와 동일 패턴) */
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
    if (ErrorStatusITs & FDCAN_IT_BUS_OFF)
    {
        HAL_FDCAN_Stop(hfdcan);
        can_offline = 1;
    }
}


/* CAN Rx FIFO0 수신 콜백 (인터럽트) */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if (!(RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)) return;

    FDCAN_RxHeaderTypeDef rxHeader;
    uint8_t data[8];

    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, data) != HAL_OK) return;

    /* 체크섬 검증 (byte 0~6 XOR = byte 7) */
    uint8_t cs = 0;
    for (int i = 0; i < 7; i++) cs ^= data[i];

    if (cs != data[7]) return;

    /* 상태 알림(0x102)은 BT/CAN 모드 스위치와 무관하게 항상 처리 — 서보 제어 데이터가 아니므로 */
    if (rxHeader.Identifier == CAN_MSG_ERROR_HAND)
    {
        memcpy((void *)&g_canRxHandStatusPkt, data, 8);
        return;
    }

    if (comm_mode != COMM_CAN) return;

    if (rxHeader.Identifier == CAN_MSG_SENSOR)
    {
        memcpy((void *)&g_canRxGSensorPkt, data, 8);
        memcpy((void *)&latest_gPkt, (const void *)&g_canRxGSensorPkt, 8);
        comm_connected = COMM_CONNECTED_CAN;
    }
    else if (rxHeader.Identifier == CAN_MSG_FLEX)
    {
        memcpy((void *)&g_canRxFlexPkt, data, 8);
        memcpy((void *)&latest_fPkt, (const void *)&g_canRxFlexPkt, 8);
        comm_connected = COMM_CONNECTED_CAN;
    }
}


