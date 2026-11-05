/*
 * aht10.h
 *
 * Driver for AHT10 - digital relative humidity and temperature sensor (Aosong).
 * Compatible also with AHT20 (same address, measurement command and data format,
 * only initialization command is different).
 *
 * AHT10 has no registers - it is controlled by commands. Measurement returns humidity
 * and temperature together, so one measurement (AHT10_Measure) is followed by separate
 * functions for each measured quantity.
 *
 * The driver does not use I2C functions directly - write/read functions and delay function
 * have to be registered as callbacks with AHT10_RegisterCallbacks().
 */

#ifndef AHT10_H_
#define AHT10_H_

#include <stdint.h>

/* I2C address (8-bit form): ADDR pin = 0 -> 0x70 (0x38), ADDR pin = 1 -> 0x72 (0x39) */
#define AHT10_DEVICE_ADDRESS		0x70U
#define AHT10_DEVICE_ADDRESS_ALT	0x72U

/* Return values */
#define AHT10_OK					0U
#define AHT10_ERROR					1U

/* Commands */
#define AHT10_CMD_INIT				0xE1U		//+ 0x08, 0x00 (calibration enable)
#define AHT20_CMD_INIT				0xBEU		//+ 0x08, 0x00 (AHT20 version of init command)
#define AHT10_CMD_MEASURE			0xACU		//+ 0x33, 0x00 (trigger measurement)
#define AHT10_CMD_SOFT_RESET		0xBAU
#define AHT10_INIT_PARAM1			0x08U
#define AHT10_INIT_PARAM2			0x00U
#define AHT10_MEASURE_PARAM1		0x33U
#define AHT10_MEASURE_PARAM2		0x00U

/* Status byte bits */
#define AHT10_STATUS_BUSY			0x80U		//1 = measurement in progress
#define AHT10_STATUS_CAL_ENABLED	0x08U		//1 = sensor calibrated (identification of AHT10)

/* Timing [ms] */
#define AHT10_POWER_ON_DELAY_MS		40U
#define AHT10_SOFT_RESET_DELAY_MS	20U
#define AHT10_INIT_DELAY_MS			10U
#define AHT10_MEASURE_DELAY_MS		80U			//measurement takes max. 75 ms

/* Callback function types (provided by MCU part of application) */
typedef uint8_t (*AHT10_WriteCallback)(uint8_t dev_address, uint8_t *data, uint8_t length);
typedef uint8_t (*AHT10_ReadCallback)(uint8_t dev_address, uint8_t *data, uint8_t length);
typedef void (*AHT10_DelayCallback)(uint32_t ms);

/* Registration of I2C write/read functions and delay function */
void AHT10_RegisterCallbacks(AHT10_WriteCallback write_callback, AHT10_ReadCallback read_callback,
							 AHT10_DelayCallback delay_callback);

/* Checks connection (sensor answers and is calibrated) and writes initialization command */
uint8_t AHT10_Init(void);

/* Low level write of command / read of data */
uint8_t AHT10_WriteCommand(uint8_t command, uint8_t param1, uint8_t param2);
uint8_t AHT10_ReadData(uint8_t *data, uint8_t length);
uint8_t AHT10_ReadStatus(uint8_t *status);

/* Starts measurement and reads result (humidity and temperature) */
uint8_t AHT10_Measure(void);

/* Values from last measurement */
uint8_t AHT10_ReadTemperature(float *temperature);	//[degC]
uint8_t AHT10_ReadHumidity(float *humidity);			//[% rH]

#endif /* AHT10_H_ */
