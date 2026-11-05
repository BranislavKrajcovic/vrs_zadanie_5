/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
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
/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */
/* max. number of loop iterations while waiting for an I2C flag (protection against blocking) */
#define I2C_TIMEOUT		50000UL

/* Waits until flag(s) in I2C1->ISR are set. Returns I2C_ERROR on NACK or timeout. */
static uint8_t i2c_wait_flag(uint32_t flag)
{
	uint32_t timeout = I2C_TIMEOUT;

	while((I2C1->ISR & flag) == 0U)
	{
		if(LL_I2C_IsActiveFlag_NACK(I2C1) || (--timeout == 0U))
		{
			return I2C_ERROR;
		}
	}
	return I2C_OK;
}

/* Ends transfer after error (NACK / timeout) and prepares I2C for next transfer. */
static void i2c_recover(void)
{
	uint32_t timeout = I2C_TIMEOUT;

	/* after NACK the master sends STOP automatically - wait for it */
	while(!LL_I2C_IsActiveFlag_STOP(I2C1) && (--timeout != 0U));

	LL_I2C_ClearFlag_STOP(I2C1);
	LL_I2C_ClearFlag_NACK(I2C1);
	LL_I2C_ClearFlag_TXE(I2C1);		//flush TXDR
}

/* USER CODE END 0 */

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  LL_I2C_InitTypeDef I2C_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  /**I2C1 GPIO Configuration
  PB6   ------> I2C1_SCL
  PB7   ------> I2C1_SDA
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6|LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

  /* I2C1 interrupt Init */
  NVIC_SetPriority(I2C1_EV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(I2C1_EV_IRQn);

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */

  /** I2C Initialization
  */
  LL_I2C_EnableAutoEndMode(I2C1);
  LL_I2C_DisableOwnAddress2(I2C1);
  LL_I2C_DisableGeneralCall(I2C1);
  LL_I2C_EnableClockStretching(I2C1);
  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.Timing = 0x00201D2B;
  I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  I2C_InitStruct.DigitalFilter = 0;
  I2C_InitStruct.OwnAddress1 = 0;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  LL_I2C_Init(I2C1, &I2C_InitStruct);
  LL_I2C_SetOwnAddress2(I2C1, 0, LL_I2C_OWNADDRESS2_NOMASK);
  /* USER CODE BEGIN I2C1_Init 2 */
  /* internal pull-ups on SDA/SCL (sensor board has its own, these only help) and enable I2C1 */
  LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_6, LL_GPIO_PULL_UP);
  LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_7, LL_GPIO_PULL_UP);
  LL_I2C_Enable(I2C1);

  /* USER CODE END I2C1_Init 2 */

}

/* USER CODE BEGIN 1 */
/*
 * Writes "length" bytes to slave register(s) - one I2C transaction:
 * START, address+W, register, data[0] ... data[length-1], STOP
 */
