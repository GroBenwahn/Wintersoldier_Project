/*
 * readSensor.c
 *
 *  Created on: 2026. 5. 17.
 *      Author: MinJae
 */


/****************************************************************
    Include Files
****************************************************************/
#include "readSensor.h"
#include "comm_can.h"
#include "bt_comm.h"
#include "adxl345.h"

/****************************************************************
	Function Declaration
****************************************************************/
void ReadSensor_Init(void);
void ReadSensor_Update_10ms(void);
void ReadSensor_Update_100ms(void);
void Read_BendingSensor(void);
void Read_GyroSensor(void);
void Read_SwitchRelay(void);
void Read_MotorStatus(void);
void Read_LCDStatus(void);

/****************************************************************
    External Variables
    (adc, i2c, tim 핸들은 main.h/main.c에서 extern으로 선언됨)
****************************************************************/
uint16_t adcBuffer[2] = {0};
extern ADC_HandleTypeDef  hadc1;
extern I2C_HandleTypeDef  hi2c1;
extern TIM_HandleTypeDef  htim1;
extern TIM_HandleTypeDef  htim2;
extern TIM_HandleTypeDef  htim3;

/****************************************************************
    로컬 상태 변수 정의
    — comm_can_controller/robot TX 함수가 extern으로 읽어서 CAN 송신에 사용
****************************************************************/
#if (!ProjModeState)             // 리모콘 전용
uint8_t localSensorStatus = 0;   // bit0=bending0, bit1=bending1, bit2=gyro 이상
volatile uint8_t DIAG_adxl_init = 0;   /* Live Expression: 0=init실패(Standby), 1=정상 */
#endif

#if (ProjModeState)              // 로봇팔 전용
uint8_t localLcdStatus      = 0;
uint8_t localMotorStatus[6] = {0};
#endif

uint8_t localRelayStatus  = 0;

/****************************************************************
    Function: ReadSensor_Init
    Description: 센서 초기화
****************************************************************/
void ReadSensor_Init(void) {
#if (!ProjModeState)
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcBuffer, 2);
    /* ADC DMA는 adcBuffer를 폴링으로 읽으므로 완료 IRQ 불필요.
     * DMA1_CH2 IRQ(우선순위 5)가 2.82μs마다 발생하면 SysTick(우선순위 15)을
     * 선점해 FreeRTOS tick이 지연되므로 영구적으로 비활성화한다.          */
    HAL_NVIC_DisableIRQ(DMA1_Channel2_IRQn);

    /* ADXL345 초기화 — I2C 버스 안정화 후 최대 3회 재시도
     * 실패 시 POWER_CTL 미설정 → Standby 모드 → GetAngle이 항상 0 반환
     * DIAG_adxl_init: 0=실패(Standby), 1=정상(Measure 모드)               */
    HAL_Delay(10);   /* 전원 인가 후 ADXL345 안정화 대기 (~1.1ms 필요) */
    for (uint8_t i = 0; i < 3; i++) {
        if (ADXL345_Init(&hi2c1) == HAL_OK) {
            DIAG_adxl_init = 1;
            break;
        }
        HAL_Delay(20);
    }
#endif
}

/****************************************************************
    Function: ReadSensor_Update_10ms
    Description: 빠른 주기 (Timer10ms) — 핵심 센서 읽기
****************************************************************/
void ReadSensor_Update_10ms(void) {
#if (!ProjModeState)
    Read_BendingSensor();
    Read_GyroSensor();
#endif

#if (ProjModeState)
    Read_MotorStatus();
#endif
}

/****************************************************************
    Function: ReadSensor_Update_100ms
    Description: 느린 주기 (Timer100ms) — 상태 확인
****************************************************************/
void ReadSensor_Update_100ms(void) {
#if (!ProjModeState)
    Read_SwitchRelay();
#endif

#if (ProjModeState)
    Read_LCDStatus();
#endif
}

/* ============================================================
   리모콘 전용 함수
   ============================================================ */
#if (!ProjModeState)

