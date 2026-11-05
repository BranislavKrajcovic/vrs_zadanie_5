/*
 * hts221.h
 *
 * Driver for HTS221 - capacitive digital sensor for relative humidity and temperature
 * (X-NUCLEO-IKS01A1 sensor board).
 *
 * The driver does not use I2C functions directly - read/write functions have to be
 * registered as callbacks with HTS221_RegisterCallbacks().
 */

#ifndef HTS221_H_
#define HTS221_H_

#include <stdint.h>

/* I2C address (8-bit form, 7-bit address 0x5F) */
#define HTS221_DEVICE_ADDRESS		0xBEU

/* Return values */
#define HTS221_OK					0U
#define HTS221_ERROR				1U

/* Register addresses */
#define HTS221_WHO_AM_I				0x0FU
#define HTS221_AV_CONF				0x10U
#define HTS221_CTRL_REG1			0x20U
#define HTS221_CTRL_REG2			0x21U
#define HTS221_CTRL_REG3			0x22U
#define HTS221_STATUS_REG			0x27U
#define HTS221_HUMIDITY_OUT_L		0x28U
#define HTS221_HUMIDITY_OUT_H		0x29U
#define HTS221_TEMP_OUT_L			0x2AU
#define HTS221_TEMP_OUT_H			0x2BU

/* Calibration registers (0x30 - 0x3F) */
#define HTS221_CALIB_START			0x30U
#define HTS221_CALIB_LENGTH			16U
#define HTS221_H0_RH_X2				0x30U
#define HTS221_H1_RH_X2				0x31U
#define HTS221_T0_DEGC_X8			0x32U
#define HTS221_T1_DEGC_X8			0x33U
#define HTS221_T1_T0_MSB			0x35U
#define HTS221_H0_T0_OUT_L			0x36U
#define HTS221_H1_T0_OUT_L			0x3AU
#define HTS221_T0_OUT_L				0x3CU
#define HTS221_T1_OUT_L				0x3EU

/* Register values */
#define HTS221_WHO_AM_I_VALUE		0xBCU
#define HTS221_AUTO_INCREMENT		0x80U		//MSB of register address = auto increment (multi-byte read/write)

/* CTRL_REG1 bits */
#define HTS221_CTRL_REG1_PD			0x80U		//power on
#define HTS221_CTRL_REG1_BDU		0x04U		//block data update (MSB and LSB from the same sample)
#define HTS221_CTRL_REG1_ODR_1HZ	0x01U
#define HTS221_CTRL_REG1_ODR_7HZ	0x02U
#define HTS221_CTRL_REG1_ODR_12HZ	0x03U		//12.5 Hz

/* AV_CONF - number of averaged samples: temperature 16 (AVGT = 011), humidity 32 (AVGH = 011) */
#define HTS221_AV_CONF_VALUE		0x1BU

/* Callback function types (provided by MCU part of application) */
typedef uint8_t (*HTS221_ReadCallback)(uint8_t dev_address, uint8_t reg_address, uint8_t *data, uint8_t length);
typedef uint8_t (*HTS221_WriteCallback)(uint8_t dev_address, uint8_t reg_address, uint8_t *data, uint8_t length);

/* Registration of I2C read/write functions */
void HTS221_RegisterCallbacks(HTS221_ReadCallback read_callback, HTS221_WriteCallback write_callback);

/* Checks WHO_AM_I, writes configuration and reads calibration data */
uint8_t HTS221_Init(void);

/* Read/write of sensor registers (multi-byte if length > 1) */
uint8_t HTS221_ReadRegisters(uint8_t reg_address, uint8_t *data, uint8_t length);
uint8_t HTS221_WriteRegisters(uint8_t reg_address, uint8_t *data, uint8_t length);

/* Measured values */
uint8_t HTS221_ReadTemperature(float *temperature);		//[degC]
uint8_t HTS221_ReadHumidity(float *humidity);				//[% rH]

#endif /* HTS221_H_ */
