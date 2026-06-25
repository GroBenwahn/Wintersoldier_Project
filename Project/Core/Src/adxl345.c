/*
 * adxl345.c  —  ADXL345 3축 가속도계 드라이버
 *
 * MPU6050(보완 필터) 대비 차이점:
 *   - 자이로 없음 → 순수 가속도 tilt 계산
 *   - 보완 필터 대신 지수 이동 평균(LPF) 적용
 *   - 빠른 진동에 약하나 느린 손목 움직임 제어에는 충분
 *
 * ADXL345 데이터 포맷: Little-Endian (LSB 먼저, MPU6050과 반대)
 */

#include "adxl345.h"
#include <math.h>

/****************************************************************
    LPF 상태 (정적 유지)
****************************************************************/
static float s_pitch = 0.0f;
static float s_roll  = 0.0f;

/****************************************************************
    내부 헬퍼: 레지스터 1바이트 쓰기
****************************************************************/
static HAL_StatusTypeDef WriteReg(I2C_HandleTypeDef *hi2c,
                                   uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(hi2c, ADXL345_ADDR,
                             reg, I2C_MEMADD_SIZE_8BIT,
                             &val, 1, 10);
}

/****************************************************************
    Function: ADXL345_Init
    Description: 장치 확인 → 측정 모드 시작
    Return: HAL_OK / HAL_ERROR (DEVID 불일치 포함)
****************************************************************/
HAL_StatusTypeDef ADXL345_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t devid = 0;

    if (HAL_I2C_Mem_Read(hi2c, ADXL345_ADDR,
                         ADXL345_REG_DEVID, I2C_MEMADD_SIZE_8BIT,
                         &devid, 1, 10) != HAL_OK) return HAL_ERROR;
    if (devid != ADXL345_DEVID_VAL) return HAL_ERROR;

    /* 출력 데이터 속도: 50Hz (0x09) — 10ms 폴링에 적합 */
    if (WriteReg(hi2c, ADXL345_REG_BW_RATE,     0x09) != HAL_OK) return HAL_ERROR;

    /* Full Resolution 모드, ±2g 범위 */
    if (WriteReg(hi2c, ADXL345_REG_DATA_FORMAT, 0x08) != HAL_OK) return HAL_ERROR;

    /* 측정 모드 시작 (Measure bit = 1) */
    if (WriteReg(hi2c, ADXL345_REG_POWER_CTL,   0x08) != HAL_OK) return HAL_ERROR;

    return HAL_OK;
}

/****************************************************************
    Function: ADXL345_GetAngle
    Description: XYZ 가속도 읽기 → tilt 각도 → LPF → pitch/roll
                 Read_GyroSensor()에서 10ms마다 호출
    Return: HAL_OK / HAL_ERROR (I2C 실패 시)
    Output: *pitch — 위아래 기울기 (°), *roll — 좌우 회전 (°)

    축 정의 (ADXL345 모듈 기본 방향):
      pitch = atan2(-X, sqrt(Y²+Z²))  — 앞뒤 기울기
      roll  = atan2( Y, sqrt(X²+Z²))  — 좌우 기울기

    모듈 장착 방향에 따라 부호/축을 바꿔야 할 수 있음.
****************************************************************/
HAL_StatusTypeDef ADXL345_GetAngle(I2C_HandleTypeDef *hi2c,
                                    int16_t *pitch, int16_t *roll)
{
    uint8_t buf[6];

    /* 0x32부터 6바이트 연속 읽기: X0 X1 Y0 Y1 Z0 Z1
     * 타임아웃 3ms: 100kHz I2C 기준 6byte ≈ 1ms, 400kHz ≈ 0.25ms.
     * 20ms 시 CommTask 주기(10ms)를 초과 → CAN_SemHandle 누적 →
     * CommTask CPU 독점 → Mode Task 1초 주기로 굶주림(starvation).  */
    if (HAL_I2C_Mem_Read(hi2c, ADXL345_ADDR,
                         ADXL345_REG_DATA_X0, I2C_MEMADD_SIZE_8BIT,
                         buf, 6, 3) != HAL_OK) return HAL_ERROR;

    /* Little-Endian: LSB(buf[0]) + MSB(buf[1]) */
    int16_t xr = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t yr = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t zr = (int16_t)((buf[5] << 8) | buf[4]);

    float x = xr / ADXL345_SCALE;   /* 단위: g */
    float y = yr / ADXL345_SCALE;
    float z = zr / ADXL345_SCALE;

    /* 가속도 → tilt 각도 */
    float raw_pitch = atan2f(-x, sqrtf(y*y + z*z)) * (180.0f / (float)M_PI);
    float raw_roll  = atan2f( y, sqrtf(x*x + z*z)) * (180.0f / (float)M_PI);

    /* 지수 이동 평균 필터 (진동 노이즈 제거) */
    s_pitch = ADXL345_LPF_ALPHA * raw_pitch + (1.0f - ADXL345_LPF_ALPHA) * s_pitch;
    s_roll  = ADXL345_LPF_ALPHA * raw_roll  + (1.0f - ADXL345_LPF_ALPHA) * s_roll;

    *pitch = (int16_t)s_pitch;
    *roll  = (int16_t)s_roll;

    return HAL_OK;
}
