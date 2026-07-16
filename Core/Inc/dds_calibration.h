/**
 * @file dds_calibration.h
 * @brief DDS output-amplitude calibration lookup interface.
 */

#ifndef DDS_CALIBRATION_H
#define DDS_CALIBRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DDS_CALIBRATION_FREQUENCY_HZ       35000000UL
#define DDS_CALIBRATION_MIN_RMS_MV         100U
#define DDS_CALIBRATION_MAX_RMS_MV         1000U
#define DDS_CALIBRATION_RMS_STEP_MV        100U
#define DDS_CALIBRATION_POINT_COUNT        10U

/**
 * @brief One measured DDS amplitude-calibration point.
 */
typedef struct {
    uint16_t rms_mv;
    uint16_t amplitude_code;
} DDS_CalibrationPoint;

/**
 * @brief Get the amplitude code for an exactly calibrated operating point.
 * @param frequency_hz DDS output frequency in hertz.
 * @param rms_mv Requested carrier RMS voltage in millivolts.
 * @param amplitude_code Destination for the AD9959 amplitude code.
 * @retval true The requested point was found.
 * @retval false A parameter was invalid or the point was not calibrated.
 */
bool DDS_Calibration_GetAmplitudeCode(uint32_t frequency_hz, uint16_t rms_mv, uint16_t* amplitude_code);

/**
 * @brief Get the read-only 35 MHz calibration table.
 * @param point_count Destination for the number of calibration points; may be NULL.
 * @retval Pointer to the first calibration point.
 */
const DDS_CalibrationPoint* DDS_Calibration_GetTable(size_t* point_count);

#ifdef __cplusplus
}
#endif

#endif /* DDS_CALIBRATION_H */
