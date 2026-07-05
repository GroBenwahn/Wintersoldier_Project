/*
 * comm_can_robot.c  —  로봇팔 보드 CAN 처리
 *
 * TX: 상태 → 0x202 (100ms)
 * RX: 0x100 → remoteSensorRx, 0x101 → sysStatus
 */

#include "comm_can_robot.h"
#include "readSensor.h"

#if (ProjModeState)

/****************************************************************
    Function: Robot_CAN_TX_Status
    Description: 로봇팔 상태 → CAN 0x202 송신 (100ms)
                 MotorStatus: bit0~5 = 모터 0~5 PWM 출력 여부 (CCR > 0)
****************************************************************/
void Robot_CAN_TX_Status(void)
{
    uint8_t mStatus = 0;
    uint8_t i;
    for (i = 0; i < 6; i++) {
        mStatus |= (localMotorStatus[i] & 0x01) << i;
    }

    RobotCanMsg.ROBOT_STATUS.BIT_FIELD.MotorStatus     = mStatus;
    RobotCanMsg.ROBOT_STATUS.BIT_FIELD.RobotCommStatus = (uint8_t)currentCommMode;
    RobotCanMsg.ROBOT_STATUS.BIT_FIELD._reserved0      = 0;
    RobotCanMsg.ROBOT_STATUS.BIT_FIELD.LcdStatus       = localLcdStatus;

    CAN_Transmit(CAN_ID_ROBOT_STATUS, RobotCanMsg.ROBOT_STATUS.BYTE_FIELD);
}

/****************************************************************
    Function: Robot_CAN_RX_Handle
    Description: 0x100 수신 → remoteSensorRx 업데이트
                 0x101 수신 → sysStatus 업데이트
****************************************************************/
void Robot_CAN_RX_Handle(uint32_t id, uint8_t *data)
{
    (void)data;   /* RemoteCanMsg는 콜백에서 이미 복사됨 */

    switch (id) {
        case CAN_ID_REMOTE_SENSOR:   /* 0x100 */
            remoteSensorRx.bendingSensor[0] = RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.BendingSensor_0;
            remoteSensorRx.bendingSensor[1] = RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.BendingSensor_1;
            remoteSensorRx.gyro_pitch       = RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.GyroPitch;
            remoteSensorRx.gyro_roll        = RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.GyroRoll;
            break;

        case CAN_ID_REMOTE_STATUS:   /* 0x101 */
        {
            uint8_t packed;
            sysStatus.peerCommMode = (CommMode)RemoteCanMsg.REMOTE_STATUS.BIT_FIELD.RemoteCommStatus;
            packed                 = RemoteCanMsg.REMOTE_STATUS.BIT_FIELD.RemoteSensorStatus;
            sysStatus.sensorStatus = packed & 0x07;
            sysStatus.switchStatus = (packed >> 3) & 0x01;
            sysStatus.relayStatus  = (packed >> 4) & 0x01;
            break;
        }

        default:
            break;
    }
}

#endif /* ProjModeState */
