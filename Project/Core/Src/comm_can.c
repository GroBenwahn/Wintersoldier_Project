/*
 * comm_can.c  —  CAN 하드웨어 계층
 *
 * 역할: 필터 설정/시작, 단일 TX 함수, RX 인터럽트 콜백(→ 보드별 핸들러 dispatch)
 *       데이터 변수 정의 (remoteSensorTx, remoteSensorRx, sysStatus, raw 프레임)
 */

#include "comm_can.h"
#include "comm_can_controller.h"
#include "comm_can_robot.h"
#include <string.h>

/****************************************************************
    데이터 변수 정의
****************************************************************/
REMOTE_CAN_MESSAGE RemoteCanMsg = {0};
ROBOT_CAN_MESSAGE  RobotCanMsg  = {0};

RemoteSensorTx remoteSensorTx = {0};   /* readSensor.c가 채움 → Controller TX */
RemoteSensorRx remoteSensorRx = {0};   /* Robot RX가 채움   → Servo_Task 사용 */
SystemStatus   sysStatus      = {0};   /* 상대방 보드 상태 (RX 핸들러가 채움)  */

uint32_t DIAG_MsgRxCnt_Remote = 0;
uint32_t DIAG_MsgRxCnt_Robot  = 0;

/****************************************************************
    내부 변수
****************************************************************/
static FDCAN_TxHeaderTypeDef s_txHeader;

/****************************************************************
    Function: CAN_Start
    Description: 수신 필터(0x100~0x2FF) 설정 및 FDCAN 시작
                 자기 자신이 송신한 메시지는 수신되지 않으므로
                 양쪽 보드 동일한 필터 1개로 충분
****************************************************************/
void CAN_Start(void)
{
    FDCAN_FilterTypeDef filter;

    filter.IdType       = FDCAN_STANDARD_ID;
    filter.FilterType   = FDCAN_FILTER_RANGE;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterIndex  = 0;
    filter.FilterID1    = 0x100;
    filter.FilterID2    = 0x2FF;
    HAL_FDCAN_ConfigFilter(&hfdcan1, &filter);

    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_Start(&hfdcan1);
}

/****************************************************************
    Function: CAN_Transmit
    Description: 8바이트 데이터를 지정 ID로 송신
****************************************************************/
HAL_StatusTypeDef CAN_Transmit(uint32_t id, uint8_t *data)
{
    uint8_t txBuf[8];

    s_txHeader.Identifier         = id;
    s_txHeader.IdType             = FDCAN_STANDARD_ID;
    s_txHeader.TxFrameType        = FDCAN_DATA_FRAME;
    s_txHeader.DataLength         = FDCAN_DLC_BYTES_8;
    s_txHeader.BitRateSwitch      = FDCAN_BRS_OFF;
    s_txHeader.FDFormat           = FDCAN_CLASSIC_CAN;
    s_txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    s_txHeader.MessageMarker      = 0;

    memcpy(txBuf, data, 8);
    return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &s_txHeader, txBuf);
}

/****************************************************************
    Callback: HAL_FDCAN_RxFifo0Callback
    Description: 수신 → raw 프레임 복사 → 보드별 핸들러 dispatch
                 0x100~0x1FF: 리모콘 메시지 → 로봇팔이 처리
                 0x200~0x2FF: 로봇팔 메시지 → 리모콘이 처리
****************************************************************/
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan->Instance != FDCAN1) return;

    FDCAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8] = {0};
    HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, rxData);

    uint32_t id = rxHeader.Identifier;

    if (id >= 0x100 && id <= 0x1FF) {
        memcpy(RemoteCanMsg.REMOTE_SENSOR.BYTE_FIELD, rxData, 8);
        DIAG_MsgRxCnt_Remote++;
#if (ProjModeState)
        Robot_CAN_RX_Handle(id, rxData);        /* 로봇팔만 처리 */
#endif

    } else if (id >= 0x200 && id <= 0x2FF) {
        memcpy(RobotCanMsg.ROBOT_STATUS.BYTE_FIELD, rxData, 8);
        DIAG_MsgRxCnt_Robot++;
#if (!ProjModeState)
        Controller_CAN_RX_Handle(id, rxData);   /* 리모콘만 처리 */
#endif
    }
}
