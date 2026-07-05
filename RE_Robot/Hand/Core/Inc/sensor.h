#ifndef __SENSOR_H
#define __SENSOR_H

#include "Packet.h"
#include "main.h"

/* HAL 핸들 (main.c에서 선언) */
extern I2C_HandleTypeDef  hi2c1;
extern ADC_HandleTypeDef  hadc1;

/* 센서 변경 플래그 (인터럽트 콜백에서 세팅) */
extern volatile uint8_t gsensor_changed;
extern volatile uint8_t flex_changed;
extern volatile uint8_t switch_changed;

/* 함수 선언 */
void Sensor_Init(void);
void Sensor_ReadAll(GSensorPacket_t *gPkt, FlexPacket_t *fPkt);

#endif /* __SENSOR_H */
