/*
 * can_comm.h
 *
 *  Created on: 2026. 5. 14.
 *      Author: MinJae
 */

#ifndef INC_CAN_COMM_H_
#define INC_CAN_COMM_H_

#include "main.h"
#include "config.h"

/****************************************************************
    CAN ID 정의
****************************************************************/
#define CAN_ID_REMOTE_SENSOR    0x100  // 리모콘 센서 데이터   10ms 마다 송신
#define CAN_ID_REMOTE_STATUS    0x101  // 리모콘 시스템 상태  100ms 마다 송신
/* CAN_ID_ROBOT_MOTOR_1(0x200), CAN_ID_ROBOT_MOTOR_2(0x201) 제거:
   서보는 PWM 단방향 제어이므로 로봇팔→리모콘 모터 각도 전송 불필요 */
#define CAN_ID_ROBOT_STATUS     0x202  // 로봇팔 시스템 상태  100ms 마다 송신

#define CAN_TIMEOUT_MS          300

typedef struct
{
    union
    {
        uint8_t BYTE_FIELD[8];
        struct __attribute__((packed))
        {
        	uint16_t BendingSensor_0 : 16;
        	uint16_t BendingSensor_1 : 16;
        	int16_t  GyroPitch       : 16;
        	int16_t  GyroRoll        : 16;
        }BIT_FIELD;
    };
} _REMOTE_SENSOR;

typedef struct
{
    union
    {
        uint8_t BYTE_FIELD[8];
        struct __attribute__((packed))
        {
        	uint8_t RemoteCommStatus   : 8;
        	uint8_t _reserved0         : 8;   // 전압 측정 핀 없음
        	uint8_t RemoteSensorStatus : 8;   // bit0=bending0, bit1=bending1, bit2=gyro, bit3=switch, bit4=relay
        	uint8_t RemoteChecksum     : 8;
        }BIT_FIELD;
    };
} _REMOTE_STATUS;

typedef struct
{
    union
    {
        uint8_t BYTE_FIELD[8];
        struct __attribute__((packed))
        {
        	uint8_t MotorStatus     : 8;   // bit0~5 = 모터 0~5 이상 여부
        	uint8_t RobotCommStatus : 8;
        	uint8_t _reserved0      : 8;   // 전압 측정 핀 없음
        	uint8_t LcdStatus       : 8;
        }BIT_FIELD;
    };
} _ROBOT_STATUS;

typedef struct
{
	_REMOTE_SENSOR	REMOTE_SENSOR;
	_REMOTE_STATUS	REMOTE_STATUS;
}REMOTE_CAN_MESSAGE;

typedef struct
{
	_ROBOT_STATUS	ROBOT_STATUS;
}ROBOT_CAN_MESSAGE;

extern REMOTE_CAN_MESSAGE	RemoteCanMsg;
extern ROBOT_CAN_MESSAGE	RobotCanMsg;

/****************************************************************
    함수 선언
****************************************************************/
// 초기화 / 유틸
void             CAN_Start(void);
uint8_t          CAN_IsConnected(void);
uint8_t          CAN_CalcChecksum(uint8_t *data, uint8_t length);

// 수신 처리 (인터럽트 콜백 → Fill → Update)
void             Fill_Remote_CAN_Message(uint32_t canID, uint8_t rxData[8]);
void             Fill_Robot_CAN_Message(uint32_t canID, uint8_t rxData[8]);
void             Update_RemoteSensorRx(void);           // 0x100 수신 → remoteSensorRx
void             Update_SystemStatus_FromRemote(void);  // 0x101 수신 → sysStatus
void             Update_SystemStatus_FromRobot(void);   // 0x202 수신 → sysStatus

// 송신 처리 (Pack → Tx)
void             Pack_Remote_CAN_Message(uint32_t canID);  // remoteSensorTx → RemoteCanMsg
void             Pack_Robot_CAN_Message(uint32_t canID);   // robotMotorTx   → RobotCanMsg
HAL_StatusTypeDef Tx_Remote_CAN_Message(uint32_t canID);
HAL_StatusTypeDef Tx_Robot_CAN_Message(uint32_t canID);


#endif /* INC_CAN_COMM_H_ */
