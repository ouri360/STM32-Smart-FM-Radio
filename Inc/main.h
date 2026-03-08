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

#include <stdint.h>
extern uint8_t bass_boost;
extern uint8_t volume_act;
extern uint8_t reg_volume;
extern char freq_str[16];
extern uint8_t RSSI;
extern uint16_t raw_battery_adc;
extern uint8_t hauteur_remplissage;
extern char battery_act[16];
extern uint8_t reg_station;

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx_hal.h"

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
#define USER_BTN_Pin GPIO_PIN_13
#define USER_BTN_GPIO_Port GPIOC
#define LED_R_Pin GPIO_PIN_0
#define LED_R_GPIO_Port GPIOC
#define LED_G_Pin GPIO_PIN_1
#define LED_G_GPIO_Port GPIOC
#define LED_B_Pin GPIO_PIN_2
#define LED_B_GPIO_Port GPIOC
#define LEFT_BTN_Pin GPIO_PIN_3
#define LEFT_BTN_GPIO_Port GPIOC
#define CENTER_BTN_Pin GPIO_PIN_0
#define CENTER_BTN_GPIO_Port GPIOA
#define RIGHT_BTN_Pin GPIO_PIN_1
#define RIGHT_BTN_GPIO_Port GPIOA
#define SHUTDOWN_AMPLI_Pin GPIO_PIN_4
#define SHUTDOWN_AMPLI_GPIO_Port GPIOA
#define V_BAT_Pin GPIO_PIN_5
#define V_BAT_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
