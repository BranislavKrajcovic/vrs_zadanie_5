/*
 * bmp180.h
 *
 * Driver for BMP180 - digital barometric pressure and temperature sensor (Bosch).
 *
 * The driver does not use I2C functions directly - read/write functions and delay function
 * have to be registered as callbacks with BMP180_RegisterCallbacks().
 */

#ifndef BMP180_H_
#define BMP180_H_

#include <stdint.h>

/* I2C address (8-bit form, 7-bit address 0x77) */
#define BMP180_DEVICE_ADDRESS		0xEEU

/* Return values */
#define BMP180_OK					0U
#define BMP180_ERROR				1U

/* Register addresses */
#define BMP180_CALIB_START			0xAAU		//calibration coefficients 0xAA - 0xBF
#define BMP180_CALIB_LENGTH			22U
#define BMP180_CHIP_ID				0xD0U		//"WHO_AM_I" register
#define BMP180_SOFT_RESET			0xE0U
#define BMP180_CTRL_MEAS			0xF4U
#define BMP180_OUT_MSB				0xF6U
#define BMP180_OUT_LSB				0xF7U
#define BMP180_OUT_XLSB				0xF8U

/* Register values */
#define BMP180_CHIP_ID_VALUE		0x55U
#define BMP180_SOFT_RESET_VALUE		0xB6U
#define BMP180_CMD_TEMPERATURE		0x2EU
#define BMP180_CMD_PRESSURE			0x34U		//+ (oversampling << 6)
#define BMP180_CTRL_MEAS_SCO		0x20U		//start of conversion bit (1 = conversion running)

/*
 * Oversampling setting for pressure (0 - ultra low power ... 3 - ultra high resolution)
 * conversion time: 0 - 4.5 ms, 1 - 7.5 ms, 2 - 13.5 ms, 3 - 25.5 ms
 */
#define BMP180_OVERSAMPLING			3U

/* Callback function types (provided by MCU part of application) */
typedef uint8_t (*BMP180_ReadCallback)(uint8_t dev_address, uint8_t reg_address, uint8_t *data, uint8_t length);
typedef uint8_t (*BMP180_WriteCallback)(uint8_t dev_address, uint8_t reg_address, uint8_t *data, uint8_t length);
typedef void (*BMP180_DelayCallback)(uint32_t ms);

/* Registration of I2C read/write functions and delay function */
void BMP180_RegisterCallbacks(BMP180_ReadCallback read_callback, BMP180_WriteCallback write_callback,
							  BMP180_DelayCallback delay_callback);

/* Checks chip ID ("WHO_AM_I"), sets configuration (oversampling) and reads calibration data */
uint8_t BMP180_Init(void);

/* Read/write of sensor registers (multi-byte if length > 1) */
uint8_t BMP180_ReadRegisters(uint8_t reg_address, uint8_t *data, uint8_t length);
uint8_t BMP180_WriteRegisters(uint8_t reg_address, uint8_t *data, uint8_t length);

/* Measured values */
uint8_t BMP180_ReadTemperature(float *temperature);	//[degC]
uint8_t BMP180_ReadPressure(float *pressure);			//[hPa]

/* Post processing - relative altitude [m] from actual pressure and reference (ground) pressure [hPa] */
float BMP180_CalculateRelativeAltitude(float pressure, float reference_pressure);

#endif /* BMP180_H_ */
