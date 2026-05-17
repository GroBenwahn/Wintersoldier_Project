/*
 * config.h
 *
 *  Created on: 2026. 5. 14.
 *      Author: MinJae
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

#include "main.h"
#include "cmsis_os.h"

/****************************************************************
    프로젝트 모드 설정
****************************************************************/
#define PROJ_MODE_ROBOT_ARM  1
#define PROJ_MODE_REMOTE     0
#define ProjModeState        PROJ_MODE_ROBOT_ARM

/****************************************************************
    열거형 정의
****************************************************************/
typedef enum {
    COMM_MODE_IDLE = 0,
    COMM_MODE_CAN,
    COMM_MODE_BT,
    COMM_MODE_ERROR
} CommMode;

typedef enum {
    SYS_OK      = 0,
    SYS_WARNING,
    SYS_ERROR
} SysState;

/****************************************************************
    CAN TX 구조체 — ReadSensor가 채운 뒤 Pack → Tx

    리모콘 보드:  Read_BendingSensor / Read_GyroSensor → remoteSensorTx
    로봇팔 보드:  서보 제어 로직                       → robotMotorTx
****************************************************************/
typedef struct {
    uint16_t bendingSensor[2];   // ADC 밴딩센서 원시값
    int16_t  gyro_pitch;         // MPU6050 Pitch
    int16_t  gyro_roll;          // MPU6050 Roll
} RemoteSensorTx;                // 리모콘 → CAN 0x100

typedef struct {
    uint16_t motorAngle[6];      // 목표 서보 각도 (0~180°)
} RobotMotorTx;                  // 로봇팔 → CAN 0x200/0x201

/****************************************************************
    CAN RX 구조체 — Update 함수가 채운 뒤 Task에서 사용

    로봇팔 보드: Update_RemoteSensorRx → remoteSensorRx (서보 제어 입력)
    리모콘 보드: Update_RobotMotorRx   → robotMotorRx   (피드백 표시용)
****************************************************************/
typedef struct {
    uint16_t bendingSensor[2];   // CAN 0x100에서 파싱
    int16_t  gyro_pitch;
    int16_t  gyro_roll;
    uint8_t  checksum;           // CAN 0x101에서 파싱
} RemoteSensorRx;                // 로봇팔이 수신

typedef struct {
    uint16_t motorAngle[6];      // CAN 0x200/0x201에서 파싱
} RobotMotorRx;                  // 리모콘이 수신

/****************************************************************
    CAN 상태 수신 구조체
    상대방 보드 상태 메시지(0x101 / 0x202) 수신 시에만 채움
    절대 이 보드 자신의 상태를 쓰지 않음

    로봇팔 보드: Update_SystemStatus_FromRemote (0x101 수신)
    리모콘 보드: Update_SystemStatus_FromRobot  (0x202 수신)
****************************************************************/
typedef struct {
    CommMode peerCommMode;        // 상대방 통신 모드

    #if (ProjModeState)           // 로봇팔: 리모콘 상태 수신
    uint8_t  sensorStatus;        // bit0=bending0, bit1=bending1, bit2=gyro 이상
    uint8_t  switchStatus;        // bit0=스위치 상태
    uint8_t  relayStatus;         // bit0=릴레이 상태
    #else                         // 리모콘: 로봇팔 상태 수신
    uint8_t  lcdStatus;
    uint8_t  motorStatus[6];      // 모터 0~5 이상 여부 (0=정상, 1=이상)
    #endif
} SystemStatus;

/****************************************************************
    전역 변수 extern 선언
****************************************************************/
// CAN TX 데이터
extern RemoteSensorTx remoteSensorTx;
extern RobotMotorTx   robotMotorTx;

// CAN RX 데이터
extern RemoteSensorRx remoteSensorRx;
extern RobotMotorRx   robotMotorRx;

// 상대방 시스템 상태 (CAN RX 전용)
extern SystemStatus   sysStatus;

// 시스템
extern CommMode currentCommMode;
extern SysState currentSysState;

// 주변장치 핸들
extern osMessageQueueId_t CAN_QueueHandle;
extern osMessageQueueId_t BT_QueueHandle;
extern FDCAN_HandleTypeDef hfdcan1;
extern UART_HandleTypeDef  huart1;


#endif /* INC_CONFIG_H_ */
