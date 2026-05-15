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
    공통 데이터 구조체
    → CAN, BT 둘 다 이 구조체 사용
****************************************************************/

// 리모콘 → 로봇팔 송신 데이터
typedef struct {
    uint16_t bendingSensor[2];  // 밴딩센서 2개
    int16_t  gyro_pitch;        // 자이로 앞뒤
    int16_t  gyro_roll;         // 자이로 좌우
    uint8_t  checksum;          // 오류 검출
} RemoteData;

// 로봇팔 서보 데이터
typedef struct {
    uint16_t motorAngle[6];     // 서보 6개 각도
} MotorData;

// 시스템 상태
typedef struct {
    uint8_t canStatus;
    uint8_t btStatus;
    uint8_t sysVolt;

    #if (ProjModeState)
    uint8_t lcdStatus;
    uint8_t motorStatus[6];
    #else
    uint8_t bendingStatus[2];
    uint8_t gyroStatus;
    uint8_t switchStatus;
    #endif
} SystemStatus;

/****************************************************************
    전역 변수 extern 선언
****************************************************************/

extern CommMode currentCommMode;
extern SysState currentSysState;
extern RemoteData  remoteData;      // 공용 데이터
extern MotorData   motorData;       // 공용 데이터
extern SystemStatus sysStatus;


extern osMessageQueueId_t CAN_QueueHandle;
extern osMessageQueueId_t BT_QueueHandle;
extern FDCAN_HandleTypeDef hfdcan1;
extern UART_HandleTypeDef huart1;



#endif /* INC_CONFIG_H_ */
