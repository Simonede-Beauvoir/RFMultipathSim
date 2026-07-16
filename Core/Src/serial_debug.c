#include "serial_debug.h"

#include "am_calibration.h"
#include "dds_calibration.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SERIAL_DEBUG_COMMAND_BUFFER_SIZE          96U
#define SERIAL_DEBUG_RX_TIMEOUT_MS                1U
#define SERIAL_DEBUG_TX_TIMEOUT_MS                100U
#define SERIAL_DEBUG_RESET_DELAY_MS               20U
#define SERIAL_DEBUG_AM_MODULATION_FREQUENCY_HZ   2000000U

typedef enum {
    SERIAL_DEBUG_MODE_OFF,
    SERIAL_DEBUG_MODE_CW,
    SERIAL_DEBUG_MODE_AM
} SerialDebug_Mode;

static UART_HandleTypeDef* g_serial_uart;
static AD9959_Handler* g_serial_ad9959;
static char g_command_buffer[SERIAL_DEBUG_COMMAND_BUFFER_SIZE];
static size_t g_command_length;
static uint32_t g_frequency_hz;
static uint16_t g_amplitude_code;
static int32_t g_phase_degree;
static uint16_t g_modulation_amplitude_code;
static uint16_t g_carrier_rms_mv;
static uint8_t g_modulation_percent;
static bool g_am_uses_calibration;
static SerialDebug_Mode g_mode;

static bool SerialDebug_ApplyCW(uint32_t frequency_hz, uint16_t amplitude_code, int32_t phase_degree);
static bool SerialDebug_ApplyAM(uint32_t carrier_frequency_hz, uint16_t carrier_amplitude_code, uint16_t modulation_amplitude_code);
static bool SerialDebug_MuteAll(void);
static void SerialDebug_PrintDDSStatus(void);
static void SerialDebug_PrintAMStatus(void);
static void SerialDebug_ExecuteCommand(const char* command);

/**
  * @brief  Redirects printf output to the configured debug USART.
  * @param  ch Character to transmit.
  * @retval Transmitted character, or EOF when output is unavailable.
  */
int __io_putchar(int ch) {
    uint8_t data = (uint8_t)ch;

    if ((g_serial_uart == NULL) || (HAL_UART_Transmit(g_serial_uart, &data, 1U, SERIAL_DEBUG_TX_TIMEOUT_MS) != HAL_OK)) {
        return EOF;
    }

    return ch;
}

/**
  * @brief  Applies one CW configuration and mutes the modulation channel.
  * @param  frequency_hz Carrier frequency in hertz.
  * @param  amplitude_code CH0 amplitude code from 0 to 1023.
  * @param  phase_degree CH0 phase in degrees.
  * @retval true All AD9959 transfers completed successfully.
  * @retval false One or more AD9959 transfers failed.
  */
static bool SerialDebug_ApplyCW(uint32_t frequency_hz, uint16_t amplitude_code, int32_t phase_degree) {
    AD9959_SetAmplitude(g_serial_ad9959, Channel_1, 0U);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_SetChannelFrequency(g_serial_ad9959, Channel_0, frequency_hz);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_SetPhase(g_serial_ad9959, Channel_0, phase_degree);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_SetAmplitude(g_serial_ad9959, Channel_0, amplitude_code);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_Update(g_serial_ad9959);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    g_frequency_hz = frequency_hz;
    g_amplitude_code = amplitude_code;
    g_phase_degree = phase_degree;
    g_modulation_amplitude_code = 0U;
    g_carrier_rms_mv = 0U;
    g_modulation_percent = 0U;
    g_am_uses_calibration = false;
    g_mode = SERIAL_DEBUG_MODE_CW;
    return true;
}

/**
  * @brief  Applies a two-channel AM test configuration.
  * @param  carrier_frequency_hz CH0 carrier frequency in hertz.
  * @param  carrier_amplitude_code CH0 amplitude code from 0 to 1023.
  * @param  modulation_amplitude_code CH1 2 MHz amplitude code from 0 to 1023.
  * @retval true All AD9959 transfers completed successfully.
  * @retval false One or more AD9959 transfers failed.
  */
static bool SerialDebug_ApplyAM(uint32_t carrier_frequency_hz, uint16_t carrier_amplitude_code, uint16_t modulation_amplitude_code) {
    AD9959_SetChannelFrequency(g_serial_ad9959, Channel_0, carrier_frequency_hz);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_SetPhase(g_serial_ad9959, Channel_0, 0);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_SetAmplitude(g_serial_ad9959, Channel_0, carrier_amplitude_code);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_SetChannelFrequency(g_serial_ad9959, Channel_1, SERIAL_DEBUG_AM_MODULATION_FREQUENCY_HZ);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_SetPhase(g_serial_ad9959, Channel_1, 0);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_SetAmplitude(g_serial_ad9959, Channel_1, modulation_amplitude_code);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_Update(g_serial_ad9959);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    g_frequency_hz = carrier_frequency_hz;
    g_amplitude_code = carrier_amplitude_code;
    g_phase_degree = 0;
    g_modulation_amplitude_code = modulation_amplitude_code;
    g_carrier_rms_mv = 0U;
    g_modulation_percent = 0U;
    g_am_uses_calibration = false;
    g_mode = SERIAL_DEBUG_MODE_AM;
    return true;
}

