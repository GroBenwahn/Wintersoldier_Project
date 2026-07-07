#include "can_comm.h"
#include "cmsis_os.h"

#define CAN_RECOVERY_INTERVAL_MS  2000

uint8_t  can_offline = 0;     /* can 연결 상태를 확인 */
static uint32_t can_retry_tick = 0;

static FDCAN_TxHeaderTypeDef tx_header = {
    .IdType      = FDCAN_STANDARD_ID,
    .TxFrameType = FDCAN_DATA_FRAME,
    .DataLength  = FDCAN_DLC_BYTES_8,
    .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
    .BitRateSwitch       = FDCAN_BRS_OFF,
    .FDFormat            = FDCAN_CLASSIC_CAN,
    .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
    .MessageMarker       = 0,
};


/* CAN 초기화 */
void CAN_Comm_Init(void)
{
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_BUS_OFF, 0);
    HAL_FDCAN_Start(&hfdcan1);
}


/* 프레임 1개 송신 후 에러 카운터 확인 */
static uint8_t CAN_SendFrame(uint32_t id, const uint8_t *data)
{
    FDCAN_ProtocolStatusTypeDef status;

    tx_header.Identifier = id;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, (uint8_t *)data);
    HAL_FDCAN_GetProtocolStatus(&hfdcan1, &status);

    if (status.TxErrorCnt > 0)
    {
        HAL_FDCAN_Stop(&hfdcan1);   /* TEC 즉시 리셋 */
        can_offline = 1;
        return 0;
    }
    return 1;
}


/* GSensor + Flex 패킷 CAN 송신 */
void CAN_Send(const GSensorPacket_t *gPkt, const FlexPacket_t *fPkt)
{
    if (can_offline) return;

    if (!CAN_SendFrame(CAN_MSG_ID_SENSOR, (const uint8_t *)gPkt)) return;
    CAN_SendFrame(CAN_MSG_ID_FLEX, (const uint8_t *)fPkt);
}


/* 2초마다 CAN 복구 시도 */
void CAN_Recovery(void)
{
    if (!can_offline) return;

    if ((HAL_GetTick() - can_retry_tick) < CAN_RECOVERY_INTERVAL_MS) return;
    can_retry_tick = HAL_GetTick();

    HAL_FDCAN_Start(&hfdcan1);

    FDCAN_ProtocolStatusTypeDef status;
    uint8_t probe[8] = {0};
    tx_header.Identifier = CAN_MSG_ID_SENSOR;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, probe);
    osDelay(5);
    HAL_FDCAN_GetProtocolStatus(&hfdcan1, &status);

    if (status.TxErrorCnt == 0)
    {
        can_offline = 0;    /* 복구 성공 */
    }
    else
    {
        HAL_FDCAN_Stop(&hfdcan1);   /* 아직 끊김, 다시 대기 */
    }
}


/* Bus-Off 인터럽트 콜백 */
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
    if (ErrorStatusITs & FDCAN_IT_BUS_OFF)
    {
        HAL_FDCAN_Stop(hfdcan);
        can_offline = 1;
    }
}
