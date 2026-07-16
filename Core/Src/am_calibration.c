/**
 * @file am_calibration.c
 * @brief Measured 35 MHz carrier and 2 MHz AM calibration data.
 *
 * Calibration conditions:
 * - Carrier frequency: 35 MHz
 * - Modulation frequency: 2 MHz
 * - CH0 carrier amplitude code: 881
 * - Output load and oscilloscope input: 50 ohms
 * - Modulation depth calculated from the mean of both FFT sidebands
 *
 * The table must be verified at other carrier amplitude codes before it is
 * treated as independent of carrier amplitude.
 */

#include "am_calibration.h"

static const AM_CalibrationPoint g_am_calibration_35mhz[AM_CALIBRATION_POINT_COUNT] = {
    { 30U, 163U },
    { 40U, 217U },
    { 50U, 271U },
    { 60U, 326U },
    { 70U, 380U },
    { 80U, 434U },
    { 90U, 488U }
};

bool AM_Calibration_GetAmplitudeCode(uint32_t carrier_frequency_hz, uint32_t modulation_frequency_hz, uint8_t modulation_percent,
                                     uint16_t* amplitude_code) {
    uint32_t point_index;

    if (amplitude_code == NULL) {
        return false;
    }

    if ((carrier_frequency_hz != AM_CALIBRATION_FREQUENCY_HZ) ||
        (modulation_frequency_hz != AM_CALIBRATION_MODULATION_FREQUENCY_HZ)) {
        return false;
    }

    if ((modulation_percent < AM_CALIBRATION_MIN_PERCENT) || (modulation_percent > AM_CALIBRATION_MAX_PERCENT)) {
        return false;
    }

    if ((modulation_percent % AM_CALIBRATION_PERCENT_STEP) != 0U) {
        return false;
    }

    point_index = ((uint32_t)(modulation_percent - AM_CALIBRATION_MIN_PERCENT)) / AM_CALIBRATION_PERCENT_STEP;
    if (point_index >= AM_CALIBRATION_POINT_COUNT) {
        return false;
    }

    *amplitude_code = g_am_calibration_35mhz[point_index].amplitude_code;
    return true;
}

const AM_CalibrationPoint* AM_Calibration_GetTable(size_t* point_count) {
    if (point_count != NULL) {
        *point_count = AM_CALIBRATION_POINT_COUNT;
    }

    return g_am_calibration_35mhz;
}
