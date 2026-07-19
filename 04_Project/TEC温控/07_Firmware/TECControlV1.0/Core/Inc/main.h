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
#define TEC_VSEN_Pin GPIO_PIN_0
#define TEC_VSEN_GPIO_Port GPIOA
#define TEC_PWMN_Pin GPIO_PIN_1
#define TEC_PWMN_GPIO_Port GPIOA
#define TEC_PWM_Pin GPIO_PIN_2
#define TEC_PWM_GPIO_Port GPIOA
#define TEC_VSET_Pin GPIO_PIN_4
#define TEC_VSET_GPIO_Port GPIOA
#define U14_13_Pin GPIO_PIN_5
#define U14_13_GPIO_Port GPIOA
#define U14_14_Pin GPIO_PIN_6
#define U14_14_GPIO_Port GPIOA
#define U14_15_Pin GPIO_PIN_7
#define U14_15_GPIO_Port GPIOA
#define PWR_ON_Pin GPIO_PIN_0
#define PWR_ON_GPIO_Port GPIOB
#define PWR_EN_Pin GPIO_PIN_1
#define PWR_EN_GPIO_Port GPIOB
#define DAC_SYNC_Pin GPIO_PIN_12
#define DAC_SYNC_GPIO_Port GPIOB
#define ADC_DRDY_Pin GPIO_PIN_6
#define ADC_DRDY_GPIO_Port GPIOC
#define ADC_CS_Pin GPIO_PIN_11
#define ADC_CS_GPIO_Port GPIOA
#define U1_DE_Pin GPIO_PIN_12
#define U1_DE_GPIO_Port GPIOA
#define IO0_Pin GPIO_PIN_10
#define IO0_GPIO_Port GPIOC
#define IO1_Pin GPIO_PIN_11
#define IO1_GPIO_Port GPIOC
#define IO2_Pin GPIO_PIN_3
#define IO2_GPIO_Port GPIOB
#define MPWM1_Pin GPIO_PIN_4
#define MPWM1_GPIO_Port GPIOB
#define MPWM2_Pin GPIO_PIN_5
#define MPWM2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
