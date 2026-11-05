/*
 * lps25hb.c
 *
 * Driver for LPS25HB - pressure sensor.
 */

#include "lps25hb.h"
#include <math.h>

/* Registered I2C functions */
static LPS25HB_ReadCallback lps25hb_read = 0;
static LPS25HB_WriteCallback lps25hb_write = 0;

/* I2C address found during init */
static uint8_t lps25hb_address = LPS25HB_DEVICE_ADDRESS_H;


void LPS25HB_RegisterCallbacks(LPS25HB_ReadCallback read_callback, LPS25HB_WriteCallback write_callback)
{
	if(read_callback != 0)
	{
		lps25hb_read = read_callback;
	}
	if(write_callback != 0)
	{
		lps25hb_write = write_callback;
	}
}

uint8_t LPS25HB_ReadRegisters(uint8_t reg_address, uint8_t *data, uint8_t length)
{
	if(lps25hb_read == 0) return LPS25HB_ERROR;

	/* multi-byte read needs auto increment of register address */
	if(length > 1U)
	{
		reg_address |= LPS25HB_AUTO_INCREMENT;
	}
	return (lps25hb_read(lps25hb_address, reg_address, data, length) == 0U) ? LPS25HB_OK : LPS25HB_ERROR;
}

uint8_t LPS25HB_WriteRegisters(uint8_t reg_address, uint8_t *data, uint8_t length)
{
	if(lps25hb_write == 0) return LPS25HB_ERROR;

	if(length > 1U)
	{
		reg_address |= LPS25HB_AUTO_INCREMENT;
	}
	return (lps25hb_write(lps25hb_address, reg_address, data, length) == 0U) ? LPS25HB_OK : LPS25HB_ERROR;
}

uint8_t LPS25HB_Init(void)
{
	uint8_t value = 0;
	const uint8_t ctrl_reg1 = LPS25HB_CTRL_REG1_PD | LPS25HB_CTRL_REG1_ODR_12HZ | LPS25HB_CTRL_REG1_BDU;

	/* 1. check connection - WHO_AM_I, try both possible addresses (SA0 = 1 / SA0 = 0) */
	lps25hb_address = LPS25HB_DEVICE_ADDRESS_H;
	if(LPS25HB_ReadRegisters(LPS25HB_WHO_AM_I, &value, 1U) != LPS25HB_OK || value != LPS25HB_WHO_AM_I_VALUE)
	{
		lps25hb_address = LPS25HB_DEVICE_ADDRESS_L;
		if(LPS25HB_ReadRegisters(LPS25HB_WHO_AM_I, &value, 1U) != LPS25HB_OK || value != LPS25HB_WHO_AM_I_VALUE)
		{
			return LPS25HB_ERROR;
		}
	}

	/* 2. configuration - power down while changing settings */
	value = 0x00U;
	if(LPS25HB_WriteRegisters(LPS25HB_CTRL_REG1, &value, 1U) != LPS25HB_OK) return LPS25HB_ERROR;

	value = LPS25HB_RES_CONF_VALUE;				//averaging: 16 temperature, 32 pressure samples
	if(LPS25HB_WriteRegisters(LPS25HB_RES_CONF, &value, 1U) != LPS25HB_OK) return LPS25HB_ERROR;

	value = ctrl_reg1;							//power on, 12.5 Hz, BDU
	if(LPS25HB_WriteRegisters(LPS25HB_CTRL_REG1, &value, 1U) != LPS25HB_OK) return LPS25HB_ERROR;

	/* check written configuration */
	if(LPS25HB_ReadRegisters(LPS25HB_CTRL_REG1, &value, 1U) != LPS25HB_OK || value != ctrl_reg1)
	{
		return LPS25HB_ERROR;
	}

	return LPS25HB_OK;
}

uint8_t LPS25HB_ReadPressure(float *pressure)
{
	uint8_t data[3];
	int32_t raw;

	/* PRESS_OUT_XL, PRESS_OUT_L, PRESS_OUT_H in one multi-byte read */
	if(LPS25HB_ReadRegisters(LPS25HB_PRESS_OUT_XL, data, 3U) != LPS25HB_OK)
	{
		return LPS25HB_ERROR;
	}

	/* 24-bit two's complement value */
	raw = (int32_t)(((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0]);
	if(raw & 0x00800000L)
	{
		raw |= (int32_t)0xFF000000UL;		//sign extension
	}

	*pressure = (float)raw / LPS25HB_PRESSURE_SENS;
	return LPS25HB_OK;
}

uint8_t LPS25HB_ReadTemperature(float *temperature)
{
	uint8_t data[2];
	int16_t raw;

	/* TEMP_OUT_L and TEMP_OUT_H in one multi-byte read */
	if(LPS25HB_ReadRegisters(LPS25HB_TEMP_OUT_L, data, 2U) != LPS25HB_OK)
	{
		return LPS25HB_ERROR;
	}
	raw = (int16_t)(((uint16_t)data[1] << 8) | data[0]);

	*temperature = LPS25HB_TEMP_OFFSET + (float)raw / LPS25HB_TEMP_SENS;
	return LPS25HB_OK;
}

/*
 * Relative altitude from barometric formula (international standard atmosphere):
 * h = 44330 * (1 - (p / p0)^(1 / 5.255))
 * p0 - reference pressure measured at ground level (altitude 0 m)
 */
float LPS25HB_CalculateRelativeAltitude(float pressure, float reference_pressure)
{
	if(reference_pressure <= 0.0f)
	{
		return 0.0f;
	}
	return 44330.0f * (1.0f - powf(pressure / reference_pressure, 1.0f / 5.255f));
}
