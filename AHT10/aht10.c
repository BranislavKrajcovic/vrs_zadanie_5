/*
 * aht10.c
 *
 * Driver for AHT10 - relative humidity and temperature sensor.
 */

#include "aht10.h"

/* Registered functions */
static AHT10_WriteCallback aht10_write = 0;
static AHT10_ReadCallback aht10_read = 0;
static AHT10_DelayCallback aht10_delay = 0;

/* I2C address found during init */
static uint8_t aht10_address = AHT10_DEVICE_ADDRESS;

/* Raw values of last measurement (20-bit) */
static uint32_t raw_humidity = 0;
static uint32_t raw_temperature = 0;
static uint8_t measurement_valid = 0;


void AHT10_RegisterCallbacks(AHT10_WriteCallback write_callback, AHT10_ReadCallback read_callback,
							 AHT10_DelayCallback delay_callback)
{
	if(write_callback != 0) aht10_write = write_callback;
	if(read_callback != 0)  aht10_read = read_callback;
	if(delay_callback != 0) aht10_delay = delay_callback;
}

uint8_t AHT10_WriteCommand(uint8_t command, uint8_t param1, uint8_t param2)
{
	uint8_t data[3] = {command, param1, param2};

	if(aht10_write == 0) return AHT10_ERROR;
	return (aht10_write(aht10_address, data, 3U) == 0U) ? AHT10_OK : AHT10_ERROR;
}

uint8_t AHT10_ReadData(uint8_t *data, uint8_t length)
{
	if(aht10_read == 0) return AHT10_ERROR;
	return (aht10_read(aht10_address, data, length) == 0U) ? AHT10_OK : AHT10_ERROR;
}

uint8_t AHT10_ReadStatus(uint8_t *status)
{
	return AHT10_ReadData(status, 1U);
}

/* Soft reset (single command byte) */
static uint8_t AHT10_SoftReset(void)
{
	uint8_t command = AHT10_CMD_SOFT_RESET;

	if(aht10_write == 0) return AHT10_ERROR;
	if(aht10_write(aht10_address, &command, 1U) != 0U) return AHT10_ERROR;
	aht10_delay(AHT10_SOFT_RESET_DELAY_MS);
	return AHT10_OK;
}

uint8_t AHT10_Init(void)
{
	uint8_t status;

	if(aht10_delay == 0) return AHT10_ERROR;

	aht10_delay(AHT10_POWER_ON_DELAY_MS);

	/*
	 * 1. check connection - AHT10 has no WHO_AM_I register, the sensor is identified
	 *    by answering on its I2C address (both possible addresses are tried)
	 */
	aht10_address = AHT10_DEVICE_ADDRESS;
	if(AHT10_SoftReset() != AHT10_OK)
	{
		aht10_address = AHT10_DEVICE_ADDRESS_ALT;
		if(AHT10_SoftReset() != AHT10_OK)
		{
			return AHT10_ERROR;
		}
	}

	/* 2. configuration - initialization command (enables calibration coefficients) */
	if(AHT10_WriteCommand(AHT10_CMD_INIT, AHT10_INIT_PARAM1, AHT10_INIT_PARAM2) != AHT10_OK)
	{
		return AHT10_ERROR;
	}
	aht10_delay(AHT10_INIT_DELAY_MS);

	/* 3. status must report calibrated sensor (CAL bit = 1) */
	if(AHT10_ReadStatus(&status) != AHT10_OK || (status & AHT10_STATUS_CAL_ENABLED) == 0U)
	{
		return AHT10_ERROR;
	}

	measurement_valid = 0;
	return AHT10_OK;
}

uint8_t AHT10_Measure(void)
{
	uint8_t data[6];
	uint8_t retry = 5;

	if(AHT10_WriteCommand(AHT10_CMD_MEASURE, AHT10_MEASURE_PARAM1, AHT10_MEASURE_PARAM2) != AHT10_OK)
	{
		return AHT10_ERROR;
	}
	aht10_delay(AHT10_MEASURE_DELAY_MS);

	/* status + 20-bit humidity + 20-bit temperature in one multi-byte read */
	do
	{
		if(AHT10_ReadData(data, 6U) != AHT10_OK) return AHT10_ERROR;
		if((data[0] & AHT10_STATUS_BUSY) == 0U) break;
		aht10_delay(10);
	} while(--retry > 0U);

	if(data[0] & AHT10_STATUS_BUSY) return AHT10_ERROR;

	raw_humidity    = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
	raw_temperature = ((uint32_t)(data[3] & 0x0FU) << 16) | ((uint32_t)data[4] << 8) | data[5];
	measurement_valid = 1;

	return AHT10_OK;
}

uint8_t AHT10_ReadTemperature(float *temperature)
{
	if(!measurement_valid) return AHT10_ERROR;

	/* T = raw / 2^20 * 200 - 50 */
	*temperature = (float)raw_temperature * 200.0f / 1048576.0f - 50.0f;
	return AHT10_OK;
}

uint8_t AHT10_ReadHumidity(float *humidity)
{
	if(!measurement_valid) return AHT10_ERROR;

	/* RH = raw / 2^20 * 100 */
	*humidity = (float)raw_humidity * 100.0f / 1048576.0f;
	return AHT10_OK;
}
