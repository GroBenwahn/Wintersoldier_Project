/*
 * comm_can_controller.c  —  리모콘 보드 CAN 처리
 *
 * TX: remoteSensorTx → 0x100 (10ms), 상태 → 0x101 (100ms)
 * RX: 0x202 → sysStatus
 */

#include "comm_can_controller.h"
#include "readSensor.h"

#if (!ProjModeState)

/****************************************************************
    Function: Controller_CAN_TX_Sensor
    Description: remoteSensorTx → CAN 0x100 송신 (10ms)
****************************************************************/
void Controller_CAN_TX_Sensor(void)
{
    RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.BendingSensor_0 = remoteSensorTx.bendingSensor[0];
    RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.BendingSensor_1 = remoteSensorTx.bendingSensor[1];
    RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.GyroPitch        = remoteSensorTx.gyro_pitch;
    RemoteCanMsg.REMOTE_SENSOR.BIT_FIELD.GyroRoll         = remoteSensorTx.gyro_roll;

    CAN_Transmit(CAN_ID_REMOTE_SENSOR, RemoteCanMsg.REMOTE_SENSOR.BYTE_FIELD);
}

/****************************************************************
    Function: Controller_CAN_TX_Status
    Description: 리모콘 상태 → CAN 0x101 송신 (100ms)
                 RemoteSensorStatus 구성:
                   bit0~2 = 센서 이상 (bending0/1, gyro)
                   bit3   = localSwitchStatus
                   bit4   = localRelayStatus
****************************************************************/
void Controller_CAN_TX_Status(void)
{
    Controller_CAN_TX_Status_Forced(currentCommMode);
}

/* CAN→BT 전환 시, CAN 중단 전에 명시적 CommMode를 담아 마지막 0x101 패킷 송신 */
void Controller_CAN_TX_Status_Forced(CommMode mode)
{
    uint8_t packed = (localSensorStatus & 0x07)        |
                     ((localSwitchStatus & 0x01) << 3) |
                     ((localRelayStatus  & 0x01) << 4);

    RemoteCanMsg.REMOTE_STATUS.BIT_FIELD.RemoteCommStatus   = (uint8_t)mode;
    RemoteCanMsg.REMOTE_STATUS.BIT_FIELD._reserved0         = 0;
    RemoteCanMsg.REMOTE_STATUS.BIT_FIELD.RemoteSensorStatus = packed;
    RemoteCanMsg.REMOTE_STATUS.BIT_FIELD._reserved1         = 0;

    CAN_Transmit(CAN_ID_REMOTE_STATUS, RemoteCanMsg.REMOTE_STATUS.BYTE_FIELD);
}

/****************************************************************
    Function: Controller_CAN_RX_Handle
    Description: 0x202 수신 → sysStatus 업데이트
****************************************************************/
void Controller_CAN_RX_Handle(uint32_t id, uint8_t *data)
{
    (void)data;   /* RobotCanMsg는 콜백에서 이미 복사됨 */

    if (id != CAN_ID_ROBOT_STATUS) return;

    sysStatus.peerCommMode = (CommMode)RobotCanMsg.ROBOT_STATUS.BIT_FIELD.RobotCommStatus;

    uint8_t mStatus = RobotCanMsg.ROBOT_STATUS.BIT_FIELD.MotorStatus;
    uint8_t i;
    for (i = 0; i < 6; i++) {
        sysStatus.motorStatus[i] = (mStatus >> i) & 0x01;
    }
    sysStatus.lcdStatus = RobotCanMsg.ROBOT_STATUS.BIT_FIELD.LcdStatus;
}

#endif /* !ProjModeState */
