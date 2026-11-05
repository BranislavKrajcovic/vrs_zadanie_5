/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hts221.h"
#include "lps25hb.h"
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MEASUREMENT_PERIOD_MS		100U	//CSV data sent with 10 Hz
#define REFERENCE_SAMPLES			20U		//number of pressure samples averaged for reference (ground) pressure
#define INIT_RETRY_PERIOD_MS		1000U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* reference (ground) pressure for relative altitude [hPa] */
static float reference_pressure = 0.0f;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Sensors_Init(void);
static float Measure_ReferencePressure(void);
static void Format_Value(char *buffer, float value, uint8_t int_digits, uint8_t decimals);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* System interrupt init*/
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* I2C functions are registered as callbacks - sensor drivers do not use i2c.c directly */
  HTS221_RegisterCallbacks(i2c_read, i2c_write);
  LPS25HB_RegisterCallbacks(i2c_read, i2c_write);

  Sensors_Init();

  /* reference pressure = actual position is altitude 0 m */
  reference_pressure = Measure_ReferencePressure();

  float temperature = 0.0f, humidity = 0.0f, pressure = 0.0f, altitude = 0.0f;
  char temperature_str[12], humidity_str[12], pressure_str[12], altitude_str[12];
  char message[64];

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  /* read measured values (previous value is kept if reading fails) */
	  HTS221_ReadTemperature(&temperature);
	  HTS221_ReadHumidity(&humidity);
	  LPS25HB_ReadPressure(&pressure);

	  /* post processing - relative altitude from pressure */
	  altitude = LPS25HB_CalculateRelativeAltitude(pressure, reference_pressure);

	  /* CSV: temperature [degC] xx.x, humidity [%] xx, pressure [hPa] xxxx.xx, altitude [m] xx.xx */
	  Format_Value(temperature_str, temperature, 2, 1);
	  Format_Value(humidity_str, humidity, 2, 0);
	  Format_Value(pressure_str, pressure, 4, 2);
	  Format_Value(altitude_str, altitude, 2, 2);

	  sprintf(message, "%s,%s,%s,%s\r\n", temperature_str, humidity_str, pressure_str, altitude_str);
	  USART2_SendString(message);

	  LL_mDelay(MEASUREMENT_PERIOD_MS);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_0)
  {
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {

  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {

  }
  LL_Init1msTick(8000000);
  LL_SetSystemCoreClock(8000000);
  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_HSI);
}

/* USER CODE BEGIN 4 */
/* Initialization of both sensors, repeated until successful (error is reported via USART) */
static void Sensors_Init(void)
{
	uint8_t hts221_ok = 0, lps25hb_ok = 0;

	while(!hts221_ok || !lps25hb_ok)
	{
		if(!hts221_ok)
		{
			hts221_ok = (HTS221_Init() == HTS221_OK);
			if(!hts221_ok) USART2_SendString("HTS221 init error\r\n");
		}
		if(!lps25hb_ok)
		{
			lps25hb_ok = (LPS25HB_Init() == LPS25HB_OK);
			if(!lps25hb_ok) USART2_SendString("LPS25HB init error\r\n");
		}
		if(!hts221_ok || !lps25hb_ok)
		{
			LL_mDelay(INIT_RETRY_PERIOD_MS);
		}
	}
	USART2_SendString("Sensors HTS221 and LPS25HB initialized\r\n");
}

/* Average of several pressure samples - reference pressure for relative altitude */
static float Measure_ReferencePressure(void)
{
	float sum = 0.0f, p;
	uint8_t count = 0;

	LL_mDelay(200);		//first conversion after power on

	for(uint8_t i = 0; i < REFERENCE_SAMPLES; i++)
	{
		if(LPS25HB_ReadPressure(&p) == LPS25HB_OK)
		{
			sum += p;
			count++;
		}
		LL_mDelay(MEASUREMENT_PERIOD_MS);
	}
	return (count > 0U) ? (sum / count) : 0.0f;
}

/*
 * Converts float value to text with fixed number of digits (without printf float support),
 * e.g. int_digits = 2, decimals = 1: 5.27 -> "05.3", -1.234 -> "-01.2"
 */
static void Format_Value(char *buffer, float value, uint8_t int_digits, uint8_t decimals)
{
	uint32_t scale = 1;
	int32_t scaled;
	uint32_t abs_value;

	for(uint8_t i = 0; i < decimals; i++)
	{
		scale *= 10U;
	}

	/* rounding to required number of decimal places */
	scaled = (value >= 0.0f) ? (int32_t)(value * scale + 0.5f) : (int32_t)(value * scale - 0.5f);
	abs_value = (scaled < 0) ? (uint32_t)(-scaled) : (uint32_t)scaled;

	if(decimals > 0U)
	{
		sprintf(buffer, "%s%0*u.%0*u", (scaled < 0) ? "-" : "",
				int_digits, (unsigned int)(abs_value / scale),
				decimals, (unsigned int)(abs_value % scale));
	}
	else
	{
		sprintf(buffer, "%s%0*u", (scaled < 0) ? "-" : "", int_digits, (unsigned int)abs_value);
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
