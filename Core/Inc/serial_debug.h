#ifndef SERIAL_DEBUG_H
#define SERIAL_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ad9959.h"
#include "stm32h7xx_hal.h"

/**
  * @brief  Initializes USART command processing and printf redirection.
  * @param  uart USART handle used for command input and printf output.
  * @param  ad9959 AD9959 device controlled by serial commands.
  * @param  frequency_hz Initial CH0 frequency in hertz.
  * @param  amplitude_code Initial CH0 amplitude code from 0 to 1023.
  * @param  phase_degree Initial CH0 phase in degrees.
  */
void SerialDebug_Init(UART_HandleTypeDef* uart, AD9959_Handler* ad9959, uint32_t frequency_hz, uint16_t amplitude_code, int32_t phase_degree);

/**
  * @brief  Polls the debug USART and executes complete command lines.
  */
void SerialDebug_Task(void);

#ifdef __cplusplus
}
#endif

#endif
