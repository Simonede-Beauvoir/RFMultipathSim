/**
 * @file dds_calibration.c
 * @brief Measured 35 MHz DDS output-amplitude calibration data.
 *
 * Calibration conditions:
 * - Carrier frequency: 35 MHz
 * - Output load and oscilloscope input: 50 ohms
 * - Values selected from the automated RMS calibration results
 *
 * Recalibration is required if the DDS module, output stage, supply,
 * measurement port, cabling, or load impedance changes.
 */

#include "dds_calibration.h"

static const DDS_CalibrationPoint g_dds_calibration_35mhz[DDS_CALIBRATION_POINT_COUNT] = {
	{ 100U, 77U  },
	{ 200U, 171U },
	{ 300U, 262U },
	{ 400U, 350U },
	{ 500U, 439U },
	{ 600U, 527U },
	{ 700U, 616U },
	{ 800U, 705U },
	{ 900U, 794U },
	{ 1000U,881U }
};

bool DDS_Calibration_GetAmplitudeCode(uint32_t frequency_hz, uint16_t rms_mv, uint16_t *amplitude_code) {
	uint32_t point_index;

	if (amplitude_code == NULL) {
		return false;
	}

	if (frequency_hz != DDS_CALIBRATION_FREQUENCY_HZ) {
		return false;
	}

	if ((rms_mv < DDS_CALIBRATION_MIN_RMS_MV) || (rms_mv > DDS_CALIBRATION_MAX_RMS_MV)) {
		return false;
	}

	if ((rms_mv % DDS_CALIBRATION_RMS_STEP_MV) != 0U) {
		return false;
	}

	point_index = ((uint32_t)rms_mv / DDS_CALIBRATION_RMS_STEP_MV) - 1U;
	if (point_index >= DDS_CALIBRATION_POINT_COUNT) {
		return false;
	}

	*amplitude_code = g_dds_calibration_35mhz[point_index].amplitude_code;
	return true;
}

const DDS_CalibrationPoint *DDS_Calibration_GetTable(size_t *point_count) {
	if (point_count != NULL) {
		*point_count = DDS_CALIBRATION_POINT_COUNT;
	}

	return g_dds_calibration_35mhz;
}
