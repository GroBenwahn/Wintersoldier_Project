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

void CAN_Start(void);
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan,
                                uint32_t RxFifo0ITs);
uint8_t CAN_IsConnected(void);

/****************************************************************
	Data Define
****************************************************************/
FDCAN_TxHeaderTypeDef CAN_TxHeader;
uint8_t CAN_TxData[8];

/****************************************************************
    Function: CAN_Start
    Description: CAN 필터 설정 및 시작
****************************************************************/
void CAN_Start(void) {
    FDCAN_FilterTypeDef filterConfig;

    // 필터 0: 리모콘 데이터 범위 (0x100~0x1FF)
    filterConfig.IdType       = FDCAN_STANDARD_ID;
    filterConfig.FilterIndex  = 0;
    filterConfig.FilterType   = FDCAN_FILTER_RANGE;
    filterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filterConfig.FilterID1    = 0x100;
    filterConfig.FilterID2    = 0x1FF;
    HAL_FDCAN_ConfigFilter(&hfdcan1, &filterConfig);

    // 필터 1: 로봇팔 데이터 범위 (0x200~0x2FF)
    filterConfig.FilterIndex  = 1;
    filterConfig.FilterID1    = 0x200;
    filterConfig.FilterID2    = 0x2FF;
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
        HAL_FDCAN_GetRxMessage(hfdcan,
            FDCAN_RX_FIFO0, &rxHeader, canRxRaw.data);

        canRxRaw.id  = rxHeader.Identifier;
        canRxRaw.dlc = rxHeader.DataLength;
        lastCanRxTick = HAL_GetTick();
        canConnected  = 1;

        osMessageQueuePut(CAN_QueueHandle, &canRxRaw, 0, 0);
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
