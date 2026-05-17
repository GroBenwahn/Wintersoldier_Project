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
uint8_t CAN_CalcChecksum(uint8_t *data, uint8_t length);
void Fill_Remote_CAN_Message(uint32_t canID, uint8_t rxData[8]);
/****************************************************************
	Data Define
****************************************************************/
REMOTE_CAN_MESSAGE RemoteCanMsg = {0};
ROBOT_CAN_MESSAGE  RobotCanMsg  = {0};

FDCAN_TxHeaderTypeDef CAN_TxHeader;
uint8_t CAN_TxData[8];

uint32_t DIAG_MsgRxCnt_Remote = 0;
uint32_t DIAG_MsgRxCnt_Robot  = 0;

static uint32_t lastCanRxTick = 0;
static uint8_t  canConnected  = 0;
/****************************************************************
    Function: CAN_Start
    Description: CAN 필터 설정 및 시작
****************************************************************/
void CAN_Start(void) {
    FDCAN_FilterTypeDef filterConfig;

    // 리모콘 데이터 범위 (0x100~0x1FF)
    filterConfig.IdType       = FDCAN_STANDARD_ID;
    filterConfig.FilterIndex  = 0;
    filterConfig.FilterType   = FDCAN_FILTER_RANGE;
    filterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filterConfig.FilterID1    = 0x100;
    filterConfig.FilterID2    = 0x1FF;
    HAL_FDCAN_ConfigFilter(&hfdcan1, &filterConfig);

    // 로봇팔 데이터 범위 (0x200~0x2FF)
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
    Description: CAN 수신 인터럽트 콜백
****************************************************************/
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan,
                                uint32_t RxFifo0ITs) {
    if(hfdcan->Instance == FDCAN1) {
        FDCAN_RxHeaderTypeDef rxHeader;
        uint8_t rxData[8] = {0};

        HAL_FDCAN_GetRxMessage(hfdcan,
            FDCAN_RX_FIFO0, &rxHeader, rxData);

        lastCanRxTick = HAL_GetTick();
        canConnected  = 1;

        // ID 범위로 Fill 함수 분기
        if(rxHeader.Identifier >= 0x100 &&
           rxHeader.Identifier <= 0x1FF) {
            Fill_Remote_CAN_Message(
                rxHeader.Identifier, rxData);

        } else if(rxHeader.Identifier >= 0x200 &&
                  rxHeader.Identifier <= 0x2FF) {
            Fill_Robot_CAN_Message(
                rxHeader.Identifier, rxData);
        }
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

/****************************************************************
    Function: CAN_CalcChecksum
    Description:
****************************************************************/
uint8_t CAN_CalcChecksum(uint8_t *data, uint8_t length) {
    uint8_t sum = 0;
    uint8_t i;
    for(i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

/****************************************************************
    Function: Fill_Remote_CAN_Message
    Description: 리모콘 RAW CAN 데이터 → RemoteCanMsg 채우기
****************************************************************/
void Fill_Remote_CAN_Message(uint32_t canID, uint8_t rxData[8]) {
    uint8_t i;

    switch(canID) {
        case CAN_ID_REMOTE_SENSOR:   // 0x100
            for(i = 0; i < 8; i++) {
                RemoteCanMsg.REMOTE_SENSOR.BYTE_FIELD[i] = rxData[i];
            }
            DIAG_MsgRxCnt_Remote++;
            break;

        case CAN_ID_REMOTE_STATUS:   // 0x101
            for(i = 0; i < 8; i++) {
                RemoteCanMsg.REMOTE_STATUS.BYTE_FIELD[i] = rxData[i];
            }
            DIAG_MsgRxCnt_Remote++;
            break;

        default:
            break;
    }

    // RAW 채운 후 공용 구조체 업데이트
    Update_RemoteData();
}

/****************************************************************
    Function: Fill_Robot_CAN_Message
    Description: 로봇팔 RAW CAN 데이터 → RobotCanMsg 채우기
****************************************************************/
void Fill_Robot_CAN_Message(uint32_t canID, uint8_t rxData[8]) {
    uint8_t i;

    switch(canID) {
        case CAN_ID_ROBOT_MOTOR_1:   // 0x200
            for(i = 0; i < 8; i++) {
                RobotCanMsg.ROBOT_MOTOR_1.BYTE_FIELD[i] = rxData[i];
            }
            DIAG_MsgRxCnt_Robot++;
            break;

        case CAN_ID_ROBOT_MOTOR_2:   // 0x201
            for(i = 0; i < 8; i++) {
                RobotCanMsg.ROBOT_MOTOR_2.BYTE_FIELD[i] = rxData[i];
            }
            DIAG_MsgRxCnt_Robot++;
            break;

        case CAN_ID_ROBOT_STATUS:    // 0x202
            for(i = 0; i < 8; i++) {
                RobotCanMsg.ROBOT_STATUS.BYTE_FIELD[i] = rxData[i];
            }
            DIAG_MsgRxCnt_Robot++;
            break;

        default:
            break;
    }

    // RAW 채운 후 공용 구조체 업데이트
    Update_MotorData();
    Update_SystemStatus_CAN();
}

//이후 채워야함 헤더 파일 좀 바꿔서 업뎃 함수랑 이것저것 살짝 수정 필요
