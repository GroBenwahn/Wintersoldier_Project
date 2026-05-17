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
#define CAN_ID_REMOTE_SENSOR    0x100  // 리모콘 센서 데이터_1  10ms 마다 송신
#define CAN_ID_REMOTE_STATUS    0x101  // 리모콘 시스템 상태  100ms 마다 송신
#define CAN_ID_ROBOT_MOTOR_1      0x200  // 로봇팔 모터 데이터  10ms 마다 송신
#define CAN_ID_ROBOT_MOTOR_2      0x201  // 로봇팔 모터 데이터  10ms 마다 송신
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
        	uint16_t GyroPitch : 8;
        	uint16_t GyroRoll : 8;
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
        	uint8_t RemoteCommStatus : 8;
        	uint8_t RemoteSysVolt : 8;
        	uint8_t RemoteSensorStatus : 8; // 밴딩센서, 자이로센서 정보로 나눌 필요가 있을수도?
        	uint8_t RemoteChecksum : 8;
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
        	uint16_t MotorAngle_0 : 16;
        	uint16_t MotorAngle_1 : 16;
        	uint16_t MotorAngle_2 : 16;
        	uint16_t MotorAngle_3 : 16;
        }BIT_FIELD;
    };
} _ROBOT_MOTOR_1;

typedef struct
{
    union
    {
        uint8_t BYTE_FIELD[8];
        struct __attribute__((packed))
        {
        	uint16_t MotorAngle_4 : 16;
        	uint16_t MotorAngle_5 : 16;
        }BIT_FIELD;
    };
} _ROBOT_MOTOR_2;

typedef struct
{
    union
    {
        uint8_t BYTE_FIELD[8];
        struct __attribute__((packed))
        {
        	uint8_t MotorStatus : 8;
        	uint8_t RobotCommStatus : 8;
        	uint8_t RobotSysVolt : 8;
        	uint8_t LcdStatus : 8;
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
	_ROBOT_MOTOR_1	ROBOT_MOTOR_1;
	_ROBOT_MOTOR_2	ROBOT_MOTOR_2;
	_ROBOT_STATUS	ROBOT_STATUS;
}ROBOT_CAN_MESSAGE;

extern REMOTE_CAN_MESSAGE	RemoteCanMsg;
extern ROBOT_CAN_MESSAGE	RobotCanMsg;


#endif /* INC_CAN_COMM_H_ */
