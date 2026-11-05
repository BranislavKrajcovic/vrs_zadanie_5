/*
 * bmp180.c
 *
 * Driver for BMP180 - pressure and temperature sensor.
 * Compensation formulas are taken from BMP180 datasheet (chapter 3.5).
 */

#include "bmp180.h"
#include <math.h>

/* Registered functions */
static BMP180_ReadCallback bmp180_read = 0;
static BMP180_WriteCallback bmp180_write = 0;
static BMP180_DelayCallback bmp180_delay = 0;

/* Calibration coefficients (EEPROM of sensor) */
static int16_t  ac1, ac2, ac3, b1, b2, mb, mc, md;
static uint16_t ac4, ac5, ac6;

/* conversion time of pressure measurement for each oversampling setting [ms] (rounded up) */
static const uint8_t pressure_conversion_ms[4] = {5, 8, 14, 26};


void BMP180_RegisterCallbacks(BMP180_ReadCallback read_callback, BMP180_WriteCallback write_callback,
							  BMP180_DelayCallback delay_callback)
{
	if(read_callback != 0)  bmp180_read = read_callback;
	if(write_callback != 0) bmp180_write = write_callback;
	if(delay_callback != 0) bmp180_delay = delay_callback;
}

uint8_t BMP180_ReadRegisters(uint8_t reg_address, uint8_t *data, uint8_t length)
{
	/* BMP180 increments register address automatically */
	if(bmp180_read == 0) return BMP180_ERROR;
	return (bmp180_read(BMP180_DEVICE_ADDRESS, reg_address, data, length) == 0U) ? BMP180_OK : BMP180_ERROR;
}

uint8_t BMP180_WriteRegisters(uint8_t reg_address, uint8_t *data, uint8_t length)
{
	if(bmp180_write == 0) return BMP180_ERROR;
	return (bmp180_write(BMP180_DEVICE_ADDRESS, reg_address, data, length) == 0U) ? BMP180_OK : BMP180_ERROR;
}

/* Reads all 11 calibration coefficients at once (22 bytes, MSB first) */
static uint8_t BMP180_ReadCalibration(void)
{
	uint8_t c[BMP180_CALIB_LENGTH];

	if(BMP180_ReadRegisters(BMP180_CALIB_START, c, BMP180_CALIB_LENGTH) != BMP180_OK)
	{
		return BMP180_ERROR;
	}

	ac1 = (int16_t)((c[0]  << 8) | c[1]);
	ac2 = (int16_t)((c[2]  << 8) | c[3]);
	ac3 = (int16_t)((c[4]  << 8) | c[5]);
	ac4 = (uint16_t)((c[6] << 8) | c[7]);
	ac5 = (uint16_t)((c[8] << 8) | c[9]);
	ac6 = (uint16_t)((c[10] << 8) | c[11]);
	b1  = (int16_t)((c[12] << 8) | c[13]);
	b2  = (int16_t)((c[14] << 8) | c[15]);
	mb  = (int16_t)((c[16] << 8) | c[17]);
	mc  = (int16_t)((c[18] << 8) | c[19]);
	md  = (int16_t)((c[20] << 8) | c[21]);

	/* no coefficient may be 0x0000 or 0xFFFF (communication error) */
	for(uint8_t i = 0; i < BMP180_CALIB_LENGTH; i += 2U)
	{
		uint16_t v = (uint16_t)((c[i] << 8) | c[i + 1U]);
		if(v == 0x0000U || v == 0xFFFFU)
		{
			return BMP180_ERROR;
		}
	}
	return BMP180_OK;
}

uint8_t BMP180_Init(void)
{
	uint8_t value;

	if(bmp180_delay == 0) return BMP180_ERROR;

	/* 1. check connection - chip ID */
	if(BMP180_ReadRegisters(BMP180_CHIP_ID, &value, 1U) != BMP180_OK || value != BMP180_CHIP_ID_VALUE)
	{
		return BMP180_ERROR;
	}

	/* 2. soft reset - sensor starts from defined state */
	value = BMP180_SOFT_RESET_VALUE;
	if(BMP180_WriteRegisters(BMP180_SOFT_RESET, &value, 1U) != BMP180_OK) return BMP180_ERROR;
	bmp180_delay(10);

	/* 3. calibration coefficients (configuration of BMP180 = oversampling, set with each measurement) */
	return BMP180_ReadCalibration();
}