uint8_t i2c_write(uint8_t dev_address, uint8_t reg_address, uint8_t *data, uint8_t length)
{
	if(length == 0U || length == 255U) return I2C_ERROR;

	LL_I2C_HandleTransfer(I2C1, dev_address, LL_I2C_ADDRSLAVE_7BIT, length + 1U,
						  LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

	/* register address */
	if(i2c_wait_flag(I2C_ISR_TXIS) != I2C_OK) { i2c_recover(); return I2C_ERROR; }
	LL_I2C_TransmitData8(I2C1, reg_address);

	/* data */
	for(uint8_t i = 0; i < length; i++)
	{
		if(i2c_wait_flag(I2C_ISR_TXIS) != I2C_OK) { i2c_recover(); return I2C_ERROR; }
		LL_I2C_TransmitData8(I2C1, data[i]);
	}

	/* STOP generated automatically (AUTOEND) */
	if(i2c_wait_flag(I2C_ISR_STOPF) != I2C_OK) { i2c_recover(); return I2C_ERROR; }
	LL_I2C_ClearFlag_STOP(I2C1);

	return I2C_OK;
}

/*
 * Reads "length" bytes from slave register(s) - one I2C transaction with repeated start:
 * START, address+W, register, RESTART, address+R, data[0] ... data[length-1], STOP
 */
uint8_t i2c_read(uint8_t dev_address, uint8_t reg_address, uint8_t *data, uint8_t length)
{
	if(length == 0U) return I2C_ERROR;

	/* write register address, no STOP (SOFTEND) */
	LL_I2C_HandleTransfer(I2C1, dev_address, LL_I2C_ADDRSLAVE_7BIT, 1U,
						  LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);

	if(i2c_wait_flag(I2C_ISR_TXIS) != I2C_OK) { i2c_recover(); return I2C_ERROR; }
	LL_I2C_TransmitData8(I2C1, reg_address);

	if(i2c_wait_flag(I2C_ISR_TC) != I2C_OK) { i2c_recover(); return I2C_ERROR; }

	/* repeated start + read all bytes, STOP generated automatically */
	LL_I2C_HandleTransfer(I2C1, dev_address, LL_I2C_ADDRSLAVE_7BIT, length,
						  LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

	for(uint8_t i = 0; i < length; i++)
	{
		if(i2c_wait_flag(I2C_ISR_RXNE) != I2C_OK) { i2c_recover(); return I2C_ERROR; }
		data[i] = LL_I2C_ReceiveData8(I2C1);
	}

	if(i2c_wait_flag(I2C_ISR_STOPF) != I2C_OK) { i2c_recover(); return I2C_ERROR; }
	LL_I2C_ClearFlag_STOP(I2C1);

	return I2C_OK;
}

/*
 * Writes "length" bytes without register address: START, address+W, data..., STOP
 */
uint8_t i2c_write_raw(uint8_t dev_address, uint8_t *data, uint8_t length)
{
	if(length == 0U) return I2C_ERROR;

	LL_I2C_HandleTransfer(I2C1, dev_address, LL_I2C_ADDRSLAVE_7BIT, length,
						  LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

	for(uint8_t i = 0; i < length; i++)
	{
		if(i2c_wait_flag(I2C_ISR_TXIS) != I2C_OK) { i2c_recover(); return I2C_ERROR; }
		LL_I2C_TransmitData8(I2C1, data[i]);
	}

	if(i2c_wait_flag(I2C_ISR_STOPF) != I2C_OK) { i2c_recover(); return I2C_ERROR; }
	LL_I2C_ClearFlag_STOP(I2C1);

	return I2C_OK;
}

/*
 * Reads "length" bytes without register address: START, address+R, data..., STOP
 */
uint8_t i2c_read_raw(uint8_t dev_address, uint8_t *data, uint8_t length)
{
	if(length == 0U) return I2C_ERROR;

	LL_I2C_HandleTransfer(I2C1, dev_address, LL_I2C_ADDRSLAVE_7BIT, length,
						  LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

	for(uint8_t i = 0; i < length; i++)
	{
		if(i2c_wait_flag(I2C_ISR_RXNE) != I2C_OK) { i2c_recover(); return I2C_ERROR; }
		data[i] = LL_I2C_ReceiveData8(I2C1);
	}

	if(i2c_wait_flag(I2C_ISR_STOPF) != I2C_OK) { i2c_recover(); return I2C_ERROR; }
	LL_I2C_ClearFlag_STOP(I2C1);

	return I2C_OK;
}

/* Single byte write */
uint8_t i2c_write_byte(uint8_t dev_address, uint8_t reg_address, uint8_t value)
{
	return i2c_write(dev_address, reg_address, &value, 1U);
}

/* Single byte read */
uint8_t i2c_read_byte(uint8_t dev_address, uint8_t reg_address, uint8_t *value)
{
	return i2c_read(dev_address, reg_address, value, 1U);
}

/* USER CODE END 1 */

