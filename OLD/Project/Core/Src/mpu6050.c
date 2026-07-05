/*
 * mpu6050.c
 *
 *  Created on: 2026. 5. 19.
 *      Author: MinJae
 */

#include "mpu6050.h"
#include <math.h>

/****************************************************************
    보완 필터 상태 (정적 유지)
****************************************************************/
static float s_pitch = 0.0f;
static float s_roll  = 0.0f;

/****************************************************************
    내부 헬퍼: 레지스터 1바이트 쓰기
****************************************************************/
static HAL_StatusTypeDef WriteReg(I2C_HandleTypeDef *hi2c,
                                  uint8_t reg, uint8_t val) {
    return HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR,
                             reg, I2C_MEMADD_SIZE_8BIT,
                             &val, 1, 10);
}

/****************************************************************
    Function: MPU6050_Init
    Description: 슬립 해제, 샘플레이트/필터/범위 설정
    Return: HAL_OK / HAL_ERROR (WHO_AM_I 불일치 포함)
****************************************************************/
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t who = 0;

    // 장치 확인
    if(HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR,
                        MPU6050_REG_WHO_AM_I, I2C_MEMADD_SIZE_8BIT,
                        &who, 1, 10) != HAL_OK) return HAL_ERROR;
    if(who != MPU6050_WHO_AM_I_VAL) return HAL_ERROR;

    // 슬립 모드 해제 (클럭 소스: 내부 8MHz)
    if(WriteReg(hi2c, MPU6050_REG_PWR_MGMT_1, 0x00) != HAL_OK) return HAL_ERROR;

    // 샘플레이트 = 1kHz / (1 + 7) = 125Hz
    if(WriteReg(hi2c, MPU6050_REG_SMPLRT_DIV, 0x07) != HAL_OK) return HAL_ERROR;

    // DLPF 저역통과 필터 ~44Hz (진동 노이즈 제거)
    if(WriteReg(hi2c, MPU6050_REG_CONFIG,    0x03) != HAL_OK) return HAL_ERROR;

    // 자이로 범위: ±250°/s
    if(WriteReg(hi2c, MPU6050_REG_GYRO_CFG,  0x00) != HAL_OK) return HAL_ERROR;

    // 가속도 범위: ±2g
    if(WriteReg(hi2c, MPU6050_REG_ACCEL_CFG, 0x00) != HAL_OK) return HAL_ERROR;

    return HAL_OK;
}

/****************************************************************
    Function: MPU6050_GetAngle
    Description: 가속도 + 자이로 읽기 → 보완 필터 → pitch/roll (°)
                 Read_GyroSensor()에서 10ms마다 호출
    Return: HAL_OK / HAL_ERROR (I2C 실패 시)
    Output: *pitch, *roll — 단위: 정수 도(°), 범위 -180 ~ +180
****************************************************************/
HAL_StatusTypeDef MPU6050_GetAngle(I2C_HandleTypeDef *hi2c,
                                   int16_t *pitch, int16_t *roll) {
    uint8_t buf[14];

    // 0x3B부터 14바이트 연속 읽기
    // [0~5]=Accel XYZ, [6~7]=Temp, [8~13]=Gyro XYZ
    if(HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR,
                        MPU6050_REG_ACCEL_XOUT, I2C_MEMADD_SIZE_8BIT,
                        buf, 14, 20) != HAL_OK) return HAL_ERROR;

    int16_t ax_r = (int16_t)(buf[0]  << 8 | buf[1]);
    int16_t ay_r = (int16_t)(buf[2]  << 8 | buf[3]);
    int16_t az_r = (int16_t)(buf[4]  << 8 | buf[5]);
    // buf[6~7]: 온도 (미사용)
    int16_t gx_r = (int16_t)(buf[8]  << 8 | buf[9]);
    int16_t gy_r = (int16_t)(buf[10] << 8 | buf[11]);

    float ax = ax_r / MPU6050_ACCEL_SCALE;
    float ay = ay_r / MPU6050_ACCEL_SCALE;
    float az = az_r / MPU6050_ACCEL_SCALE;
    float gx = gx_r / MPU6050_GYRO_SCALE;   // °/s
    float gy = gy_r / MPU6050_GYRO_SCALE;   // °/s

    // 가속도계 단독 각도 계산
    float accel_pitch = atan2f(ax, sqrtf(ay*ay + az*az)) * (180.0f / (float)M_PI);
    float accel_roll  = atan2f(ay, sqrtf(ax*ax + az*az)) * (180.0f / (float)M_PI);

    // 보완 필터: 자이로 적분 98% + 가속도계 2%
    s_pitch = MPU6050_ALPHA * (s_pitch + gx * MPU6050_DT)
            + (1.0f - MPU6050_ALPHA) * accel_pitch;
    s_roll  = MPU6050_ALPHA * (s_roll  + gy * MPU6050_DT)
            + (1.0f - MPU6050_ALPHA) * accel_roll;

    *pitch = (int16_t)s_pitch;
    *roll  = (int16_t)s_roll;

    return HAL_OK;
}
