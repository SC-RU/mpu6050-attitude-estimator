/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Hook prototypes */

/**
 * @brief             Handles a FreeRTOS task stack overflow fault.
 *
 * @details           FreeRTOS calls this hook automatically when stack overflow checking
 *                    is enabled in FreeRTOSConfig.h and a task exceeds its allocated
 *                    stack region. On Cortex-M systems, such an overflow can silently
 *                    corrupt adjacent RAM rather than producing an immediate fault.
 *                    This hook converts that condition into an explicit failure by
 *                    reporting the offending task name over UART and then halting the
 *                    system through Error_Handler().
 *
 * @param xTask:      Handle of the task whose stack overflowed.
 * @param pcTaskName: Pointer to the name string of the task whose stack overflowed.
 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
	// The task handle itself is not needed here, only its name string.
	// Casting to void silences an unused-parameter warning.

	(void)xTask;

	// Print an explicit fault message before halting so the failure is visible
	// on the serial terminal instead of appearing as silent memory corruption.

	uart_print("\r\n*** STACK OVERFLOW DETECTED IN TASK: ");
	uart_print((const char *)pcTaskName);
	uart_print(" ***\r\n");

	// A detected stack overflow is unrecoverable for this application. Stop the
	// system immediately rather than continuing with corrupted task memory.

	Error_Handler();
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
  // Print an explicit fault message before halting so the failure is visible
	// on the serial terminal instead of appearing as silent memory corruption.

  uart_print("\r\n*** MALLOC FAILED ***\r\n");
  
  // A detected heap overflow is unrecoverable for this application. Stop the
	// system immediately rather than continuing with corrupted task memory.

  Error_Handler();
}
/* USER CODE END 5 */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

