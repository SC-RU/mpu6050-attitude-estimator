/* USER CODE BEGIN Header */

/******************************************************************************
 * @file    main.c
 * @brief   Main application entry point for the MPU6050 attitude estimator.
 *
 * @details Responsibilities:
 *          - Initialize hardware interfaces
 *          - Calibrate the IMU
 *          - Manage loop timing
 *          - Acquire sensor data
 *          - Execute attitude estimation algorithms
 *          - Output telemetry
 *
 * @author  Sumedh Camarushi
 * @date    June 25, 2026
 * ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 ******************************************************************************/

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"

/* Private includes ----------------------------------------------------------*/

/* USER CODE BEGIN Includes */

#include <string.h>
#include <stdio.h>

#include "app_tasks.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* Definitions for defaultTask */

/* USER CODE BEGIN PV */

// -----------------------------------------------------------------------------
// Shared RTOS objects
// -----------------------------------------------------------------------------

QueueHandle_t sensorQueue       = 0;	///< Queue carrying raw sensor samples
SemaphoreHandle_t attitudeMutex	= 0;	///< Mutex protecting shared attitude state

// -----------------------------------------------------------------------------
// Task handles
// -----------------------------------------------------------------------------

TaskHandle_t sensorTaskHandle    = NULL;	///< Handle for SensorTask
TaskHandle_t attitudeTaskHandle  = NULL;	///< Handle for AttitudeTask
TaskHandle_t telemetryTaskHandle = NULL;	///< Handle for TelemetryTask

// -----------------------------------------------------------------------------
// Calibration data
// -----------------------------------------------------------------------------

AccelBias accelBias = {0};	///< Accelerometer bias values
GyroBias gyroBias   = {0};	///< Gyroscope bias values

// -----------------------------------------------------------------------------
// Attitude estimates
// -----------------------------------------------------------------------------

Attitude accelAttitude    = {0};	///< Accelerometer-based attitude estimate
Attitude gyroOnlyAttitude = {0};	///< Gyroscope-only attitude estimate (for plotting purposes)
Attitude gyroAttitude     = {0};	///< Corrected attitude estimate used for gyro integration
Attitude filteredAttitude = {0};	///< Complementary-filtered attitude estimate

// -----------------------------------------------------------------------------
// Fault counters
// -----------------------------------------------------------------------------

volatile uint32_t uartFailureCount  = 0U; ///< UART transmit timeout count
volatile uint32_t missedSampleCount = 0U; ///< SensorTask cycles with no valid sample

// -----------------------------------------------------------------------------
// Application state
// -----------------------------------------------------------------------------

uint8_t imuReady = 0;	///< IMU initialization state flag

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN PFP */

// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------

/**
 * @brief   Performs one-time application initialization.
 *
 * @details Handles sensor startup, calibration, initial attitude estimation,
 *          and loop timing initialization.
 */
