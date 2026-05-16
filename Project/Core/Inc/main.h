/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

#include "stm32g4xx_nucleo.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Clock_Timing_IN_Pin GPIO_PIN_0
#define Clock_Timing_IN_GPIO_Port GPIOF
#define Clock_Timing_OUT_Pin GPIO_PIN_1
#define Clock_Timing_OUT_GPIO_Port GPIOF
#define Bending_Sensor_01_Pin GPIO_PIN_0
#define Bending_Sensor_01_GPIO_Port GPIOA
#define Bending_Sensor_02_Pin GPIO_PIN_1
#define Bending_Sensor_02_GPIO_Port GPIOA
#define TIM3_Motor_CH2_Pin GPIO_PIN_4
#define TIM3_Motor_CH2_GPIO_Port GPIOA
#define TIM2_Motor_CH1_Pin GPIO_PIN_5
#define TIM2_Motor_CH1_GPIO_Port GPIOA
#define TIM3_Motor_CH1_Pin GPIO_PIN_6
#define TIM3_Motor_CH1_GPIO_Port GPIOA
#define GPIO_Input_Switch_Pin GPIO_PIN_7
#define GPIO_Input_Switch_GPIO_Port GPIOA
#define TIM3_Motor_CH3_Pin GPIO_PIN_0
#define TIM3_Motor_CH3_GPIO_Port GPIOB
#define TIM1_Motor_CH1_Pin GPIO_PIN_8
#define TIM1_Motor_CH1_GPIO_Port GPIOA
#define TIM1_Motor_CH2_Pin GPIO_PIN_9
#define TIM1_Motor_CH2_GPIO_Port GPIOA
#define Bluetooth_RX_Pin GPIO_PIN_10
#define Bluetooth_RX_GPIO_Port GPIOA
#define CAN_RX_Pin GPIO_PIN_11
#define CAN_RX_GPIO_Port GPIOA
#define CAN_TX_Pin GPIO_PIN_12
#define CAN_TX_GPIO_Port GPIOA
#define T_SWDIO_Pin GPIO_PIN_13
#define T_SWDIO_GPIO_Port GPIOA
#define T_SWCLK_Pin GPIO_PIN_14
#define T_SWCLK_GPIO_Port GPIOA
#define Gyro_Sensro_LDC_SCL_Pin GPIO_PIN_15
#define Gyro_Sensro_LDC_SCL_GPIO_Port GPIOA
#define T_SWO_Pin GPIO_PIN_3
#define T_SWO_GPIO_Port GPIOB
#define Relay_Output_Pin GPIO_PIN_5
#define Relay_Output_GPIO_Port GPIOB
#define Bluetooth_TX_Pin GPIO_PIN_6
#define Bluetooth_TX_GPIO_Port GPIOB
#define Gyro_Sensor_LDC_SDA_Pin GPIO_PIN_7
#define Gyro_Sensor_LDC_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
