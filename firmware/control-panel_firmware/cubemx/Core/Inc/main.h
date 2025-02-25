/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32g0xx_hal.h"

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
#define Encoder2_Button_Pin GPIO_PIN_14
#define Encoder2_Button_GPIO_Port GPIOC
#define Encoder0_A_Pin GPIO_PIN_0
#define Encoder0_A_GPIO_Port GPIOA
#define Encoder0_B_Pin GPIO_PIN_1
#define Encoder0_B_GPIO_Port GPIOA
#define Encoder0_Button_Pin GPIO_PIN_2
#define Encoder0_Button_GPIO_Port GPIOA
#define addressBit0_Pin GPIO_PIN_0
#define addressBit0_GPIO_Port GPIOB
#define addressBit1_Pin GPIO_PIN_1
#define addressBit1_GPIO_Port GPIOB
#define addressBit2_Pin GPIO_PIN_2
#define addressBit2_GPIO_Port GPIOB
#define Encoder1_A_Pin GPIO_PIN_8
#define Encoder1_A_GPIO_Port GPIOA
#define Encoder1_B_Pin GPIO_PIN_9
#define Encoder1_B_GPIO_Port GPIOA
#define Encoder1_Button_Pin GPIO_PIN_6
#define Encoder1_Button_GPIO_Port GPIOC
#define ledRed_Pin GPIO_PIN_10
#define ledRed_GPIO_Port GPIOA
#define ledGreen_Pin GPIO_PIN_11
#define ledGreen_GPIO_Port GPIOA
#define Encoder3_Button_Pin GPIO_PIN_15
#define Encoder3_Button_GPIO_Port GPIOA
#define Encoder3_A_Pin GPIO_PIN_4
#define Encoder3_A_GPIO_Port GPIOB
#define Encoder3_B_Pin GPIO_PIN_5
#define Encoder3_B_GPIO_Port GPIOB
#define Encoder2_A_Pin GPIO_PIN_6
#define Encoder2_A_GPIO_Port GPIOB
#define Encoder2_B_Pin GPIO_PIN_7
#define Encoder2_B_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