/**
  * @brief  Mutes all AD9959 channels.
  * @retval true The mute command completed successfully.
  * @retval false The AD9959 transfer failed.
  */
static bool SerialDebug_MuteAll(void) {
    AD9959_SetAmplitude(g_serial_ad9959, Channel_All, 0U);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    AD9959_Update(g_serial_ad9959);
    if (AD9959_GetLastStatus(g_serial_ad9959) != HAL_OK) {
        return false;
    }

    g_amplitude_code = 0U;
    g_modulation_amplitude_code = 0U;
    g_carrier_rms_mv = 0U;
    g_modulation_percent = 0U;
    g_am_uses_calibration = false;
    g_mode = SERIAL_DEBUG_MODE_OFF;
    return true;
}

/**
  * @brief  Prints the current CW settings.
  */
static void SerialDebug_PrintDDSStatus(void) {
    printf("OK DDS FREQ=%lu AMP=%u PHASE=%ld\r\n", (unsigned long)g_frequency_hz, (unsigned int)g_amplitude_code, (long)g_phase_degree);
}

/**
  * @brief  Prints the current AM test settings.
  */
static void SerialDebug_PrintAMStatus(void) {
    if (g_am_uses_calibration) {
        printf("OK AM FC=%lu CARRIER_RMS=%u CARRIER_AMP=%u FM=%lu MOD=%u MOD_AMP=%u\r\n", (unsigned long)g_frequency_hz,
               (unsigned int)g_carrier_rms_mv, (unsigned int)g_amplitude_code, (unsigned long)SERIAL_DEBUG_AM_MODULATION_FREQUENCY_HZ,
               (unsigned int)g_modulation_percent, (unsigned int)g_modulation_amplitude_code);
        return;
    }

    printf("OK AM FC=%lu CARRIER_AMP=%u FM=%lu MOD_AMP=%u\r\n", (unsigned long)g_frequency_hz, (unsigned int)g_amplitude_code,
           (unsigned long)SERIAL_DEBUG_AM_MODULATION_FREQUENCY_HZ, (unsigned int)g_modulation_amplitude_code);
}

/**
  * @brief  Parses and executes one complete command line.
  * @param  command Null-terminated command without CR or LF.
  */