/****************************************************************
    Function: Read_BendingSensor
    Description: ADC DMA 버퍼값 → remoteSensorTx 업데이트
                 이상값 감지 시 localSensorStatus bit 세팅
****************************************************************/
void Read_BendingSensor(void) {
    remoteSensorTx.bendingSensor[0] = adcBuffer[0];
    remoteSensorTx.bendingSensor[1] = adcBuffer[1];

    // 센서 이상 감지 (범위: 0~4095, 0 또는 최대값이면 단선/단락 의심)
    if(adcBuffer[0] == 0 || adcBuffer[0] == 4095) {
        localSensorStatus |= (1 << 0);
    } else {
        localSensorStatus &= ~(1 << 0);
    }

    if(adcBuffer[1] == 0 || adcBuffer[1] == 4095) {
        localSensorStatus |= (1 << 1);
    } else {
        localSensorStatus &= ~(1 << 1);
    }
}

/****************************************************************
    Function: Read_GyroSensor
    Description: MPU6050 I2C → remoteSensorTx 업데이트
****************************************************************/
void Read_GyroSensor(void) {
    /* init 실패 상태(Standby)면 주기적으로 재시도 (1초 = 100 × 10ms) */
    if (DIAG_adxl_init == 0) {
        static uint8_t reinit_cnt = 0;
        if (++reinit_cnt >= 100) {
            reinit_cnt = 0;
            if (ADXL345_Init(&hi2c1) == HAL_OK) {
                DIAG_adxl_init = 1;
            }
        }
        localSensorStatus |= (1 << 2);
        return;
    }

    HAL_StatusTypeDef ret = ADXL345_GetAngle(&hi2c1,
                                              &remoteSensorTx.gyro_pitch,
                                              &remoteSensorTx.gyro_roll);
    if (ret != HAL_OK) {
        localSensorStatus |=  (1 << 2);
        DIAG_adxl_init = 0;   /* I2C 에러 → 다음 주기에 재초기화 시도 */
    } else {
        localSensorStatus &= ~(1 << 2);
    }
}
#endif  // !ProjModeState


/****************************************************************
    Function: Read_SwitchRelay
    Description: 스위치/릴레이 GPIO 상태 → 로컬 변수 업데이트
                 Pack_Remote_CAN_Message(0x101)이 읽어서 송신
****************************************************************/
void Read_SwitchRelay(void) {
    // localSwitchStatus: Comm_Power_Select.c가 관리 (버튼 토글 시 갱신)
    // localRelayStatus:  릴레이 출력 핀 실제 레벨 읽기 → CAN 0x101 상태 보고용
    localRelayStatus =
        (HAL_GPIO_ReadPin(GPIO_Relay_Output_GPIO_Port,
                          GPIO_Relay_Output_Pin) == GPIO_PIN_SET) ? 1 : 0;
}



/* ============================================================
   로봇팔 전용 함수
   ============================================================ */
#if (ProjModeState)

/****************************************************************
    Function: Read_MotorStatus
    Description: 타이머 CCR 현재값으로 PWM 출력 여부 → localMotorStatus[]
                 Pack_Robot_CAN_Message(0x202)이 읽어서 송신
****************************************************************/
void Read_MotorStatus(void) {
    localMotorStatus[0] = (uint8_t)(htim1.Instance->CCR1 > 0);
    localMotorStatus[1] = (uint8_t)(htim1.Instance->CCR2 > 0);
    localMotorStatus[2] = (uint8_t)(htim2.Instance->CCR1 > 0);
    localMotorStatus[3] = (uint8_t)(htim3.Instance->CCR1 > 0);
    localMotorStatus[4] = (uint8_t)(htim3.Instance->CCR2 > 0);
    localMotorStatus[5] = (uint8_t)(htim3.Instance->CCR3 > 0);
}

/****************************************************************
    Function: Read_LCDStatus
    Description: I2C ACK 확인 → localLcdStatus
****************************************************************/
void Read_LCDStatus(void) {
    HAL_StatusTypeDef ret =
        HAL_I2C_IsDeviceReady(&hi2c1, 0x27 << 1, 1, 10);

    localLcdStatus = (ret == HAL_OK) ? 1 : 0;
}

#endif  // ProjModeState
