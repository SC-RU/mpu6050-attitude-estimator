/******************************************************************************
 * @file    app_tasks.h
 * @brief   Header file for the FreeRTOS application tasks.
 *
 * @details This module provides:
 *          - Task prototypes for sensor acquisition,
 *            attitude estimation, and telemetry output
 *          - Shared RTOS object declarations
 *          - Shared application-state declarations
 *          - Task timing, stack-size, and priority constants
 *
 *          The task layer reorganizes the existing
 *          superloop-based firmware into separate
 *          FreeRTOS tasks while preserving the current
 *          driver, calibration, and attitude modules.
 *
 * @author  Sumedh Camarushi
 * @date    July 01, 2026
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Define to prevent recursive inclusion
// -----------------------------------------------------------------------------

#ifndef APP_TASKS_H
#define APP_TASKS_H

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "mpu6050.h"
#include "calibration.h"
#include "attitude.h"

// -----------------------------------------------------------------------------
// Shared RTOS objects
// -----------------------------------------------------------------------------

extern QueueHandle_t		sensorQueue;	///< Queue carrying raw sensor samples
extern SemaphoreHandle_t	attitudeMutex;	///< Mutex protecting shared attitude state

// -----------------------------------------------------------------------------
// Shared application state
// -----------------------------------------------------------------------------

extern AccelBias	accelBias;			///< Accelerometer bias values
extern GyroBias		gyroBias;			///< Gyroscope bias values
extern Attitude 	accelAttitude;      ///< Accelerometer-based attitude estimate
extern Attitude 	gyroOnlyAttitude;	///< Gyroscope-only attitude estimate
extern Attitude 	gyroAttitude;		///< Corrected gyroscope attitude estimate
extern Attitude 	filteredAttitude;	///< Complementary-filtered attitude estimate
extern uint8_t 		imuReady;			///< IMU initialization state flag

// -----------------------------------------------------------------------------
// Task configuration constants
// -----------------------------------------------------------------------------

#define SENSOR_TASK_STACK_SIZE       256U                   ///< Sensor task stack size in words
#define ATTITUDE_TASK_STACK_SIZE     256U                   ///< Attitude task stack size in words
#define TELEMETRY_TASK_STACK_SIZE    512U                   ///< Telemetry task stack size in words

#define SENSOR_TASK_PRIORITY         (tskIDLE_PRIORITY + 3) ///< Highest task priority	- Sensor acquisition
#define ATTITUDE_TASK_PRIORITY       (tskIDLE_PRIORITY + 2) ///< Medium task priority	- Attitude estimation
#define TELEMETRY_TASK_PRIORITY      (tskIDLE_PRIORITY + 1) ///< Lowest task priority	- Telemetry

#define SENSOR_QUEUE_LENGTH          5U                     ///< Maximum queued sensor samples
#define SENSOR_TASK_PERIOD_MS        10U                    ///< 100 Hz sensor acquisition period
#define TELEMETRY_TASK_PERIOD_MS     100U                   ///< 10 Hz telemetry period
#define COMPLEMENTARY_ALPHA_RTOS     0.98f                  ///< Complementary filter weighting coefficient (0.98 = 98% gyro, 2% accel)
#define MILLISECONDS_PER_SECOND_F    1000.0f                ///< Number of milliseconds in one second (used for time conversion)

// -----------------------------------------------------------------------------
// Task interface
// -----------------------------------------------------------------------------

/**
 * @brief           Prints one telemetry sample in CSV format.
 *
 * @details         Builds one comma-separated telemetry line and sends it
 *                  over UART.
 *
 *                  Output format:
 *                  time_MS, accel.roll, accel.pitch,
 *                  gyro.roll, gyro.pitch,
 *                  filtered.roll, filtered.pitch
 *
 *                  This format is useful for serial logging, plotting,
 *                  and host-side parsing.
 *
 * @param time_MS:  Timestamp in milliseconds.
 * @param accel:    Pointer to the accelerometer-only attitude estimate.
 * @param gyro:     Pointer to the gyroscope-only attitude estimate.
 * @param filtered: Pointer to the complementary-filtered attitude estimate.
 */
void printTelemetryCSV(
    uint32_t time_MS,
    const Attitude *accel,
    const Attitude *gyro,
    const Attitude *filtered);

/**
 * @brief           Prints telemetry in a human-readable multi-line format.
 *
 * @details         This function preserves the earlier serial output style used
 *                  during bring-up and algorithm debugging. It is intended for
 *                  direct readability in a serial terminal, where each telemetry
 *                  group is labeled and separated for easier inspection.
 *
 *                  CSV output should remain the default choice for structured
 *                  logging, plotting, and external analysis. This readable
 *                  format is retained primarily for debugging and learning.
 *
 * @param time_MS:  Timestamp in milliseconds associated with the current sample.
 * @param accel:    Pointer to the accelerometer-based attitude estimate.
 * @param gyro:     Pointer to the gyro-only attitude estimate.
 * @param filtered: Pointer to the complementary-filtered attitude estimate.
 */
#define ENABLE_LEGACY_TELEMETRY 0U
#if ENABLE_LEGACY_TELEMETRY

void printTelemetryReadable(
    uint32_t time_MS,
    const Attitude *accel,
    const Attitude *gyro,
    const Attitude *filtered);

#endif

/**
 * @brief           Sensor acquisition task.
 *
 * @details         Periodically reads raw accelerometer and
 *                  gyroscope data from the MPU6050 and pushes
 *                  one combined sample into the sensor queue.
 *
 * @param argument: Unused task argument.
 */
void SensorTask(void *argument);

/**
 * @brief           Attitude estimation task.
 *
 * @details         Receives raw sensor samples from the queue,
 *                  converts them to physical units, updates the
 *                  accelerometer and gyroscope attitude estimates,
 *                  and applies the complementary filter.
 *
 * @param argument: Unused task argument.
 */
void AttitudeTask(void *argument);

/**
 * @brief           Telemetry output task.
 *
 * @details         Periodically snapshots the shared attitude
 *                  estimates and transmits them over UART in
 *                  CSV format.
 *
 * @param argument: Unused task argument.
 */
void TelemetryTask(void *argument);

#endif /* APP_TASKS_H */
