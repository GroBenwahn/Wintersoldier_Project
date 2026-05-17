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
#include "readSensor.h"
#include "config.h"
#include <string.h>

/****************************************************************
	Function Declaration
****************************************************************/
void CAN_Start(void);
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan,
                                uint32_t RxFifo0ITs);
uint8_t CAN_IsConnected(void);
uint8_t CAN_CalcChecksum(uint8_t *data, uint8_t length);
void Fill_Remote_CAN_Message(uint32_t canID, uint8_t rxData[8]);
void Fill_Robot_CAN_Message(uint32_t canID, uint8_t rxData[8]);
void Update_RemoteSensorRx(void);
void Update_RobotMotorRx(void);
void Update_SystemStatus_FromRemote(void);
void Update_SystemStatus_FromRobot(void);
void Pack_Remote_CAN_Message(uint32_t canID);
void Pack_Robot_CAN_Message(uint32_t canID);
HAL_StatusTypeDef Tx_Remote_CAN_Message(uint32_t canID);
HAL_StatusTypeDef Tx_Robot_CAN_Message(uint32_t canID);

/****************************************************************
	Data Define
	— 이 모듈이 모든 CAN TX/RX 데이터의 소유자
****************************************************************/
// CAN 하드웨어 구조체 (union BIT_FIELD / BYTE_FIELD)
REMOTE_CAN_MESSAGE RemoteCanMsg = {0};
ROBOT_CAN_MESSAGE  RobotCanMsg  = {0};

// CAN TX 데이터 (ReadSensor / 서보 제어 로직이 채움)
RemoteSensorTx remoteSensorTx = {0};
RobotMotorTx   robotMotorTx   = {0};

// CAN RX 데이터 (Update 함수가 채움)
RemoteSensorRx remoteSensorRx = {0};
RobotMotorRx   robotMotorRx   = {0};

// 상대방 시스템 상태 (Update_SystemStatus_From* 만 채움)
SystemStatus sysStatus = {0};

// 진단 카운터
uint32_t DIAG_MsgRxCnt_Remote = 0;
uint32_t DIAG_MsgRxCnt_Robot  = 0;

// 연결 상태 내부 변수
static uint32_t lastCanRxTick = 0;
static uint8_t  canConnected  = 0;

// TX 헤더 / 버퍼 (Tx 함수 공용)
FDCAN_TxHeaderTypeDef CAN_TxHeader;
uint8_t CAN_TxData[8];

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

        if(rxHeader.Identifier >= 0x100 &&
           rxHeader.Identifier <= 0x1FF) {
            Fill_Remote_CAN_Message(rxHeader.Identifier, rxData);

        } else if(rxHeader.Identifier >= 0x200 &&
                  rxHeader.Identifier <= 0x2FF) {
            Fill_Robot_CAN_Message(rxHeader.Identifier, rxData);
        }
    }
}

/****************************************************************
    Function: CAN_IsConnected
    Description: CAN 연결 상태 확인
****************************************************************/
uint8_t CAN_IsConnected(void) {
    if(!canConnected) return 0;  // 한 번도 수신 안 했으면 무조건 0 (부팅 직후 false positive 방지)
    if((HAL_GetTick() - lastCanRxTick) < CAN_TIMEOUT_MS) {
        return 1;
    }
    canConnected = 0;
    return 0;
}

/****************************************************************
    Function: CAN_CalcChecksum
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
    Description: 0x100 수신 → Update_RemoteSensorRx
                 0x101 수신 → Update_SystemStatus_FromRemote
****************************************************************/
void Fill_Remote_CAN_Message(uint32_t canID, uint8_t rxData[8]) {
    uint8_t i;

    switch(canID) {
        case CAN_ID_REMOTE_SENSOR:   // 0x100 — 핵심 센서 데이터
            for(i = 0; i < 8; i++) {
                RemoteCanMsg.REMOTE_SENSOR.BYTE_FIELD[i] = rxData[i];
            }
            DIAG_MsgRxCnt_Remote++;
            Update_RemoteSensorRx();
            break;

        case CAN_ID_REMOTE_STATUS:   // 0x101 — 리모콘 시스템 상태
            for(i = 0; i < 8; i++) {
                RemoteCanMsg.REMOTE_STATUS.BYTE_FIELD[i] = rxData[i];
            }
            DIAG_MsgRxCnt_Remote++;
            Update_SystemStatus_FromRemote();
            break;

        default:
            break;
    }
}

