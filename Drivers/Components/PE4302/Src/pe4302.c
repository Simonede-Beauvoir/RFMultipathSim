#include "pe4302.h"

#define PE4302_SERIAL_DELAY_CYCLES    256U

static void PE4302_SerialDelay(void);

/**
  * @brief  Initializes the PE4302 serial control interface.
  * @param  device Pointer to the PE4302 handle.
  * @param  le_port Latch-enable GPIO port.
  * @param  le_pin Latch-enable GPIO pin.
  * @param  clk_port Serial-clock GPIO port.
  * @param  clk_pin Serial-clock GPIO pin.
  * @param  data_port Serial-data GPIO port.
  * @param  data_pin Serial-data GPIO pin.
  * @retval HAL_OK The device handle was initialized.
  * @retval HAL_ERROR One or more parameters are invalid.
  */
HAL_StatusTypeDef PE4302_Init(PE4302_Handler* device, GPIO_TypeDef* le_port, uint16_t le_pin, GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port,
                              uint16_t data_pin) {
    if ((device == NULL) || (le_port == NULL) || (le_pin == 0U) || (clk_port == NULL) || (clk_pin == 0U) || (data_port == NULL) || (data_pin == 0U)) {
        return HAL_ERROR;
    }

    device->le_port = le_port;
    device->le_pin = le_pin;
    device->clk_port = clk_port;
    device->clk_pin = clk_pin;
    device->data_port = data_port;
    device->data_pin = data_pin;
    device->attenuation_code = 0U;

    HAL_GPIO_WritePin(device->le_port, device->le_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(device->clk_port, device->clk_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(device->data_port, device->data_pin, GPIO_PIN_RESET);

    return PE4302_SetAttenuationCode(device, 0U);
}

/**
  * @brief  Sets attenuation in whole-decibel units.
  * @param  device Pointer to the PE4302 handle.
  * @param  attenuation_db Attenuation from 0 dB to 31 dB.
  * @retval HAL_OK The attenuation word was transferred.
  * @retval HAL_ERROR The device pointer or attenuation value is invalid.
  */
HAL_StatusTypeDef PE4302_SetAttenuation(PE4302_Handler* device, uint8_t attenuation_db) {
    if ((device == NULL) || (attenuation_db > PE4302_ATTENUATION_DB_MAX)) {
        return HAL_ERROR;
    }

    return PE4302_SetAttenuationCode(device, (uint8_t)(attenuation_db * 2U));
}

/**
  * @brief  Sets attenuation using 0.5 dB control steps.
  * @param  device Pointer to the PE4302 handle.
  * @param  attenuation_code Control word from 0 to 63.
  * @retval HAL_OK The attenuation word was transferred.
  * @retval HAL_ERROR The device pointer or control word is invalid.
  */
HAL_StatusTypeDef PE4302_SetAttenuationCode(PE4302_Handler* device, uint8_t attenuation_code) {
    int32_t bit;

    if ((device == NULL) || (attenuation_code > PE4302_ATTENUATION_CODE_MAX)) {
        return HAL_ERROR;
    }

    HAL_GPIO_WritePin(device->le_port, device->le_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(device->clk_port, device->clk_pin, GPIO_PIN_RESET);

    for (bit = 5; bit >= 0; bit--) {
        GPIO_PinState data_state;

        if (((attenuation_code >> (uint32_t)bit) & 0x01U) != 0U) {
            data_state = GPIO_PIN_SET;
        }
        else {
            data_state = GPIO_PIN_RESET;
        }

        HAL_GPIO_WritePin(device->data_port, device->data_pin, data_state);
        PE4302_SerialDelay();

        HAL_GPIO_WritePin(device->clk_port, device->clk_pin, GPIO_PIN_SET);
        PE4302_SerialDelay();

        HAL_GPIO_WritePin(device->clk_port, device->clk_pin, GPIO_PIN_RESET);
        PE4302_SerialDelay();
    }

    HAL_GPIO_WritePin(device->le_port, device->le_pin, GPIO_PIN_SET);
    PE4302_SerialDelay();
    HAL_GPIO_WritePin(device->le_port, device->le_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(device->data_port, device->data_pin, GPIO_PIN_RESET);

    device->attenuation_code = attenuation_code;

    return HAL_OK;
}

/**
  * @brief  Returns the most recently transferred attenuation code.
  * @param  device Pointer to the PE4302 handle.
  * @return Attenuation code from 0 to 63.
  */
uint8_t PE4302_GetAttenuationCode(const PE4302_Handler* device) {
    if (device == NULL) {
        return 0U;
    }

    return device->attenuation_code;
}

/**
  * @brief  Provides a conservative delay for the serial interface.
  */
static void PE4302_SerialDelay(void) {
    volatile uint32_t count;

    for (count = 0U; count < PE4302_SERIAL_DELAY_CYCLES; count++) {
        __NOP();
    }
}
