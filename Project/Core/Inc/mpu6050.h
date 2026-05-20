/*
 * mpu6050.h
 *
 *  Created on: 2026. 5. 19.
 *      Author: MinJae
 */

#ifndef INC_MPU6050_H_
#define INC_MPU6050_H_

#include "main.h"

/****************************************************************
    I2C 주소 (AD0 핀 = GND → 0x68)
****************************************************************/
#define MPU6050_ADDR            (0x68 << 1)   // HAL은 8-bit 주소 사용

/****************************************************************
    레지스터 맵
****************************************************************/
#define MPU6050_REG_WHO_AM_I    0x75
#define MPU6050_REG_PWR_MGMT_1  0x6B
#define MPU6050_REG_SMPLRT_DIV  0x19
#define MPU6050_REG_CONFIG      0x1A
#define MPU6050_REG_GYRO_CFG    0x1B
#define MPU6050_REG_ACCEL_CFG   0x1C
#define MPU6050_REG_ACCEL_XOUT  0x3B   // 가속도 X High 시작 (연속 14바이트)
#define MPU6050_WHO_AM_I_VAL    0x68   // WHO_AM_I 응답값

/****************************************************************
    스케일 팩터 (기본 설정)
    가속도: ±2g   → 16384 LSB/g
    자이로: ±250°/s → 131 LSB/(°/s)
****************************************************************/
#define MPU6050_ACCEL_SCALE     16384.0f
#define MPU6050_GYRO_SCALE      131.0f

/****************************************************************
    보완 필터 (Complementary Filter) 계수
    alpha = 자이로 신뢰 비율 (0.98 권장)
    dt    = 호출 주기 (Read_GyroSensor → 10ms)
****************************************************************/
#define MPU6050_ALPHA           0.98f
#define MPU6050_DT              0.01f   // 10ms

/****************************************************************
    함수 선언
****************************************************************/
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MPU6050_GetAngle(I2C_HandleTypeDef *hi2c,
                                   int16_t *pitch, int16_t *roll);

#endif /* INC_MPU6050_H_ */
