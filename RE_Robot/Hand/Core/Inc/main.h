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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Flex_Sensor_01_Pin GPIO_PIN_0
#define Flex_Sensor_01_GPIO_Port GPIOA
#define Flex_Sensor_02_Pin GPIO_PIN_1
#define Flex_Sensor_02_GPIO_Port GPIOA
#define Flex_Sensor_03_Pin GPIO_PIN_2
#define Flex_Sensor_03_GPIO_Port GPIOA
#define G_Sensor_INT_Pin GPIO_PIN_7
#define G_Sensor_INT_GPIO_Port GPIOA
#define G_Sensor_INT_EXTI_IRQn EXTI9_5_IRQn
#define BIuetooth_RX_05_Pin GPIO_PIN_10
#define BIuetooth_RX_05_GPIO_Port GPIOA
#define CAN_RX_01_Pin GPIO_PIN_11
#define CAN_RX_01_GPIO_Port GPIOA
#define CAN_TX_01_Pin GPIO_PIN_12
#define CAN_TX_01_GPIO_Port GPIOA
#define G_Sensor_SCL_Pin GPIO_PIN_15
#define G_Sensor_SCL_GPIO_Port GPIOA
#define Up_Switch_Pin GPIO_PIN_4
#define Up_Switch_GPIO_Port GPIOB
#define Up_Switch_EXTI_IRQn EXTI4_IRQn
#define Down_Switch_Pin GPIO_PIN_5
#define Down_Switch_GPIO_Port GPIOB
#define Down_Switch_EXTI_IRQn EXTI9_5_IRQn
#define BIuetooth_TX_05_Pin GPIO_PIN_6
#define BIuetooth_TX_05_GPIO_Port GPIOB
#define G_Sensor_SDA_Pin GPIO_PIN_7
#define G_Sensor_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
