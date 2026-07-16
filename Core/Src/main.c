/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ad9959.h"
#include "dds_calibration.h"
#include "pe4302.h"
#include "serial_debug.h"
#include "tjcHMI.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AD9959_REFERENCE_CLOCK_HZ   25000500U
#define AD9959_PLL_MULTIPLIER       19U
#define AD9959_TEST_FREQUENCY_HZ    35000000U
#define AD9959_STARTUP_RMS_MV       500U
#define TJC_HMI_STARTUP_DELAY_MS    10U
#define TJC_HMI_READ_TIMEOUT_MS     30U
#define TJC_HMI_READ_RETRY_COUNT    3U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static AD9959_Handler g_ad9959;
static PE4302_Handler g_pe4302;
static uint16_t g_direct_amplitude_code;
static TJC_HMI_Settings g_tjc_settings;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  Initializes the AD9959 and enables the CH0 test output.
  */
static void RFMultipathSim_AD9959_Init(void) {
    HAL_GPIO_WritePin(AD9959_PDC_GPIO_Port, AD9959_PDC_Pin, GPIO_PIN_RESET);
    HAL_Delay(50U);

    AD9959_Init(&g_ad9959, &hspi1, AD9959_CS_GPIO_Port, AD9959_CS_Pin, AD9959_RESET_GPIO_Port, AD9959_RESET_Pin, AD9959_UPDATE_GPIO_Port, AD9959_UPDATE_Pin,
                AD9959_REFERENCE_CLOCK_HZ, 0);

    if (AD9959_GetLastStatus(&g_ad9959) != HAL_OK) {
        Error_Handler();
    }

    AD9959_SetClock(&g_ad9959, AD9959_PLL_MULTIPLIER, 0);
    AD9959_Update(&g_ad9959);

    if (AD9959_GetLastStatus(&g_ad9959) != HAL_OK) {
        Error_Handler();
    }

    HAL_Delay(10U);

    AD9959_SetAmplitude(&g_ad9959, Channel_All, 0U);

    if (!DDS_Calibration_GetAmplitudeCode(AD9959_TEST_FREQUENCY_HZ, AD9959_STARTUP_RMS_MV, &g_direct_amplitude_code)) {
        Error_Handler();
    }

    AD9959_SetChannelFrequency(&g_ad9959, Channel_0, AD9959_TEST_FREQUENCY_HZ);
    AD9959_SetPhase(&g_ad9959, Channel_0, 0U);
    AD9959_SetAmplitude(&g_ad9959, Channel_0, g_direct_amplitude_code);

    AD9959_SetChannelFrequency(&g_ad9959, Channel_1, 2000000U);
    AD9959_SetPhase(&g_ad9959, Channel_1, 0U);
    AD9959_SetAmplitude(&g_ad9959, Channel_1, 0U);



    AD9959_SetChannelFrequency(&g_ad9959, Channel_2, 2000000U);
    AD9959_SetPhase(&g_ad9959, Channel_2, 0U);
    AD9959_SetAmplitude(&g_ad9959, Channel_2, 200);

    AD9959_SetChannelFrequency(&g_ad9959, Channel_3, AD9959_TEST_FREQUENCY_HZ);
    AD9959_SetPhase(&g_ad9959, Channel_3, 0U);
    AD9959_SetAmplitude(&g_ad9959, Channel_3, 1023U);



    AD9959_Update(&g_ad9959);

    if (AD9959_GetLastStatus(&g_ad9959) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MPU Configuration--------------------------------------------------------*/
    MPU_Config();

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART2_UART_Init();
    MX_USART1_UART_Init();
    /* USER CODE BEGIN 2 */
    RFMultipathSim_AD9959_Init();

    if (PE4302_Init(&g_pe4302, PE4302_LE_GPIO_Port, PE4302_LE_Pin, PE4302_CLK_GPIO_Port, PE4302_CLK_Pin, PE4302_DATA_GPIO_Port, PE4302_DATA_Pin) != HAL_OK) {
        Error_Handler();
    }
    PE4302_SetAttenuation(&g_pe4302, 6U);

    SerialDebug_Init(&huart1, &g_ad9959, AD9959_TEST_FREQUENCY_HZ, g_direct_amplitude_code, 0);

    if (tjcHMI_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }

    HAL_Delay(TJC_HMI_STARTUP_DELAY_MS);

    HAL_StatusTypeDef tjc_status = HAL_ERROR;
    for (uint32_t attempt = 0U; attempt < TJC_HMI_READ_RETRY_COUNT; ++attempt) {
        tjc_status = tjcReadSettings(&g_tjc_settings, TJC_HMI_READ_TIMEOUT_MS);
        if (tjc_status == HAL_OK) {
            break;
        }
        HAL_Delay(10U);
    }

    if (tjc_status == HAL_OK) {
        const char* mode_name = (g_tjc_settings.mode == 0) ? "CW" : ((g_tjc_settings.mode == 1) ? "AM" : "UNKNOWN");
        printf("TJC SETTINGS MODE=%s(%ld) FC=%ldMHz AMP=%ldmV PHASE=%lddeg DELAY=%ldns ATT=%lddB MOD=%ld%%\r\n", mode_name, (long)g_tjc_settings.mode,
               (long)g_tjc_settings.carrier_frequency_mhz, (long)g_tjc_settings.carrier_amplitude_mv, (long)g_tjc_settings.initial_phase_degree,
               (long)g_tjc_settings.delay_ns, (long)g_tjc_settings.attenuation_db, (long)g_tjc_settings.modulation_percent);
    }
    else {
        printf("ERROR TJC SETTINGS READ STATUS=%d\r\n", (int)tjc_status);
    }

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        SerialDebug_Task();
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Supply configuration update enable
    */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    /** Configure the main internal regulator output voltage
    */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 5;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1 |
        RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void) {
    MPU_Region_InitTypeDef MPU_InitStruct = {0};

    /* Disables the MPU */
    HAL_MPU_Disable();

    /** Initializes and configures the Region and the memory to be protected
    */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x0;
    MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.SubRegionDisable = 0x87;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    /* Enables the MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {}
    /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