static void SerialDebug_ExecuteCommand(const char* command) {
    unsigned long frequency_hz;
    unsigned long amplitude_code;
    unsigned long modulation_amplitude_code;
    unsigned long carrier_rms_mv;
    unsigned long modulation_percent;
    long phase_degree;
    char trailing;

    if (sscanf(command, "DDS SET %lu %lu %ld %c", &frequency_hz, &amplitude_code, &phase_degree, &trailing) == 3) {
        if ((frequency_hz > (unsigned long)(AD9959_GetCoreClockHz(g_serial_ad9959) / 2U)) || (amplitude_code > AD9959_AMPLITUDE_MAX)) {
            printf("ERROR RANGE FREQ=0..SYSCLK/2 AMP=0..1023 PHASE=INT32\r\n");
            return;
        }

        if (!SerialDebug_ApplyCW((uint32_t)frequency_hz, (uint16_t)amplitude_code, (int32_t)phase_degree)) {
            printf("ERROR DDS SPI\r\n");
            return;
        }

        SerialDebug_PrintDDSStatus();
        return;
    }

    if (sscanf(command, "AM SET CAL %lu %lu %lu %c", &frequency_hz, &carrier_rms_mv, &modulation_percent, &trailing) == 3) {
        uint16_t carrier_amplitude_code;
        uint16_t calibrated_modulation_code;

        if ((carrier_rms_mv > UINT16_MAX) || (modulation_percent > UINT8_MAX) ||
            !DDS_Calibration_GetAmplitudeCode((uint32_t)frequency_hz, (uint16_t)carrier_rms_mv, &carrier_amplitude_code) ||
            !AM_Calibration_GetAmplitudeCode((uint32_t)frequency_hz, SERIAL_DEBUG_AM_MODULATION_FREQUENCY_HZ,
                                             (uint8_t)modulation_percent, &calibrated_modulation_code)) {
            printf("ERROR CAL RANGE FC=35000000 CARRIER_RMS=100..1000/100 MOD=30..90/10\r\n");
            return;
        }

        if (!SerialDebug_ApplyAM((uint32_t)frequency_hz, carrier_amplitude_code, calibrated_modulation_code)) {
            printf("ERROR DDS SPI\r\n");
            return;
        }

        g_carrier_rms_mv = (uint16_t)carrier_rms_mv;
        g_modulation_percent = (uint8_t)modulation_percent;
        g_am_uses_calibration = true;
        SerialDebug_PrintAMStatus();
        return;
    }

    if (sscanf(command, "AM SET %lu %lu %lu %c", &frequency_hz, &amplitude_code, &modulation_amplitude_code, &trailing) == 3) {
        if ((frequency_hz > (unsigned long)(AD9959_GetCoreClockHz(g_serial_ad9959) / 2U)) || (amplitude_code > AD9959_AMPLITUDE_MAX) ||
            (modulation_amplitude_code > AD9959_AMPLITUDE_MAX)) {
            printf("ERROR RANGE FC=0..SYSCLK/2 CARRIER_AMP=0..1023 MOD_AMP=0..1023\r\n");
            return;
        }

        if (!SerialDebug_ApplyAM((uint32_t)frequency_hz, (uint16_t)amplitude_code, (uint16_t)modulation_amplitude_code)) {
            printf("ERROR DDS SPI\r\n");
            return;
        }

        SerialDebug_PrintAMStatus();
        return;
    }

    if ((strcmp(command, "DDS OFF") == 0) || (strcmp(command, "AM OFF") == 0)) {
        if (!SerialDebug_MuteAll()) {
            printf("ERROR DDS SPI\r\n");
            return;
        }

        printf("OK DDS OFF\r\n");
        return;
    }

    if (strcmp(command, "DDS STATUS") == 0) {
        SerialDebug_PrintDDSStatus();
        return;
    }

    if (strcmp(command, "AM STATUS") == 0) {
        SerialDebug_PrintAMStatus();
        return;
    }

    if (strcmp(command, "STATUS") == 0) {
        if (g_mode == SERIAL_DEBUG_MODE_AM) {
            SerialDebug_PrintAMStatus();
        }
        else if (g_mode == SERIAL_DEBUG_MODE_CW) {
            SerialDebug_PrintDDSStatus();
        }
        else {
            printf("OK DDS OFF\r\n");
        }
        return;
    }

    if (strcmp(command, "RESET") == 0) {
        printf("OK RESET\r\n");
        HAL_Delay(SERIAL_DEBUG_RESET_DELAY_MS);
        NVIC_SystemReset();
        return;
    }

    printf("ERROR COMMAND: [%s]\r\n", command);
}

/**
  * @brief  Initializes USART command processing and printf redirection.
  * @param  uart USART handle used for command input and printf output.
  * @param  ad9959 AD9959 device controlled by serial commands.
  * @param  frequency_hz Initial CH0 frequency in hertz.
  * @param  amplitude_code Initial CH0 amplitude code from 0 to 1023.
  * @param  phase_degree Initial CH0 phase in degrees.
  */
void SerialDebug_Init(UART_HandleTypeDef* uart, AD9959_Handler* ad9959, uint32_t frequency_hz, uint16_t amplitude_code, int32_t phase_degree) {
    g_serial_uart = uart;
    g_serial_ad9959 = ad9959;
    g_command_length = 0U;
    g_frequency_hz = frequency_hz;
    g_amplitude_code = amplitude_code;
    g_phase_degree = phase_degree;
    g_modulation_amplitude_code = 0U;
    g_carrier_rms_mv = 0U;
    g_modulation_percent = 0U;
    g_am_uses_calibration = false;
    g_mode = SERIAL_DEBUG_MODE_CW;

    printf("DDS ready. Commands: DDS SET <frequency_hz> <amplitude_code> <phase_degree>, AM SET <carrier_hz> <carrier_amp_code> <mod_amp_code>, AM SET CAL <carrier_hz> <carrier_rms_mv> <mod_percent>, STATUS, DDS OFF, RESET\r\n");
}

/**
  * @brief  Polls the debug USART and executes complete command lines.
  */
void SerialDebug_Task(void) {
    uint8_t received_byte;

    if ((g_serial_uart == NULL) || (HAL_UART_Receive(g_serial_uart, &received_byte, 1U, SERIAL_DEBUG_RX_TIMEOUT_MS) != HAL_OK)) {
        return;
    }

    if ((received_byte == '\r') || (received_byte == '\n')) {
        if (g_command_length != 0U) {
            g_command_buffer[g_command_length] = '\0';
            SerialDebug_ExecuteCommand(g_command_buffer);
            g_command_length = 0U;
        }
    }
    else if (g_command_length < (SERIAL_DEBUG_COMMAND_BUFFER_SIZE - 1U)) {
        g_command_buffer[g_command_length++] = (char)received_byte;
    }
    else {
        g_command_length = 0U;
        printf("ERROR COMMAND TOO LONG\r\n");
    }
}
