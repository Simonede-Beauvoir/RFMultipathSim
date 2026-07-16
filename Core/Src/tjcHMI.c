#include "tjcHMI.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define TJC_TX_BUFFER_LEN 128U
#define TJC_TX_TIMEOUT_MS 100U

char tjc_rx_str[TJC_RX_MAX_LEN];
volatile uint8_t tjc_strFlag;
volatile int32_t tjc_rxNum;
volatile uint8_t tjc_numFlag;

static UART_HandleTypeDef *g_tjc_uart;
static uint8_t g_tjc_rx_buffer[TJC_RX_MAX_LEN];
static uint8_t g_tjc_rx_index;

static HAL_StatusTypeDef TJC_HMI_ReceiveValue(int32_t *value, uint32_t timeout_ms)
{
    uint8_t frame[8];
    uint8_t frame_index = 0U;
    uint8_t byte;
    uint32_t start_tick;
    uint32_t elapsed;
    uint32_t remaining;
    HAL_StatusTypeDef status;

    if ((g_tjc_uart == NULL) || (value == NULL) || (timeout_ms == 0U)) {
        return HAL_ERROR;
    }

    start_tick = HAL_GetTick();
    while (1) {
        elapsed = HAL_GetTick() - start_tick;
        if (elapsed >= timeout_ms) {
            return HAL_TIMEOUT;
        }
        remaining = timeout_ms - elapsed;

        status = HAL_UART_Receive(g_tjc_uart, &byte, 1U, remaining);
        if (status != HAL_OK) {
            return status;
        }

        if (frame_index == 0U) {
            if (byte == 0x71U) {
                frame[frame_index++] = byte;
            }
            continue;
        }

        frame[frame_index++] = byte;
        if (frame_index < sizeof(frame)) {
            continue;
        }

        if ((frame[5] == 0xFFU) && (frame[6] == 0xFFU) &&
            (frame[7] == 0xFFU)) {
            uint32_t raw_value = ((uint32_t)frame[1]) |
                                 ((uint32_t)frame[2] << 8U) |
                                 ((uint32_t)frame[3] << 16U) |
                                 ((uint32_t)frame[4] << 24U);
            *value = (int32_t)raw_value;
            return HAL_OK;
        }

        frame_index = (byte == 0x71U) ? 1U : 0U;
        if (frame_index == 1U) {
            frame[0] = byte;
        }
    }
}

HAL_StatusTypeDef tjcHMI_Init(UART_HandleTypeDef *uart)
{
    if ((uart == NULL) || (uart->Instance != USART2)) {
        g_tjc_uart = NULL;
        return HAL_ERROR;
    }

    g_tjc_uart = uart;
    g_tjc_rx_index = 0U;
    tjc_strFlag = 0U;
    tjc_rxNum = 0;
    tjc_numFlag = 0U;
    memset(tjc_rx_str, 0, sizeof(tjc_rx_str));
    memset(g_tjc_rx_buffer, 0, sizeof(g_tjc_rx_buffer));
    return HAL_OK;
}

void tjc_FeedRxByte(uint8_t rx_data)
{
    uint8_t frame_len;
    uint8_t string_len;

    g_tjc_rx_buffer[g_tjc_rx_index++] = rx_data;

    if ((g_tjc_rx_index >= 3U) &&
        (g_tjc_rx_buffer[g_tjc_rx_index - 1U] == 0xFFU) &&
        (g_tjc_rx_buffer[g_tjc_rx_index - 2U] == 0xFFU) &&
        (g_tjc_rx_buffer[g_tjc_rx_index - 3U] == 0xFFU)) {
        frame_len = g_tjc_rx_index;

        if (g_tjc_rx_buffer[0] == 0x70U) {
            string_len = (uint8_t)(frame_len - 4U);
            if (string_len >= TJC_RX_MAX_LEN) {
                string_len = TJC_RX_MAX_LEN - 1U;
            }
            memcpy(tjc_rx_str, &g_tjc_rx_buffer[1], string_len);
            tjc_rx_str[string_len] = '\0';
            tjc_strFlag = 1U;
        } else if ((g_tjc_rx_buffer[0] == 0x71U) && (frame_len == 8U)) {
            uint32_t value = ((uint32_t)g_tjc_rx_buffer[1]) |
                             ((uint32_t)g_tjc_rx_buffer[2] << 8U) |
                             ((uint32_t)g_tjc_rx_buffer[3] << 16U) |
                             ((uint32_t)g_tjc_rx_buffer[4] << 24U);
            tjc_rxNum = (int32_t)value;
            tjc_numFlag = 1U;
        }

        g_tjc_rx_index = 0U;
    } else if (g_tjc_rx_index >= TJC_RX_MAX_LEN) {
        g_tjc_rx_index = 0U;
    }
}