/****************************************************************
    Function: Fill_Robot_CAN_Message
    Description: 0x200/0x201 수신 → Update_RobotMotorRx
                 0x202 수신     → Update_SystemStatus_FromRobot
****************************************************************/
void Fill_Robot_CAN_Message(uint32_t canID, uint8_t rxData[8]) {
    uint8_t i;

    switch(canID) {
        case CAN_ID_ROBOT_MOTOR_1:   // 0x200 — 모터 0~3
            for(i = 0; i < 8; i++) {
                RobotCanMsg.ROBOT_MOTOR_1.BYTE_FIELD[i] = rxData[i];
            }
            DIAG_MsgRxCnt_Robot++;
            Update_RobotMotorRx();
            break;

        case CAN_ID_ROBOT_MOTOR_2:   // 0x201 — 모터 4~5
            for(i = 0; i < 8; i++) {
                RobotCanMsg.ROBOT_MOTOR_2.BYTE_FIELD[i] = rxData[i];
            }
            DIAG_MsgRxCnt_Robot++;
            Update_RobotMotorRx();
            break;

        case CAN_ID_ROBOT_STATUS:    // 0x202 — 로봇팔 시스템 상태
            for(i = 0; i < 8; i++) {
                RobotCanMsg.ROBOT_STATUS.BYTE_FIELD[i] = rxData[i];
            }
            DIAG_MsgRxCnt_Robot++;
            Update_SystemStatus_FromRobot();
            break;

        default:
            break;
    }
}

/****************************************************************
    Function: Update_RemoteSensorRx
    Description: CAN 0x100 수신 후 호출
                 RemoteCanMsg → remoteSensorRx (서보 제어 입력)
                 밴딩센서/자이로 — 가장 중요한 데이터
****************************************************************/
void Update_RemoteSensorRx(void) {
    remoteSensorRx.bendingSensor[0] = RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.BendingSensor_0;
    remoteSensorRx.bendingSensor[1] = RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.BendingSensor_1;
    remoteSensorRx.gyro_pitch       = RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.GyroPitch;
    remoteSensorRx.gyro_roll        = RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.GyroRoll;
}

/****************************************************************
    Function: Update_RobotMotorRx
    Description: CAN 0x200/0x201 수신 후 호출
                 RobotCanMsg → robotMotorRx (리모콘 피드백용)
****************************************************************/
void Update_RobotMotorRx(void) {
    robotMotorRx.motorAngle[0] = RobotCanMsg.ROBOT_MOTOR_1.BIT_FIELD.MotorAngle_0;
    robotMotorRx.motorAngle[1] = RobotCanMsg.ROBOT_MOTOR_1.BIT_FIELD.MotorAngle_1;
    robotMotorRx.motorAngle[2] = RobotCanMsg.ROBOT_MOTOR_1.BIT_FIELD.MotorAngle_2;
    robotMotorRx.motorAngle[3] = RobotCanMsg.ROBOT_MOTOR_1.BIT_FIELD.MotorAngle_3;
    robotMotorRx.motorAngle[4] = RobotCanMsg.ROBOT_MOTOR_2.BIT_FIELD.MotorAngle_4;
    robotMotorRx.motorAngle[5] = RobotCanMsg.ROBOT_MOTOR_2.BIT_FIELD.MotorAngle_5;
}

/****************************************************************
    Function: Update_SystemStatus_FromRemote
    Description: CAN 0x101 수신 후 호출
                 리모콘 상태 → sysStatus (로봇팔 보드에서 사용)
                 remoteSensorRx.checksum 도 여기서 업데이트
****************************************************************/
void Update_SystemStatus_FromRemote(void) {
    sysStatus.peerCommMode = (CommMode)RemoteCanMsg.REMOTE_STATUS.BIT_FIELD.RemoteCommStatus;

#if (ProjModeState)   // 로봇팔: 리모콘 상태 저장
    uint8_t packed         = RemoteCanMsg.REMOTE_STATUS.BIT_FIELD.RemoteSensorStatus;
    sysStatus.sensorStatus = packed & 0x07;           // bit0~2: 센서 오류
    sysStatus.switchStatus = (packed >> 3) & 0x01;    // bit3: 스위치
    sysStatus.relayStatus  = (packed >> 4) & 0x01;    // bit4: 릴레이
#endif

    remoteSensorRx.checksum = RemoteCanMsg.REMOTE_STATUS.BIT_FIELD.RemoteChecksum;
}

