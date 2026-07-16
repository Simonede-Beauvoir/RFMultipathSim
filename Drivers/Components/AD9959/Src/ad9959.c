#include "ad9959.h"

#define AD9959_SPI_TIMEOUT_MS    100U
#define AD9959_PPB_SCALE         1000000000LL

static HAL_StatusTypeDef AD9959_Write(AD9959_Handler* device, AD9959_Register reg, uint32_t value);
static uint8_t AD9959_GetRegisterLength(AD9959_Register reg);
static void AD9959_ShortDelay(void);

/**
  * @brief  Initializes the AD9959 handle and restores a known state.
  * @param  device Pointer to the AD9959 handle.
  * @param  hspi Pointer to the STM32 HAL SPI handle.
  * @param  device_cs_port Chip-select GPIO port.
  * @param  device_cs_pin Chip-select GPIO pin.
  * @param  device_mrst_port Master-reset GPIO port.
  * @param  device_mrst_pin Master-reset GPIO pin.
  * @param  device_update_port I/O_UPDATE GPIO port.
  * @param  device_update_pin I/O_UPDATE GPIO pin.
  * @param  device_crystal_clock Reference clock frequency in hertz.
  * @param  device_calibration Reference clock correction in ppb.
  */
void AD9959_Init(AD9959_Handler* device, SPI_HandleTypeDef* hspi, GPIO_TypeDef* device_cs_port, uint16_t device_cs_pin, GPIO_TypeDef* device_mrst_port,
                 uint16_t device_mrst_pin, GPIO_TypeDef* device_update_port, uint16_t device_update_pin, uint32_t device_crystal_clock,
                 int32_t device_calibration) {
    if ((device == NULL) || (hspi == NULL) || (device_cs_port == NULL) || (device_mrst_port == NULL) || (device_update_port == NULL) || (device_crystal_clock ==
        0U)) {
        return;
    }

    device->SPI_handler = hspi;
    device->cs_port = device_cs_port;
    device->cs_pin = device_cs_pin;
    device->mrst_port = device_mrst_port;
    device->mrst_pin = device_mrst_pin;
    device->update_port = device_update_port;
    device->update_pin = device_update_pin;
    device->crystal_clock = device_crystal_clock;
    device->calibration_factor = device_calibration;
    device->multiplier = 1U;
    device->core_clock = device_crystal_clock;
    device->last_channels = (AD9959_Channel)0xFFU;
    device->last_status = HAL_OK;

    if (device->SPI_handler->Init.Direction == SPI_DIRECTION_1LINE) {
        device->IO_mode = IO2Wire;
    }
    else {
        device->IO_mode = IO3Wire;
    }

    HAL_GPIO_WritePin(device->cs_port, device->cs_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(device->update_port, device->update_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(device->mrst_port, device->mrst_pin, GPIO_PIN_RESET);

    AD9959_Reset(device, (AD9959_CFR_Bits)0U);
}

/**
  * @brief  Performs a master reset and loads the initial channel state.
  * @param  device Pointer to the AD9959 handle.
  * @param  device_cfr Initial channel function register value. A zero
  *         value selects the driver default configuration.
  */
void AD9959_Reset(AD9959_Handler* device, AD9959_CFR_Bits device_cfr) {
    if (device == NULL) {
        return;
    }

    if (device_cfr == 0U) {
        device_cfr = (AD9959_CFR_Bits)(DACFullScale | MatchPipeDelay | OutputSineWave | AutoclearPhase | AutoclearSweep);
    }

    HAL_GPIO_WritePin(device->cs_port, device->cs_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(device->update_port, device->update_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(device->mrst_port, device->mrst_pin, GPIO_PIN_SET);
    HAL_Delay(1U);
    HAL_GPIO_WritePin(device->mrst_port, device->mrst_pin, GPIO_PIN_RESET);
    HAL_Delay(1U);

    device->last_channels = (AD9959_Channel)0xFFU;
    AD9959_SetChannels(device, Channel_All);
    if (device->last_status != HAL_OK) {
        return;
    }

    AD9959_Write(device, CFR, (uint32_t)device_cfr);
    if (device->last_status != HAL_OK) {
        return;
    }

    AD9959_SetChannels(device, Channel_None);
    if (device->last_status != HAL_OK) {
        return;
    }

    AD9959_SetClock(device, 0U, device->calibration_factor);
    if (device->last_status != HAL_OK) {
        return;
    }

    AD9959_Update(device);
}

/**
  * @brief  Configures the reference clock multiplier and core clock.
  * @param  device Pointer to the AD9959 handle.
  * @param  mult PLL multiplier from 4 to 20, or another value to bypass
  *         the PLL.
  * @param  calibration Reference clock correction in ppb.
  */
void AD9959_SetClock(AD9959_Handler* device, uint8_t mult, int32_t calibration) {
    uint8_t register_multiplier;
    uint8_t effective_multiplier;
    uint8_t fr1_msb;
    uint32_t transfer_data;
    int64_t correction_scale;
    uint64_t corrected_reference;
    uint64_t core_clock;

    if ((device == NULL) || (device->crystal_clock == 0U)) {
        return;
    }

    if ((mult >= 4U) && (mult <= 20U)) {
        register_multiplier = mult;
        effective_multiplier = mult;
    }
    else {
        register_multiplier = 0U;
        effective_multiplier = 1U;
    }

    correction_scale = AD9959_PPB_SCALE + (int64_t)calibration;
    if (correction_scale <= 0LL) {
        device->last_status = HAL_ERROR;
        return;
    }

    corrected_reference = ((uint64_t)device->crystal_clock * (uint64_t)correction_scale + 500000000ULL) / (uint64_t)AD9959_PPB_SCALE;
    core_clock = corrected_reference * effective_multiplier;

    if ((core_clock > AD9959_MAX_SYSCLK_HZ) || ((register_multiplier != 0U) && (core_clock > 160000000ULL) && (core_clock < 255000000ULL))) {
        device->last_status = HAL_ERROR;
        return;
    }

    fr1_msb = (uint8_t)((register_multiplier << 2U) | ChargePump3);
    if ((register_multiplier != 0U) && (core_clock >= 255000000ULL)) {
        fr1_msb |= VCOGain;
    }

    transfer_data = ((uint32_t)fr1_msb << 16U) | ((uint32_t)(ModLevels2 | RampUpDownOff | Profile0) << 8U) | (uint32_t)SyncClkDisable;
    if (AD9959_Write(device, FR1, transfer_data) != HAL_OK) {
        return;
    }

    device->multiplier = effective_multiplier;
    device->calibration_factor = calibration;
    device->core_clock = core_clock;
}

/**
  * @brief  Selects the channels that receive channel register writes.
  * @param  device Pointer to the AD9959 handle.
  * @param  chan Channel selection mask.
  */
void AD9959_SetChannels(AD9959_Handler* device, AD9959_Channel chan) {
    uint8_t csr_value;

    if (device == NULL) {
        return;
    }

    if (device->last_channels == chan) {
        device->last_status = HAL_OK;
        return;
    }

    csr_value = (uint8_t)chan | (uint8_t)MSB_First | (uint8_t)device->IO_mode;
    if (AD9959_Write(device, CSR, csr_value) == HAL_OK) {
        device->last_channels = chan;
    }
}

/**
  * @brief  Converts an output frequency to a 32-bit tuning word.
  * @param  device Pointer to the AD9959 handle.
  * @param  freq Requested output frequency in hertz.
  * @return The calculated 32-bit frequency tuning word.
  */
uint32_t AD9959_GenerateFrequencyDelta(const AD9959_Handler* device, uint32_t freq) {
    uint64_t numerator;

    if ((device == NULL) || (device->core_clock == 0ULL)) {
        return 0U;
    }

    numerator = ((uint64_t)freq << 32U) + (device->core_clock / 2ULL);
    return (uint32_t)(numerator / device->core_clock);
}

/**
  * @brief  Writes a channel frequency to the channel tuning register.
  * @param  device Pointer to the AD9959 handle.
  * @param  chan Channel selection mask.
  * @param  freq Requested output frequency in hertz.
  */
void AD9959_SetChannelFrequency(AD9959_Handler* device, AD9959_Channel chan, uint32_t freq) {
    if (device == NULL) {
        return;
    }

    AD9959_SetChannelDelta(device, chan, AD9959_GenerateFrequencyDelta(device, freq));
}

/**
  * @brief  Writes a 32-bit tuning word to selected channels.
  * @param  device Pointer to the AD9959 handle.
  * @param  chan Channel selection mask.
  * @param  delta Frequency tuning word.
  */
void AD9959_SetChannelDelta(AD9959_Handler* device, AD9959_Channel chan, uint32_t delta) {
    if (device == NULL) {
        return;
    }

    AD9959_SetChannels(device, chan);
    if (device->last_status != HAL_OK) {
        return;
    }

    AD9959_Write(device, CFTW, delta);
}

/**
  * @brief  Sets the phase offset of the selected channels in degrees.
  * @param  device Pointer to the AD9959 handle.
  * @param  chan Channel selection mask.
  * @param  phase_degrees Phase offset in degrees.
  */
void AD9959_SetPhase(AD9959_Handler* device, AD9959_Channel chan, int32_t phase_degrees) {
    int32_t normalized_degrees;
    uint32_t phase_word;

    if (device == NULL) {
        return;
    }

    normalized_degrees = phase_degrees % 360;

    if (normalized_degrees < 0) {
        normalized_degrees += 360;
    }

    phase_word = ((uint32_t)normalized_degrees * AD9959_PHASE_STEPS + 180U) / 360U;
    phase_word &= AD9959_PHASE_WORD_MAX;

    AD9959_SetChannels(device, chan);

    if (device->last_status != HAL_OK) {
        return;
    }

    AD9959_Write(device, CPOW, phase_word);
}

/**
  * @brief  Writes a 10-bit amplitude scale factor to selected channels.
  * @param  device Pointer to the AD9959 handle.
  * @param  chan Channel selection mask.
  * @param  amplitude Scale factor from 0 to 1023.
  */
void AD9959_SetAmplitude(AD9959_Handler* device, AD9959_Channel chan, uint16_t amplitude) {
    uint32_t transfer_acr;

    if (device == NULL) {
        return;
    }

    if (amplitude > AD9959_AMPLITUDE_MAX) {
        amplitude = AD9959_AMPLITUDE_MAX;
    }

    transfer_acr = (uint32_t)MultiplierEnable | (uint32_t)(amplitude & AD9959_AMPLITUDE_MAX);
    AD9959_SetChannels(device, chan);
    if (device->last_status != HAL_OK) {
        return;
    }

    AD9959_Write(device, ACR, transfer_acr);
}

/**
  * @brief  Configures a frequency sweep endpoint for selected channels.
  * @param  device Pointer to the AD9959 handle.
  * @param  chan Channel selection mask.
  * @param  freq Sweep endpoint frequency in hertz.
  * @param  follow Set to 1 for dwell operation or 0 for no-dwell.
  */
void AD9959_SweepFrequency(AD9959_Handler* device, AD9959_Channel chan, uint32_t freq, uint8_t follow) {
    if (device == NULL) {
        return;
    }

    AD9959_SweepDelta(device, chan, AD9959_GenerateFrequencyDelta(device, freq), follow);
}

/**
  * @brief  Configures a frequency sweep endpoint tuning word.
  * @param  device Pointer to the AD9959 handle.
  * @param  chan Channel selection mask.
  * @param  delta Sweep endpoint frequency tuning word.
  * @param  follow Set to 1 for dwell operation or 0 for no-dwell.
  */
void AD9959_SweepDelta(AD9959_Handler* device, AD9959_Channel chan, uint32_t delta, uint8_t follow) {
    uint32_t cfr_value;

    if (device == NULL) {
        return;
    }

    AD9959_SetChannels(device, chan);
    if (device->last_status != HAL_OK) {
        return;
    }

    cfr_value = (uint32_t)(FrequencyModulation | SweepEnable | DACFullScale | AutoclearSweep | AutoclearPhase | OutputSineWave);
    if (follow != 1U) {
        cfr_value |= SweepNoDwell;
    }

    if (AD9959_Write(device, CFR, cfr_value) != HAL_OK) {
        return;
    }

    if (AD9959_Write(device, CW1, delta) != HAL_OK) {
        return;
    }

    AD9959_Update(device);
}

/**
  * @brief  Configures rising and falling sweep step sizes and rates.
  * @param  device Pointer to the AD9959 handle.
  * @param  chan Channel selection mask.
  * @param  increment Rising delta word.
  * @param  up_rate Rising sweep ramp rate.
  * @param  decrement Falling delta word.
  * @param  down_rate Falling sweep ramp rate.
  */
void AD9959_SweepRates(AD9959_Handler* device, AD9959_Channel chan, uint32_t increment, uint8_t up_rate, uint32_t decrement, uint8_t down_rate) {
    if (device == NULL) {
        return;
    }

    AD9959_SetChannels(device, chan);
    if (device->last_status != HAL_OK) {
        return;
    }

    if (AD9959_Write(device, RDW, increment) != HAL_OK) {
        return;
    }

    if (AD9959_Write(device, FDW, decrement) != HAL_OK) {
        return;
    }

    if (AD9959_Write(device, LSRR, ((uint32_t)down_rate << 8U) | up_rate) != HAL_OK) {
        return;
    }

    AD9959_Update(device);
}

/**
  * @brief  Configures an amplitude sweep endpoint.
  * @param  device Pointer to the AD9959 handle.
  * @param  chan Channel selection mask.
  * @param  amplitude Endpoint amplitude word from 0 to 1023.
  * @param  follow Set to 1 for dwell operation or 0 for no-dwell.
  */
void AD9959_SweepAmplitude(AD9959_Handler* device, AD9959_Channel chan, uint16_t amplitude, uint8_t follow) {
    uint32_t cfr_value;

    if (device == NULL) {
        return;
    }

    if (amplitude > AD9959_AMPLITUDE_MAX) {
        amplitude = AD9959_AMPLITUDE_MAX;
    }

    AD9959_SetChannels(device, chan);
    if (device->last_status != HAL_OK) {
        return;
    }

    cfr_value = (uint32_t)(AmplitudeModulation | SweepEnable | DACFullScale | AutoclearSweep | AutoclearPhase | OutputSineWave);
    if (follow != 1U) {
        cfr_value |= SweepNoDwell;
    }

    if (AD9959_Write(device, CFR, cfr_value) != HAL_OK) {
        return;
    }

    if (AD9959_Write(device, CW1, (uint32_t)(amplitude & AD9959_AMPLITUDE_MAX) << 22U) != HAL_OK) {
        return;
    }

    AD9959_Update(device);
}

/**
  * @brief  Configures a phase sweep endpoint.
  * @param  device Pointer to the AD9959 handle.
  * @param  chan Channel selection mask.
  * @param  phase Endpoint phase word from 0 to 16383.
  * @param  follow Set to 1 for dwell operation or 0 for no-dwell.
  */
void AD9959_SweepPhase(AD9959_Handler* device, AD9959_Channel chan, uint16_t phase, uint8_t follow) {
    uint32_t cfr_value;

    if (device == NULL) {
        return;
    }

    AD9959_SetChannels(device, chan);
    if (device->last_status != HAL_OK) {
        return;
    }

    cfr_value = (uint32_t)(PhaseModulation | SweepEnable | DACFullScale | AutoclearSweep | AutoclearPhase | OutputSineWave);
    if (follow != 1U) {
        cfr_value |= SweepNoDwell;
    }

    if (AD9959_Write(device, CFR, cfr_value) != HAL_OK) {
        return;
    }

    if (AD9959_Write(device, CW1, (uint32_t)(phase & AD9959_PHASE_WORD_MAX) << 18U) != HAL_OK) {
        return;
    }

    AD9959_Update(device);
}

/**
  * @brief  Applies buffered register changes using I/O_UPDATE.
  * @param  device Pointer to the AD9959 handle.
  */
void AD9959_Update(AD9959_Handler* device) {
    if (device == NULL) {
        return;
    }

    HAL_GPIO_WritePin(device->update_port, device->update_pin, GPIO_PIN_SET);
    AD9959_ShortDelay();
    HAL_GPIO_WritePin(device->update_port, device->update_pin, GPIO_PIN_RESET);
    AD9959_ShortDelay();
}

/**
  * @brief  Returns the status of the most recent SPI operation.
  * @param  device Pointer to the AD9959 handle.
  * @return The most recent STM32 HAL status value.
  */
HAL_StatusTypeDef AD9959_GetLastStatus(const AD9959_Handler* device) {
    if (device == NULL) {
        return HAL_ERROR;
    }

    return device->last_status;
}

/**
  * @brief  Returns the configured AD9959 system clock frequency.
  * @param  device Pointer to the AD9959 handle.
  * @return The system clock frequency in hertz, or zero on error.
  */
uint32_t AD9959_GetCoreClockHz(const AD9959_Handler* device) {
    if ((device == NULL) || (device->core_clock > UINT32_MAX)) {
        return 0U;
    }

    return (uint32_t)device->core_clock;
}

/**
  * @brief  Writes one complete AD9959 register transaction.
  * @param  device Pointer to the AD9959 handle.
  * @param  reg Register address.
  * @param  value Register value aligned to the least significant bit.
  * @return The STM32 HAL SPI transfer status.
  */
static HAL_StatusTypeDef AD9959_Write(AD9959_Handler* device, AD9959_Register reg, uint32_t value) {
    uint8_t tx_data[5];
    uint8_t register_length;
    uint8_t index;
    HAL_StatusTypeDef status;

    if ((device == NULL) || (device->SPI_handler == NULL)) {
        return HAL_ERROR;
    }

    register_length = AD9959_GetRegisterLength(reg);
    tx_data[0] = (uint8_t)reg;
    for (index = 0U; index < register_length; index++) {
        uint8_t shift = (uint8_t)((register_length - 1U - index) * 8U);
        tx_data[index + 1U] = (uint8_t)((value >> shift) & 0xFFU);
    }

    HAL_GPIO_WritePin(device->cs_port, device->cs_pin, GPIO_PIN_RESET);
    AD9959_ShortDelay();
    status = HAL_SPI_Transmit(device->SPI_handler, tx_data, (uint16_t)(register_length + 1U), AD9959_SPI_TIMEOUT_MS);
    AD9959_ShortDelay();
    HAL_GPIO_WritePin(device->cs_port, device->cs_pin, GPIO_PIN_SET);

    device->last_status = status;
    return status;
}

/**
  * @brief  Returns the data length of an AD9959 register.
  * @param  reg Register address.
  * @return Register data length in bytes.
  */
static uint8_t AD9959_GetRegisterLength(AD9959_Register reg) {
    static const uint8_t register_length[] = {1U, 3U, 2U, 3U, 4U, 2U, 3U, 2U};
    uint8_t address = (uint8_t)reg & 0x1FU;

    if (address < (uint8_t)(sizeof(register_length) / sizeof(register_length[0]))) {
        return register_length[address];
    }

    return 4U;
}

/**
  * @brief  Generates a short GPIO and SPI timing delay.
  */
static void AD9959_ShortDelay(void) {
    volatile uint32_t count;

    for (count = 0U; count < 256U; count++) {
        __NOP();
    }
}
