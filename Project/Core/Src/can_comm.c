/*
 * can_comm.c
 *
 *  Created on: 2026. 5. 14.
 *      Author: MinJae
 */

/****************************************************************
	Include Files
****************************************************************/
#include "can_comm.h"
#include "config.h"

/****************************************************************
	Function Declaration
****************************************************************/

/****************************************************************
	Data Define
****************************************************************/
CAN_RxData_t canRxData = {0};
CAN_TxRemotePayload_t canTxRemotePayload = {0};
CAN_TxRobotPayload_t canTxRobotPayload = {0};
uint8_t canConnected = 0;
static uint32_t lastCanRxTick = 0;
/****************************************************************
    Function: CAN_Start
    Description: CAN 시작 (송수신 공통)
****************************************************************/
void CAN_Start(void) {
    FDCAN_FilterTypeDef filterConfig;

    filterConfig.IdType       = FDCAN_STANDARD_ID;
    filterConfig.FilterIndex  = 0;
    filterConfig.FilterType   = FDCAN_FILTER_MASK;
    filterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filterConfig.FilterID1    = 0x000;
    filterConfig.FilterID2    = 0x000;

    HAL_FDCAN_ConfigFilter(&hfdcan1, &filterConfig);
    HAL_FDCAN_ActivateNotification(&hfdcan1,
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_Start(&hfdcan1);
}

/****************************************************************
    Callback: HAL_FDCAN_RxFifo0Callback
    Description: CAN 메시지 수신 인터럽트 콜백
****************************************************************/
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan,
                                uint32_t RxFifo0ITs) {
    if(hfdcan->Instance == FDCAN1) {
        FDCAN_RxHeaderTypeDef rxHeader;

        // 메시지 읽기
        HAL_FDCAN_GetRxMessage(hfdcan,
            FDCAN_RX_FIFO0, &rxHeader, canRxData.data);

        canRxData.id  = rxHeader.Identifier;
        canRxData.dlc = rxHeader.DataLength;

        // 마지막 수신 시간 업데이트
        lastCanRxTick = HAL_GetTick();
        canConnected  = 1;

        // Queue에 데이터 전송
        osMessageQueuePut(CAN_QueueHandle,
            &canRxData, 0, 0);
    }
}

/****************************************************************
    Function: CAN_IsConnected
    Description: CAN 연결 상태 확인
****************************************************************/
uint8_t CAN_IsConnected(void) {
    if((HAL_GetTick() - lastCanRxTick) < CAN_TIMEOUT_MS) {
        return 1;
    }
    canConnected = 0;
    return 0;
}

#if (!ProjModeState)

/****************************************************************
    Function: CAN_CalcChecksum
    Description: 체크섬 계산
****************************************************************/
uint8_t CAN_CalcChecksum(CAN_TxRemotePayload_t *data) {
    uint8_t sum = 0;
    uint8_t *ptr = (uint8_t*)data;
    // checksum 필드 제외하고 계산
    for(int i = 0; i < sizeof(CAN_TxRemotePayload_t) - 1; i++) {
        sum += ptr[i];
    }
    return sum;
}