void appSetup(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */

// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------

void appSetup(void)
{
  // -------------------------------------------------------------------------
  // Hardware initialization
  // -------------------------------------------------------------------------

  // Delay briefly so the sensor can power up before I2C communication begins.

  HAL_Delay(100);

  // -------------------------------------------------------------------------
  // Sensor initialization
  // -------------------------------------------------------------------------

  // Print status messages over UART during startup.

  uart_print("\r\nInitializing MPU6050...\r\n");

  // Give the MPU6050 driver access to the configured I2C peripheral.

  MPU6050_SetI2C(&hi2c1);

  // Initialize the sensor and apply its configuration.

  if (!initializeMPU6050())
  {
    uart_print("MPU6050 initialization failed.");
    Error_Handler();
  }

  // Calibrate the accelerometer while the IMU is stationary.

  uart_print("Calibrating accelerometer...\r\n");
  accelBias = calibrateAccel();

  // Calibrate the gyroscope while the IMU is stationary.

  uart_print("Calibrating gyroscope...\r\n");
  gyroBias = calibrateGyro();

  // -------------------------------------------------------------------------
  // Attitude initialization
  // -------------------------------------------------------------------------

  // Estimate the initial roll and pitch from the calibrated accelerometer.

  if (initializeAttitude(&accelBias, &accelAttitude) != HAL_OK)
  {
    uart_print("Attitude initialization failed.\r\n");
    Error_Handler();
  }

  // Initialize both gyro-based estimates to the same starting attitude.

  gyroOnlyAttitude = accelAttitude;
  gyroAttitude     = accelAttitude;

  // Compute the initial filtered estimate.
  
  if (complementaryFilter(
    &gyroAttitude,
    &accelAttitude,
    COMPLEMENTARY_ALPHA_RTOS,
    &filteredAttitude) != HAL_OK)
  {
    uart_print("Complementary filter initialization failed.\r\n");
    Error_Handler();
  }

  uart_print("Calibration complete.\r\n");

  // Mark initialization as complete.

  imuReady = 1U;

  uart_print("MPU6050 IMU awake.\r\n\r\n");
}

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

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */

  appSetup();

  /* USER CODE END 2 */

  /* Init scheduler */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */

  attitudeMutex = xSemaphoreCreateMutex();

  if (attitudeMutex == 0)
  {
    uart_print("Failed to create attitude mutex.");
    Error_Handler();
  }

  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */

  sensorQueue = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(SensorSample_t));

  if (sensorQueue == 0)
  {
	  uart_print("Failed to create sensor queue.");
	  Error_Handler();
  }

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  
  // -------------------------------------------------------------------------
  // RTOS task creation
  // -------------------------------------------------------------------------

  // Create the sensor acquisition task.
  // This task runs at the highest priority because it establishes the sampling cadence.
  // The task handle is captured (instead of discarded as 0) so its stack usage
  // can later be queried via uxTaskGetStackHighWaterMark().
  
  if (xTaskCreate(
    SensorTask,
    "SensorTask",
    SENSOR_TASK_STACK_SIZE,
    0,
    SENSOR_TASK_PRIORITY,
    &sensorTaskHandle) != pdPASS)
  {
    uart_print("Failed to create SensorTask.");
    Error_Handler();
  }
  
  // Create the attitude estimation task.
  // It blocks on the queue and runs whenever a fresh sensor sample becomes available.

  if (xTaskCreate(
    AttitudeTask,
    "AttitudeTask",
    ATTITUDE_TASK_STACK_SIZE,
    0,
    ATTITUDE_TASK_PRIORITY,
    &attitudeTaskHandle) != pdPASS)
  {
    uart_print("Failed to create AttitudeTask.");
    Error_Handler();
  }

  // Create the telemetry output task.
  // This task runs at the lowest priority because delayed telemetry is less critical
  // than maintaining deterministic sampling and estimation timing.

  if (xTaskCreate(
    TelemetryTask,
    "TelemetryTask",
    TELEMETRY_TASK_STACK_SIZE,
    0,
    TELEMETRY_TASK_PRIORITY,
    &telemetryTaskHandle) != pdPASS)
  {
    uart_print("Failed to create TelemetryTask.");
    Error_Handler();
  }

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */

  // Returning from scheduler typically indicates insufficient heap memory to create the idle and/or timer tasks.
  
  vTaskStartScheduler();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void uart_print(const char *msg)
{
  // Ignore null pointers instead of passing an invalid buffer into the HAL
  // transmit function.

  if (msg == 0)
  {
    return;
  }

  if (HAL_UART_Transmit(&huart2,
                        (uint8_t *)msg,
                         strlen(msg),
                      UART_TRANSMIT_TIMEOUT_MS) != HAL_OK)
  {
    // Increment the UART failure counter for diagnostics.
    uartFailureCount++;
  }
}

// -----------------------------------------------------------------------------
// Stack usage reporting
// -----------------------------------------------------------------------------

#if CHECK_STACK_OVERFLOW_DEBUG

void reportStackHighWaterMarks(void)
{
  char reportBuffer[128];
  UBaseType_t sensorFreeWords    = 0U;
  UBaseType_t attitudeFreeWords  = 0U;
  UBaseType_t telemetryFreeWords = 0U;

  // uxTaskGetStackHighWaterMark() returns the smallest amount of free stack
  // (in words, not bytes) that a given task has ever had since it started
  // running. A LOW returned value means that task came close to overflowing;
  // a HIGH value means the allocated stack size (currently 256 words each,
  // set in app_tasks.h) may be larger than actually needed.

  // Query the minimum recorded free stack for each task since startup. The
  // returned values are in words, not bytes, so they should be interpreted
  // relative to the word-based stack sizes passed to xTaskCreate().

  sensorFreeWords    = uxTaskGetStackHighWaterMark(sensorTaskHandle);
  attitudeFreeWords  = uxTaskGetStackHighWaterMark(attitudeTaskHandle);
  telemetryFreeWords = uxTaskGetStackHighWaterMark(telemetryTaskHandle);

  // Format a single compact diagnostic line so stack headroom can be checked
  // quickly during hardware testing from the serial terminal or log capture.

  snprintf(
      reportBuffer,
      sizeof(reportBuffer),
      "Stack headroom (words) - Sensor: %lu, Attitude: %lu, Telemetry: %lu\r\n",
      (unsigned long)sensorFreeWords,
      (unsigned long)attitudeFreeWords,
      (unsigned long)telemetryFreeWords);

  uart_print(reportBuffer);
}

#endif

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
