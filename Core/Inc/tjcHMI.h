#ifndef TJC_HMI_H
#define TJC_HMI_H

#include "stm32h7xx_hal.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TJC_RX_MAX_LEN 64U

typedef struct {
    int32_t mode;
    int32_t carrier_frequency_mhz;
    int32_t carrier_amplitude_mv;
    int32_t initial_phase_degree;
    int32_t delay_ns;
    int32_t attenuation_db;
    int32_t modulation_percent;
} TJC_HMI_Settings;

extern char tjc_rx_str[TJC_RX_MAX_LEN];
extern volatile uint8_t tjc_strFlag;
extern volatile int32_t tjc_rxNum;
extern volatile uint8_t tjc_numFlag;

/** Initializes the display driver without transmitting data. */
HAL_StatusTypeDef tjcHMI_Init(UART_HandleTypeDef *uart);

/** Feeds one received byte into the display response parser. */
void tjc_FeedRxByte(uint8_t rx_data);

/** Sends a formatted TJC command followed by three 0xFF bytes. */
HAL_StatusTypeDef tjcPrintf(const char *format, ...);

/** Sets the text property of a display object. */
HAL_StatusTypeDef tjcSetText(const char *obj_name, const char *text);

/** Sets the numeric value property of a display object. */
HAL_StatusTypeDef tjcSetValue(const char *obj_name, int32_t value);

/** Changes the current display page. */
HAL_StatusTypeDef tjcChangePage(const char *page_name);

/** Generates a press or release event for a display object. */
HAL_StatusTypeDef tjcClick(const char *obj_name, uint8_t event_type);

/**
 * @brief Reads the numeric val property of a display object.
 * @param obj_name Display object name without the .val suffix.
 * @param value Destination for the signed 32-bit value.
 * @param timeout_ms Maximum time to wait for the response.
 * @retval HAL_OK Value received successfully.
 * @retval HAL_ERROR Invalid argument or malformed response.
 * @retval HAL_TIMEOUT No valid response before the timeout.
 */
HAL_StatusTypeDef tjcGetValue(const char *obj_name, int32_t *value, uint32_t timeout_ms);

/**
 * @brief Reads all signal settings from the display controls.
 * @param settings Destination for the complete display configuration.
 * @param timeout_ms Maximum response time for each control.
 * @retval HAL_OK All controls were read successfully.
 * @retval HAL_ERROR Invalid argument or malformed response.
 * @retval HAL_TIMEOUT One control did not respond before the timeout.
 */
HAL_StatusTypeDef tjcReadSettings(TJC_HMI_Settings *settings, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
