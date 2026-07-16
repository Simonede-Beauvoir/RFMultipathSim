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
#include "stm32h7xx_hal.h"

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
#define PE4302_LE_Pin GPIO_PIN_2
#define PE4302_LE_GPIO_Port GPIOF
#define PE4302_CLK_Pin GPIO_PIN_3
#define PE4302_CLK_GPIO_Port GPIOF
#define PE4302_DATA_Pin GPIO_PIN_4
#define PE4302_DATA_GPIO_Port GPIOF
#define LED1_Pin GPIO_PIN_12
#define LED1_GPIO_Port GPIOD
#define LED2_Pin GPIO_PIN_13
#define LED2_GPIO_Port GPIOD
#define GND_Pin GPIO_PIN_14
#define GND_GPIO_Port GPIOD
#define GNDD15_Pin GPIO_PIN_15
#define GNDD15_GPIO_Port GPIOD
#define GNDG2_Pin GPIO_PIN_2
#define GNDG2_GPIO_Port GPIOG
#define GNDG3_Pin GPIO_PIN_3
#define GNDG3_GPIO_Port GPIOG
#define GNDG4_Pin GPIO_PIN_4
#define GNDG4_GPIO_Port GPIOG
#define GNDG5_Pin GPIO_PIN_5
#define GNDG5_GPIO_Port GPIOG
#define GNDG7_Pin GPIO_PIN_7
#define GNDG7_GPIO_Port GPIOG
#define AD9959_CS_Pin GPIO_PIN_0
#define AD9959_CS_GPIO_Port GPIOD
#define AD9959_UPDATE_Pin GPIO_PIN_1
#define AD9959_UPDATE_GPIO_Port GPIOD
#define AD9959_RESET_Pin GPIO_PIN_2
#define AD9959_RESET_GPIO_Port GPIOD
#define AD9959_PDC_Pin GPIO_PIN_3
#define AD9959_PDC_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
