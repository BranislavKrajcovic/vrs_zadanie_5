/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.h
  * @brief   This file contains all the function prototypes for
  *          the i2c.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_I2C1_Init(void);

/* USER CODE BEGIN Prototypes */
/*
 * I2C master read/write functions (I2C1, polling).
 * dev_address - 8-bit slave address (7-bit address shifted left, e.g. 0xBE)
 * reg_address - register address in slave device
 * data, length - data buffer and number of bytes (multi-byte transfer is done in ONE I2C transaction)
 * return value - I2C_OK (0) or I2C_ERROR (1)
 */
#define I2C_OK			0U
#define I2C_ERROR		1U

uint8_t i2c_write(uint8_t dev_address, uint8_t reg_address, uint8_t *data, uint8_t length);
uint8_t i2c_read(uint8_t dev_address, uint8_t reg_address, uint8_t *data, uint8_t length);
uint8_t i2c_write_byte(uint8_t dev_address, uint8_t reg_address, uint8_t value);
uint8_t i2c_read_byte(uint8_t dev_address, uint8_t reg_address, uint8_t *value);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */

