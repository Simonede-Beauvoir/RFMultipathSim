/**
 * @file am_calibration.h
 * @brief AM modulation-depth calibration lookup interface.
 */

#ifndef AM_CALIBRATION_H
#define AM_CALIBRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AM_CALIBRATION_FREQUENCY_HZ             35000000UL
#define AM_CALIBRATION_MODULATION_FREQUENCY_HZ  2000000UL
#define AM_CALIBRATION_CARRIER_CODE              881U
#define AM_CALIBRATION_MIN_PERCENT               30U
#define AM_CALIBRATION_MAX_PERCENT               90U
#define AM_CALIBRATION_PERCENT_STEP              10U
#define AM_CALIBRATION_POINT_COUNT               7U

/**
 * @brief One measured AM modulation-depth calibration point.
 */
typedef struct {
    uint8_t modulation_percent;
    uint16_t amplitude_code;
} AM_CalibrationPoint;

/**
 * @brief Get the CH1 amplitude code for a calibrated modulation depth.
 * @param carrier_frequency_hz Carrier frequency in hertz.
 * @param modulation_frequency_hz Modulation frequency in hertz.
 * @param modulation_percent Requested AM depth in percent.
 * @param amplitude_code Destination for the CH1 amplitude code.
 * @retval true The requested point was found.
 * @retval false A parameter was invalid or the point was not calibrated.
 */
bool AM_Calibration_GetAmplitudeCode(uint32_t carrier_frequency_hz, uint32_t modulation_frequency_hz, uint8_t modulation_percent,
                                     uint16_t* amplitude_code);

/**
 * @brief Get the read-only 35 MHz AM calibration table.
 * @param point_count Destination for the number of points; may be NULL.
 * @retval Pointer to the first calibration point.
 */
const AM_CalibrationPoint* AM_Calibration_GetTable(size_t* point_count);

#ifdef __cplusplus
}
#endif

#endif /* AM_CALIBRATION_H */