/* Starts conversion, waits and reads result (2 or 3 bytes in one multi-byte read) */
static uint8_t BMP180_Measure(uint8_t command, uint8_t wait_ms, uint8_t *data, uint8_t length)
{
	if(BMP180_WriteRegisters(BMP180_CTRL_MEAS, &command, 1U) != BMP180_OK) return BMP180_ERROR;
	bmp180_delay(wait_ms);
	return BMP180_ReadRegisters(BMP180_OUT_MSB, data, length);
}

/* Measures temperature and returns parameter B5 (needed also for pressure calculation) */
static uint8_t BMP180_ReadB5(int32_t *b5)
{
	uint8_t data[2];
	int32_t ut, x1, x2;

	if(BMP180_Measure(BMP180_CMD_TEMPERATURE, 5U, data, 2U) != BMP180_OK) return BMP180_ERROR;

	ut = (int32_t)((data[0] << 8) | data[1]);

	x1 = ((ut - (int32_t)ac6) * (int32_t)ac5) >> 15;
	if((x1 + md) == 0) return BMP180_ERROR;
	x2 = ((int32_t)mc << 11) / (x1 + md);
	*b5 = x1 + x2;

	return BMP180_OK;
}

uint8_t BMP180_ReadTemperature(float *temperature)
{
	int32_t b5;

	if(BMP180_ReadB5(&b5) != BMP180_OK) return BMP180_ERROR;

	*temperature = (float)((b5 + 8) >> 4) / 10.0f;		//0.1 degC -> degC
	return BMP180_OK;
}

uint8_t BMP180_ReadPressure(float *pressure)
{
	uint8_t data[3];
	int32_t b5, up, b6, x1, x2, x3, b3, p;
	uint32_t b4, b7;
	const uint8_t oss = BMP180_OVERSAMPLING;

	/* temperature compensation needs actual temperature */
	if(BMP180_ReadB5(&b5) != BMP180_OK) return BMP180_ERROR;

	if(BMP180_Measure(BMP180_CMD_PRESSURE | (oss << 6), pressure_conversion_ms[oss], data, 3U) != BMP180_OK)
	{
		return BMP180_ERROR;
	}
	up = (int32_t)((((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2]) >> (8U - oss));

	/* compensation (datasheet) */
	b6 = b5 - 4000;
	x1 = ((int32_t)b2 * ((b6 * b6) >> 12)) >> 11;
	x2 = ((int32_t)ac2 * b6) >> 11;
	x3 = x1 + x2;
	b3 = ((((int32_t)ac1 * 4 + x3) << oss) + 2) / 4;

	x1 = ((int32_t)ac3 * b6) >> 13;
	x2 = ((int32_t)b1 * ((b6 * b6) >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = ((uint32_t)ac4 * (uint32_t)(x3 + 32768)) >> 15;
	if(b4 == 0U) return BMP180_ERROR;

	b7 = ((uint32_t)up - (uint32_t)b3) * (uint32_t)(50000UL >> oss);
	if(b7 < 0x80000000UL)
	{
		p = (int32_t)((b7 * 2U) / b4);
	}
	else
	{
		p = (int32_t)((b7 / b4) * 2U);
	}

	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	p = p + ((x1 + x2 + 3791) >> 4);		//[Pa]

	*pressure = (float)p / 100.0f;			//Pa -> hPa
	return BMP180_OK;
}

/*
 * Relative altitude from barometric formula (international standard atmosphere):
 * h = 44330 * (1 - (p / p0)^(1 / 5.255))
 * p0 - reference pressure measured at ground level (altitude 0 m)
 */
float BMP180_CalculateRelativeAltitude(float pressure, float reference_pressure)
{
	if(reference_pressure <= 0.0f)
	{
		return 0.0f;
	}
	return 44330.0f * (1.0f - powf(pressure / reference_pressure, 1.0f / 5.255f));
}
