/*
 * comm_can.h  —  CAN 하드웨어 계층
 *
 * 역할: CAN ID 정의, union 구조체, 데이터 변수 extern, 하드웨어 함수 선언
 * 규칙: 보드별 패킹/언팩 로직 없음 (comm_can_controller / comm_can_robot 담당)
 */

#ifndef INC_COMM_CAN_H_
#define INC_COMM_CAN_H_

#include "main.h"
#include "config.h"

/****************************************************************
    CAN ID
****************************************************************/
#define CAN_ID_REMOTE_SENSOR   0x100   /* 리모콘 → 로봇팔  10ms  센서 데이터  */
#define CAN_ID_REMOTE_STATUS   0x101   /* 리모콘 → 로봇팔 100ms  상태 메시지  */
#define CAN_ID_ROBOT_STATUS    0x202   /* 로봇팔 → 리모콘 100ms  상태 메시지  */

/****************************************************************
    CAN 프레임 union 구조체
    BYTE_FIELD: 송수신 버퍼 직접 접근
    BIT_FIELD:  필드별 읽기/쓰기
****************************************************************/
typedef struct {
    union {
        uint8_t BYTE_FIELD[8];
        struct __attribute__((packed)) {
            uint16_t BendingSensor_0 : 16;
            uint16_t BendingSensor_1 : 16;
            int16_t  GyroPitch       : 16;
            int16_t  GyroRoll        : 16;
        } BIT_FIELD;
    };
} _REMOTE_SENSOR;

typedef struct {
    union {
        uint8_t BYTE_FIELD[8];
        struct __attribute__((packed)) {
            uint8_t RemoteCommStatus   : 8;
            uint8_t _reserved0         : 8;
            uint8_t RemoteSensorStatus : 8;   /* bit0~2=센서이상, bit3=switch, bit4=relay */
            uint8_t _reserved1         : 8;   /* 이전 RemoteChecksum 자리 — 미사용 */
        } BIT_FIELD;
    };
} _REMOTE_STATUS;

typedef struct {
    union {
        uint8_t BYTE_FIELD[8];
        struct __attribute__((packed)) {
            uint8_t MotorStatus     : 8;   /* bit0~5 = 모터 0~5 PWM 출력 여부 */
            uint8_t RobotCommStatus : 8;
            uint8_t _reserved0      : 8;
            uint8_t LcdStatus       : 8;
        } BIT_FIELD;
    };
} _ROBOT_STATUS;

typedef struct {
    _REMOTE_SENSOR REMOTE_SENSOR;
    _REMOTE_STATUS REMOTE_STATUS;
} REMOTE_CAN_MESSAGE;

typedef struct {
    _ROBOT_STATUS  ROBOT_STATUS;
} ROBOT_CAN_MESSAGE;

/****************************************************************
    데이터 변수 extern
    정의 위치: comm_can.c
****************************************************************/
extern REMOTE_CAN_MESSAGE RemoteCanMsg;   /* 리모콘 관련 raw 프레임 */
extern ROBOT_CAN_MESSAGE  RobotCanMsg;    /* 로봇팔 관련 raw 프레임 */

extern RemoteSensorTx remoteSensorTx;    /* readSensor → Pack → TX */
extern RemoteSensorRx remoteSensorRx;    /* RX → Servo_Task         */
extern SystemStatus   sysStatus;         /* 상대방 보드 상태          */

extern uint32_t DIAG_MsgRxCnt_Remote;
extern uint32_t DIAG_MsgRxCnt_Robot;

/****************************************************************
    하드웨어 계층 함수
****************************************************************/
void              CAN_Start(void);
HAL_StatusTypeDef CAN_Transmit(uint32_t id, uint8_t *data);

#endif /* INC_COMM_CAN_H_ */
