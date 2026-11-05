/*
 * lps25hb.h
 *
 * Driver for LPS25HB - MEMS pressure sensor (260 - 1260 hPa)
 * (X-NUCLEO-IKS01A1 sensor board).
 *
 * The driver does not use I2C functions directly - read/write functions have to be
 * registered as callbacks with LPS25HB_RegisterCallbacks().
 */

#ifndef LPS25HB_H_
#define LPS25HB_H_

#include <stdint.h>

/* I2C address (8-bit form), depends on SA0 pin: SA0 = 1 -> 0xBA (0x5D), SA0 = 0 -> 0xB8 (0x5C) */
#define LPS25HB_DEVICE_ADDRESS_H	0xBAU
#define LPS25HB_DEVICE_ADDRESS_L	0xB8U

/* Return values */
#define LPS25HB_OK					0U
#define LPS25HB_ERROR				1U

/* Register addresses */
#define LPS25HB_REF_P_XL			0x08U
#define LPS25HB_WHO_AM_I			0x0FU
#define LPS25HB_RES_CONF			0x10U
#define LPS25HB_CTRL_REG1			0x20U
#define LPS25HB_CTRL_REG2			0x21U
#define LPS25HB_CTRL_REG3			0x22U
#define LPS25HB_CTRL_REG4			0x23U
#define LPS25HB_STATUS_REG			0x27U
#define LPS25HB_PRESS_OUT_XL		0x28U
#define LPS25HB_PRESS_OUT_L			0x29U
#define LPS25HB_PRESS_OUT_H			0x2AU
#define LPS25HB_TEMP_OUT_L			0x2BU
#define LPS25HB_TEMP_OUT_H			0x2CU

/* Register values */
#define LPS25HB_WHO_AM_I_VALUE		0xBDU
#define LPS25HB_AUTO_INCREMENT		0x80U		//MSB of register address = auto increment (multi-byte read/write)

/* CTRL_REG1 bits */
#define LPS25HB_CTRL_REG1_PD		0x80U		//power on
#define LPS25HB_CTRL_REG1_ODR_1HZ	0x10U
#define LPS25HB_CTRL_REG1_ODR_7HZ	0x20U
#define LPS25HB_CTRL_REG1_ODR_12HZ	0x30U		//12.5 Hz
#define LPS25HB_CTRL_REG1_ODR_25HZ	0x40U
#define LPS25HB_CTRL_REG1_BDU		0x04U		//block data update

/* RES_CONF - averaging: temperature 16 samples (AVGT = 01), pressure 32 samples (AVGP = 01) */
#define LPS25HB_RES_CONF_VALUE		0x05U

/* Conversion constants (datasheet) */
#define LPS25HB_PRESSURE_SENS		4096.0f		//LSB/hPa
#define LPS25HB_TEMP_SENS			480.0f		//LSB/degC
#define LPS25HB_TEMP_OFFSET			42.5f		//degC

/* Callback function types (provided by MCU part of application) */
typedef uint8_t (*LPS25HB_ReadCallback)(uint8_t dev_address, uint8_t reg_address, uint8_t *data, uint8_t length);
typedef uint8_t (*LPS25HB_WriteCallback)(uint8_t dev_address, uint8_t reg_address, uint8_t *data, uint8_t length);

/* Registration of I2C read/write functions */
void LPS25HB_RegisterCallbacks(LPS25HB_ReadCallback read_callback, LPS25HB_WriteCallback write_callback);

/* Checks WHO_AM_I (both possible I2C addresses) and writes configuration */
uint8_t LPS25HB_Init(void);

/* Read/write of sensor registers (multi-byte if length > 1) */
uint8_t LPS25HB_ReadRegisters(uint8_t reg_address, uint8_t *data, uint8_t length);
uint8_t LPS25HB_WriteRegisters(uint8_t reg_address, uint8_t *data, uint8_t length);

/* Measured values */
uint8_t LPS25HB_ReadPressure(float *pressure);			//[hPa]
uint8_t LPS25HB_ReadTemperature(float *temperature);	//[degC]

/* Post processing - relative altitude [m] from actual pressure and reference (ground) pressure [hPa] */
float LPS25HB_CalculateRelativeAltitude(float pressure, float reference_pressure);

#endif /* LPS25HB_H_ */