/****************************************************************
    Function: Update_SystemStatus_FromRobot
    Description: CAN 0x202 수신 후 호출
                 로봇팔 상태 → sysStatus (리모콘 보드에서 사용)
****************************************************************/
void Update_SystemStatus_FromRobot(void) {
    sysStatus.peerCommMode = (CommMode)RobotCanMsg.ROBOT_STATUS.BIT_FIELD.RobotCommStatus;

#if (!ProjModeState)  // 리모콘: 로봇팔 상태 저장
    uint8_t mStatus = RobotCanMsg.ROBOT_STATUS.BIT_FIELD.MotorStatus;
    uint8_t i;
    for(i = 0; i < 6; i++) {
        sysStatus.motorStatus[i] = (mStatus >> i) & 0x01;
    }
    sysStatus.lcdStatus = RobotCanMsg.ROBOT_STATUS.BIT_FIELD.LcdStatus;
#endif
}

/****************************************************************
    Function: Pack_Remote_CAN_Message
    Description: remoteSensorTx / localXxx → RemoteCanMsg (Tx 직전 호출)

    RemoteSensorStatus 바이트 구성:
      bit0 = bending0 이상, bit1 = bending1 이상, bit2 = gyro 이상
      bit3 = 스위치 상태,   bit4 = 릴레이 상태
****************************************************************/
void Pack_Remote_CAN_Message(uint32_t canID) {
    switch(canID) {
        case CAN_ID_REMOTE_SENSOR:
            RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.BendingSensor_0 = remoteSensorTx.bendingSensor[0];
            RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.BendingSensor_1 = remoteSensorTx.bendingSensor[1];
            RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.GyroPitch        = remoteSensorTx.gyro_pitch;
            RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.GyroRoll         = remoteSensorTx.gyro_roll;
            break;

        case CAN_ID_REMOTE_STATUS: {
#if (!ProjModeState)
            uint8_t packed = (localSensorStatus & 0x07)        |
                             ((localSwitchStatus & 0x01) << 3) |
                             ((localRelayStatus  & 0x01) << 4);
#else
            uint8_t packed = 0;
#endif
            RemoteCanMsg.REMOTE_STATUS.BIT_FIELD.RemoteCommStatus   = (uint8_t)currentCommMode;
            RemoteCanMsg.REMOTE_STATUS.BIT_FIELD._reserved0         = 0;
            RemoteCanMsg.REMOTE_STATUS.BIT_FIELD.RemoteSensorStatus = packed;
            RemoteCanMsg.REMOTE_STATUS.BIT_FIELD.RemoteChecksum     =
                CAN_CalcChecksum(RemoteCanMsg.REMOTE_SENSOR.BYTE_FIELD, 8);
            break;
        }

        default:
            break;
    }
}