HAL_StatusTypeDef tjcPrintf(const char *format, ...)
{
    static const uint8_t terminator[3] = {0xFFU, 0xFFU, 0xFFU};
    char buffer[TJC_TX_BUFFER_LEN];
    va_list args;
    int length;
    HAL_StatusTypeDef status;

    if ((g_tjc_uart == NULL) || (format == NULL)) {
        return HAL_ERROR;
    }

    va_start(args, format);
    length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (length < 0) {
        return HAL_ERROR;
    }
    if ((size_t)length >= sizeof(buffer)) {
        length = (int)(sizeof(buffer) - 1U);
    }

    status = HAL_UART_Transmit(g_tjc_uart, (uint8_t *)buffer,
                               (uint16_t)length, TJC_TX_TIMEOUT_MS);
    if (status != HAL_OK) {
        return status;
    }

    return HAL_UART_Transmit(g_tjc_uart, (uint8_t *)terminator,
                             sizeof(terminator), TJC_TX_TIMEOUT_MS);
}

HAL_StatusTypeDef tjcSetText(const char *obj_name, const char *text)
{
    if ((obj_name == NULL) || (text == NULL)) {
        return HAL_ERROR;
    }
    return tjcPrintf("%s.txt=\"%s\"", obj_name, text);
}

HAL_StatusTypeDef tjcSetValue(const char *obj_name, int32_t value)
{
    if (obj_name == NULL) {
        return HAL_ERROR;
    }
    return tjcPrintf("%s.val=%ld", obj_name, (long)value);
}

HAL_StatusTypeDef tjcChangePage(const char *page_name)
{
    if (page_name == NULL) {
        return HAL_ERROR;
    }
    return tjcPrintf("page %s", page_name);
}

HAL_StatusTypeDef tjcClick(const char *obj_name, uint8_t event_type)
{
    if ((obj_name == NULL) || (event_type > 1U)) {
        return HAL_ERROR;
    }
    return tjcPrintf("click %s,%u", obj_name, (unsigned int)event_type);
}

HAL_StatusTypeDef tjcGetValue(const char *obj_name, int32_t *value, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;

    if ((obj_name == NULL) || (value == NULL)) {
        return HAL_ERROR;
    }

    status = tjcPrintf("get %s.val", obj_name);
    if (status != HAL_OK) {
        return status;
    }

    return TJC_HMI_ReceiveValue(value, timeout_ms);
}

HAL_StatusTypeDef tjcReadSettings(TJC_HMI_Settings *settings, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;

    if (settings == NULL) {
        return HAL_ERROR;
    }

    status = tjcGetValue("sw0", &settings->mode, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }
    status = tjcGetValue("n4", &settings->carrier_frequency_mhz, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }
    status = tjcGetValue("n0", &settings->carrier_amplitude_mv, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }
    status = tjcGetValue("n5", &settings->initial_phase_degree, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }
    status = tjcGetValue("n2", &settings->delay_ns, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }
    status = tjcGetValue("n3", &settings->attenuation_db, timeout_ms);
    if (status != HAL_OK) {
        return status;
    }
    return tjcGetValue("n1", &settings->modulation_percent, timeout_ms);
}
