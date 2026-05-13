/*
 * config.h
 *
 *  Created on: 2026. 5. 14.
 *      Author: MinJae
 */

// 로봇팔 혹은 리모콘 모드 설정
#define PROJ_MODE_ROBOT_ARM	1
#define PROJ_MODE_REMOTE	0

//모드 선택
#define ProjModeState	PROJ_MODE_ROBOT_ARM

//통신 모드 상태
typedef enum{
	COMM_MODE_IDLE = 0,
	COMM_MODE_CAN,
	COMM_MODE_BT,
	COMM_MODE_ERROR
} CommMode;

//시스템 상태
typedef enum{
	SYS_OK = 1,
	SYS_Warning,
	SYS_ERROR
} SysState;

// 송수신 데이터 패킷
typedef struct {
    // 리모콘 → 로봇팔
    uint16_t bendingSensor[2];  // 밴딩센서
    int16_t  gyro_pitch;        // 자이로 앞뒤
    int16_t  gyro_roll;         // 자이로 좌우
    uint8_t  checksum;          // 오류 검출
} RemoteData;

/*

로봇팔에서 리모콘으로 보내는 정보(필요할까?)

typedef struct {
    uint16_t motorAngle[6];     // 서보 6개 각도
} MotorData;

*/


// 시스템 상태
typedef struct {
    uint8_t canStatus;
    uint8_t btStatus;
    uint8_t sysVolt;

    #if (ProjModeState)  // 로봇팔
    uint8_t lcdStatus;
    uint8_t motorStatus[6];
    #else                // 리모콘
    uint8_t bendingStatus[2];
    uint8_t gyroStatus;
    uint8_t switchStatus;
    #endif
} SystemStatus;

extern CommMode currentCommMode;
extern SysState currentSysState;

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_



#endif /* INC_CONFIG_H_ */
