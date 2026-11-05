/*
 * hts221.c
 *
 * Driver for HTS221 - relative humidity and temperature sensor.
 */

#include "hts221.h"

/* Registered I2C functions */
static HTS221_ReadCallback hts221_read = 0;
static HTS221_WriteCallback hts221_write = 0;

/* Calibration coefficients (read from sensor during init) */
static float h0_rh, h1_rh;			//humidity calibration points [% rH]
static int16_t h0_t0_out, h1_t0_out;	//raw humidity values in calibration points
static float t0_degc, t1_degc;		//temperature calibration points [degC]
static int16_t t0_out, t1_out;		//raw temperature values in calibration points


void HTS221_RegisterCallbacks(HTS221_ReadCallback read_callback, HTS221_WriteCallback write_callback)
{
	if(read_callback != 0)
	{
		hts221_read = read_callback;
	}
	if(write_callback != 0)
	{
		hts221_write = write_callback;
	}
}

uint8_t HTS221_ReadRegisters(uint8_t reg_address, uint8_t *data, uint8_t length)
{
	if(hts221_read == 0) return HTS221_ERROR;

	/* multi-byte read needs auto increment of register address */
	if(length > 1U)
	{
		reg_address |= HTS221_AUTO_INCREMENT;
	}
	return (hts221_read(HTS221_DEVICE_ADDRESS, reg_address, data, length) == 0U) ? HTS221_OK : HTS221_ERROR;
}

uint8_t HTS221_WriteRegisters(uint8_t reg_address, uint8_t *data, uint8_t length)
{
	if(hts221_write == 0) return HTS221_ERROR;

	if(length > 1U)
	{
		reg_address |= HTS221_AUTO_INCREMENT;
	}
	return (hts221_write(HTS221_DEVICE_ADDRESS, reg_address, data, length) == 0U) ? HTS221_OK : HTS221_ERROR;
}

/* Reads all calibration registers at once (one multi-byte read) and computes coefficients */
static uint8_t HTS221_ReadCalibration(void)
{
	uint8_t c[HTS221_CALIB_LENGTH];

	if(HTS221_ReadRegisters(HTS221_CALIB_START, c, HTS221_CALIB_LENGTH) != HTS221_OK)
	{
		return HTS221_ERROR;
	}

	/* index in array = register address - 0x30 */
	h0_rh = c[HTS221_H0_RH_X2 - HTS221_CALIB_START] / 2.0f;
	h1_rh = c[HTS221_H1_RH_X2 - HTS221_CALIB_START] / 2.0f;

	/* T0 and T1 are 10-bit values, 2 MSBs of each are in register T1_T0_MSB */
	uint8_t msb = c[HTS221_T1_T0_MSB - HTS221_CALIB_START];
	t0_degc = (float)(((uint16_t)(msb & 0x03U) << 8) | c[HTS221_T0_DEGC_X8 - HTS221_CALIB_START]) / 8.0f;
	t1_degc = (float)(((uint16_t)(msb & 0x0CU) << 6) | c[HTS221_T1_DEGC_X8 - HTS221_CALIB_START]) / 8.0f;

	h0_t0_out = (int16_t)(((uint16_t)c[HTS221_H0_T0_OUT_L - HTS221_CALIB_START + 1] << 8) | c[HTS221_H0_T0_OUT_L - HTS221_CALIB_START]);
	h1_t0_out = (int16_t)(((uint16_t)c[HTS221_H1_T0_OUT_L - HTS221_CALIB_START + 1] << 8) | c[HTS221_H1_T0_OUT_L - HTS221_CALIB_START]);
	t0_out    = (int16_t)(((uint16_t)c[HTS221_T0_OUT_L - HTS221_CALIB_START + 1] << 8) | c[HTS221_T0_OUT_L - HTS221_CALIB_START]);
	t1_out    = (int16_t)(((uint16_t)c[HTS221_T1_OUT_L - HTS221_CALIB_START + 1] << 8) | c[HTS221_T1_OUT_L - HTS221_CALIB_START]);

	/* protection against division by zero */
	if(h1_t0_out == h0_t0_out || t1_out == t0_out)
	{
		return HTS221_ERROR;
	}
	return HTS221_OK;
}

uint8_t HTS221_Init(void)
{
	uint8_t value;

	/* 1. check connection - WHO_AM_I */
	if(HTS221_ReadRegisters(HTS221_WHO_AM_I, &value, 1U) != HTS221_OK || value != HTS221_WHO_AM_I_VALUE)
	{
		return HTS221_ERROR;
	}

	/* 2. configuration */
	value = HTS221_AV_CONF_VALUE;			//averaging: 16 temperature, 32 humidity samples
	if(HTS221_WriteRegisters(HTS221_AV_CONF, &value, 1U) != HTS221_OK) return HTS221_ERROR;

	value = HTS221_CTRL_REG1_PD | HTS221_CTRL_REG1_BDU | HTS221_CTRL_REG1_ODR_12HZ;	//power on, BDU, 12.5 Hz
	if(HTS221_WriteRegisters(HTS221_CTRL_REG1, &value, 1U) != HTS221_OK) return HTS221_ERROR;

	/* check written configuration */
	if(HTS221_ReadRegisters(HTS221_CTRL_REG1, &value, 1U) != HTS221_OK ||
	   value != (HTS221_CTRL_REG1_PD | HTS221_CTRL_REG1_BDU | HTS221_CTRL_REG1_ODR_12HZ))
	{
		return HTS221_ERROR;
	}

	/* 3. calibration coefficients */
	return HTS221_ReadCalibration();
}

uint8_t HTS221_ReadTemperature(float *temperature)
{
	uint8_t data[2];
	int16_t t_out;

	/* TEMP_OUT_L and TEMP_OUT_H in one multi-byte read */
	if(HTS221_ReadRegisters(HTS221_TEMP_OUT_L, data, 2U) != HTS221_OK)
	{
		return HTS221_ERROR;
	}
	t_out = (int16_t)(((uint16_t)data[1] << 8) | data[0]);

	/* linear interpolation between calibration points */
	*temperature = t0_degc + (float)(t_out - t0_out) * (t1_degc - t0_degc) / (float)(t1_out - t0_out);

	return HTS221_OK;
}

uint8_t HTS221_ReadHumidity(float *humidity)
{
	uint8_t data[2];
	int16_t h_out;
	float h;

	/* HUMIDITY_OUT_L and HUMIDITY_OUT_H in one multi-byte read */
	if(HTS221_ReadRegisters(HTS221_HUMIDITY_OUT_L, data, 2U) != HTS221_OK)
	{
		return HTS221_ERROR;
	}
	h_out = (int16_t)(((uint16_t)data[1] << 8) | data[0]);

	/* linear interpolation between calibration points */
	h = h0_rh + (float)(h_out - h0_t0_out) * (h1_rh - h0_rh) / (float)(h1_t0_out - h0_t0_out);

	/* relative humidity must be in range 0 - 100 % */
	if(h < 0.0f)   h = 0.0f;
	if(h > 100.0f) h = 100.0f;

	*humidity = h;
	return HTS221_OK;
}