/****************************************************************
    Function: Pack_Robot_CAN_Message
    Description: robotMotorTx / localXxx → RobotCanMsg (Tx 직전 호출)

    MotorStatus 바이트: bit0~bit5 = 모터 0~5 이상 여부
****************************************************************/
void Pack_Robot_CAN_Message(uint32_t canID) {
    switch(canID) {
        case CAN_ID_ROBOT_MOTOR_1:
            RobotCanMsg.ROBOT_MOTOR_1.BIT_FIELD.MotorAngle_0 = robotMotorTx.motorAngle[0];
            RobotCanMsg.ROBOT_MOTOR_1.BIT_FIELD.MotorAngle_1 = robotMotorTx.motorAngle[1];
            RobotCanMsg.ROBOT_MOTOR_1.BIT_FIELD.MotorAngle_2 = robotMotorTx.motorAngle[2];
            RobotCanMsg.ROBOT_MOTOR_1.BIT_FIELD.MotorAngle_3 = robotMotorTx.motorAngle[3];
            break;

        case CAN_ID_ROBOT_MOTOR_2:
            RobotCanMsg.ROBOT_MOTOR_2.BIT_FIELD.MotorAngle_4 = robotMotorTx.motorAngle[4];
            RobotCanMsg.ROBOT_MOTOR_2.BIT_FIELD.MotorAngle_5 = robotMotorTx.motorAngle[5];
            break;

        case CAN_ID_ROBOT_STATUS: {
            uint8_t mStatus = 0;
#if (ProjModeState)
            uint8_t i;
            for(i = 0; i < 6; i++) {
                mStatus |= (localMotorStatus[i] & 0x01) << i;
            }
#endif
            RobotCanMsg.ROBOT_STATUS.BIT_FIELD.MotorStatus     = mStatus;
            RobotCanMsg.ROBOT_STATUS.BIT_FIELD.RobotCommStatus = (uint8_t)currentCommMode;
            RobotCanMsg.ROBOT_STATUS.BIT_FIELD._reserved0      = 0;
#if (ProjModeState)
            RobotCanMsg.ROBOT_STATUS.BIT_FIELD.LcdStatus       = localLcdStatus;
#endif
            break;
        }

        default:
            break;
    }
}

/****************************************************************
    Function: Tx_Remote_CAN_Message
    Description: 리모콘 CAN 메시지 송신 (Pack 후 호출)
****************************************************************/
HAL_StatusTypeDef Tx_Remote_CAN_Message(uint32_t canID) {
    CAN_TxHeader.Identifier         = canID;
    CAN_TxHeader.IdType             = FDCAN_STANDARD_ID;
    CAN_TxHeader.TxFrameType        = FDCAN_DATA_FRAME;
    CAN_TxHeader.DataLength         = FDCAN_DLC_BYTES_8;
    CAN_TxHeader.BitRateSwitch      = FDCAN_BRS_OFF;
    CAN_TxHeader.FDFormat           = FDCAN_CLASSIC_CAN;
    CAN_TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    CAN_TxHeader.MessageMarker      = 0;

    switch(canID) {
        case CAN_ID_REMOTE_SENSOR:
            memcpy(CAN_TxData, RemoteCanMsg.REMOTE_SENSOR.BYTE_FIELD, 8);
            break;
        case CAN_ID_REMOTE_STATUS:
            memcpy(CAN_TxData, RemoteCanMsg.REMOTE_STATUS.BYTE_FIELD, 8);
            break;
        default:
            return HAL_ERROR;
    }
    return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &CAN_TxHeader, CAN_TxData);
}

/****************************************************************
    Function: Tx_Robot_CAN_Message
    Description: 로봇팔 CAN 메시지 송신 (Pack 후 호출)
****************************************************************/
HAL_StatusTypeDef Tx_Robot_CAN_Message(uint32_t canID) {
    CAN_TxHeader.Identifier         = canID;
    CAN_TxHeader.IdType             = FDCAN_STANDARD_ID;
    CAN_TxHeader.TxFrameType        = FDCAN_DATA_FRAME;
    CAN_TxHeader.DataLength         = FDCAN_DLC_BYTES_8;
    CAN_TxHeader.BitRateSwitch      = FDCAN_BRS_OFF;
    CAN_TxHeader.FDFormat           = FDCAN_CLASSIC_CAN;
    CAN_TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    CAN_TxHeader.MessageMarker      = 0;

    switch(canID) {
        case CAN_ID_ROBOT_MOTOR_1:
            memcpy(CAN_TxData, RobotCanMsg.ROBOT_MOTOR_1.BYTE_FIELD, 8);
            break;
        case CAN_ID_ROBOT_MOTOR_2:
            memcpy(CAN_TxData, RobotCanMsg.ROBOT_MOTOR_2.BYTE_FIELD, 8);
            break;
        case CAN_ID_ROBOT_STATUS:
            memcpy(CAN_TxData, RobotCanMsg.ROBOT_STATUS.BYTE_FIELD, 8);
            break;
        default:
            return HAL_ERROR;
    }
    return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &CAN_TxHeader, CAN_TxData);
}
