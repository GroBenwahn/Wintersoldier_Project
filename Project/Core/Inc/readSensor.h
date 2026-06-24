/*
 * readSensor.h
 *
 *  Created on: 2026. 5. 17.
 *      Author: MinJae
 */

#ifndef INC_READSENSOR_H_
#define INC_READSENSOR_H_

#include "main.h"
#include "config.h"

/****************************************************************
    로컬 상태 변수 extern 선언
    — 이 보드 자신의 상태값 (comm_can_controller/robot TX 함수가 읽어감)
****************************************************************/
#if (!ProjModeState)                // 리모콘 전용
extern uint8_t localSensorStatus;   // bit0=bending0, bit1=bending1, bit2=gyro 이상
#endif
#if (ProjModeState)                 // 로봇팔 전용
extern uint8_t localLcdStatus;      // 0=정상, 1=이상
extern uint8_t localMotorStatus[6]; // 모터 0~5 PWM 출력 여부
#endif

extern uint8_t localRelayStatus;    // 릴레이 출력 상태 (0=CAN측, 1=BT측)
/****************************************************************
    Function Declaration
****************************************************************/
void ReadSensor_Init(void);
void ReadSensor_Update_10ms(void);    // Timer10ms 콜백에서 호출
void ReadSensor_Update_100ms(void);   // Timer100ms 콜백에서 호출

/* ── 리모콘 전용 ─────────────────────────────────────────── */
#if (!ProjModeState)
void Read_BendingSensor(void);   // ADC DMA → remoteSensorTx.bendingSensor[]
void Read_GyroSensor(void);      // I2C ADXL345 → remoteSensorTx.gyro_pitch/roll
void Read_SwitchRelay(void);     // GPIO → localSwitchStatus / localRelayStatus
#endif

/* ── 로봇팔 전용 ─────────────────────────────────────────── */
#if (ProjModeState)
void Read_MotorStatus(void);     // CCR 레지스터 → localMotorStatus[]
void Read_LCDStatus(void);       // I2C ACK 확인 → localLcdStatus
#endif


#endif /* INC_READSENSOR_H_ */
