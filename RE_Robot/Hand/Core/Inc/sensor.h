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

/* 센서 원시값 (패킷 조립 전 중간 데이터) */
typedef struct
{
    int16_t        roll;
    int16_t        pitch;
    HeightSwitch_t height_sw;
    uint16_t       flex1;
    uint16_t       flex2;
} SensorData_t;

/* 함수 선언 */
void Sensor_Init(void);
void Sensor_ReadAll(SensorData_t *data);
void Sensor_PackGSensor(const SensorData_t *data, GSensorPacket_t *pkt);
void Sensor_PackFlex(const SensorData_t *data, FlexPacket_t *pkt);
void Sensor_ResetFlexBaseline(void);

#endif /* __SENSOR_H */
