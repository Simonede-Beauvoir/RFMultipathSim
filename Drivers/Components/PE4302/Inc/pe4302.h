#ifndef PE4302_H
#define PE4302_H

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PE4302_ATTENUATION_CODE_MAX    63U
#define PE4302_ATTENUATION_DB_MAX      31U

typedef struct {
    GPIO_TypeDef* le_port;
    uint16_t le_pin;
    GPIO_TypeDef* clk_port;
    uint16_t clk_pin;
    GPIO_TypeDef* data_port;
    uint16_t data_pin;
    uint8_t attenuation_code;
} PE4302_Handler;

HAL_StatusTypeDef PE4302_Init(PE4302_Handler* device, GPIO_TypeDef* le_port, uint16_t le_pin, GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port,
                              uint16_t data_pin);
HAL_StatusTypeDef PE4302_SetAttenuation(PE4302_Handler* device, uint8_t attenuation_db);
HAL_StatusTypeDef PE4302_SetAttenuationCode(PE4302_Handler* device, uint8_t attenuation_code);
uint8_t PE4302_GetAttenuationCode(const PE4302_Handler* device);

#ifdef __cplusplus
}
#endif

#endif
