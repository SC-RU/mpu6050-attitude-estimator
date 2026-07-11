/******************************************************************************
 * @file    app_tasks.c
 * @brief   Implementation of the FreeRTOS application tasks.
 *
 * @details This module provides three application tasks:
 *
 *          - SensorTask for periodic raw sensor acquisition
 *          - AttitudeTask for unit conversion and attitude estimation
 *          - TelemetryTask for UART CSV output
 *
 *          The implementation preserves the current firmware
 *          processing pipeline while moving the work into
 *          separate RTOS-managed tasks.
 *
 * @author  Sumedh Camarushi
 * @date    July 01, 2026
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "app_tasks.h"
#include "main.h"

// -----------------------------------------------------------------------------
// Private state
// -----------------------------------------------------------------------------

static uint32_t latestSampleTime_MS = 0U; ///< Timestamp of the latest published attitude snapshot

// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------

void printTelemetryCSV(
  uint32_t time_MS,
  const Attitude *accel,
  const Attitude *gyro,
  const Attitude *filtered)
{
    // A buffer is temporary memory used to store text before transmission.
    // Here, the buffer holds one complete CSV telemetry line.

    char buffer[128];
    int length = 0;

    // Exit immediately if any input pointer is invalid.

    if ((accel == 0) || (gyro == 0) || (filtered == 0))
    {
        return;
    }

    // snprintf() writes formatted text into the buffer.
    // The format string creates one CSV line ending with \r\n.

    length = snprintf(
        buffer,
        sizeof(buffer),
        "%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\r\n",
        (unsigned long)time_MS,
        accel->roll,
        accel->pitch,
        gyro->roll,
        gyro->pitch,
        filtered->roll,
        filtered->pitch);
        
    // Check that the length of the formatted string will not exceed the buffer size.
    // snprintf() truncates the output if the buffer is too small, so this check prevents partial lines from being transmitted.
    
    if ((length < 0) || (length >= (int)sizeof(buffer)))
    {
        return;
    }

    // Transmit the formatted string only if formatting succeeded.

    if (length > 0)
    {
        uart_print(buffer);
    }
}

#if ENABLE_LEGACY_TELEMETRY

void printTelemetryReadable(
    uint32_t time_MS,
    const Attitude *accel,
    const Attitude *gyro,
    const Attitude *filtered)
{
    char buffer[128];
    int length = 0;

    // Exit immediately if any required input pointer is invalid.
    // This prevents the function from formatting incomplete telemetry data.

    if ((accel == 0) || (gyro == 0) || (filtered == 0))
    {
        return;
    }

    // Print the sample timestamp first so the following attitude values can be
    // tied to a specific acquisition instant during live serial monitoring.

    length = snprintf(
        buffer,
        sizeof(buffer),
        "Time (ms): %lu\r\n",
        (unsigned long)time_MS);

    if ((length > 0) && ((size_t)length < sizeof(buffer)))
    {
        uart_print(buffer);
    }

    // Print the roll estimates from each path on one line.
    // This makes it easier to compare accelerometer, gyro-only, and filtered
    // roll behavior while watching the serial output in real time.

    length = snprintf(
        buffer,
        sizeof(buffer),
        "Roll  | Accel: %.3f deg | Gyro: %.3f deg | Filtered: %.3f deg\r\n",
        accel->roll,
        gyro->roll,
        filtered->roll);

    if ((length > 0) && ((size_t)length < sizeof(buffer)))
    {
        uart_print(buffer);
    }

    // Print the pitch estimates on a separate line.
    // Keeping roll and pitch separated by axis makes the output easier to scan
    // and helps with debugging drift, noise, and filter correction behavior.

    length = snprintf(
        buffer,
        sizeof(buffer),
        "Pitch | Accel: %.3f deg | Gyro: %.3f deg | Filtered: %.3f deg\r\n\r\n",
        accel->pitch,
        gyro->pitch,
        filtered->pitch);

    if ((length > 0) && ((size_t)length < sizeof(buffer)))
    {
        uart_print(buffer);
    }
}

#endif

// -----------------------------------------------------------------------------
// Task implementations
// -----------------------------------------------------------------------------

void SensorTask(void *argument)
{
	SensorSample_t sample   = {0};
	TickType_t lastWakeTime = xTaskGetTickCount();

	(void)argument;

	while (1)
	{
        // Enforce a fixed 100 Hz sensor read rate.
	    // vTaskDelayUntil() is used instead of vTaskDelay() so the task wakes
	    // on an absolute schedule and does not drift over time.

	    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));

	    // Do not attempt sensor reads until startup initialization and
	    // calibration have completed. This keeps the task alive while ensuring
	    // no invalid or uncalibrated data enters the queue.

	    if (imuReady == 0U)
	    {
            continue;
	    }

	    // Read accelerometer data first. Skip this cycle if the I2C transaction fails.
	    // The queue should only ever receive complete accel+gyro pairs.

	    if (readRawAccel(&sample.accel) != HAL_OK)
	    {
            continue;
	    }

	    // Read gyroscope data next. Skip this cycle if the second I2C transaction fails.
	    // This avoids mixing a fresh accelerometer sample with a missing gyro sample.

	    if (readRawGyro(&sample.gyro) != HAL_OK)
	    {
            continue;
	    }

	    // Store the system tick associated with this sample.
	    // That timestamp is later transmitted with the fused attitude estimate.

	    sample.time_MS = HAL_GetTick();

	    // Push the combined sample into the queue.
	    // A zero block time means the task will not stall if the queue is full.
	    // In that case, the oldest processing falls behind and this sample is dropped.

	    (void)xQueueSend(sensorQueue, &sample, 0U);
	}
}

void AttitudeTask(void *argument)
{
	SensorSample_t sample = {0};

	float axG = 0.0f;
	float ayG = 0.0f;
	float azG = 0.0f;

	float gxDPS = 0.0f;
	float gyDPS = 0.0f;
	float gzDPS = 0.0f;

	// The sensor task runs at a fixed 10 ms period, so dt is constant.
	// This simplifies gyro integration and keeps the filter behavior predictable.

	float dt = ((float)SENSOR_TASK_PERIOD_MS) / MILLISECONDS_PER_SECOND_F;

	Attitude updatedAccel       = {0};
    Attitude updatedGyroOnly    = {0};
	Attitude updatedGyro        = {0};
    Attitude updatedFiltered    = {0};

	(void)argument;

	while (1)
	{
		// Wait indefinitely for the next raw sensor sample.
	    // This task should only run when fresh data is available.

	    if (xQueueReceive(sensorQueue, &sample, portMAX_DELAY) != pdTRUE)
	    {
            continue;
	    }

	    // Convert the raw accelerometer readings to g after subtracting startup bias.
	    // These values are used for gravity-vector-based roll and pitch estimation.

	    axG = rawAccelToG(sample.accel.x, accelBias.x);
	    ayG = rawAccelToG(sample.accel.y, accelBias.y);
	    azG = rawAccelToG(sample.accel.z, accelBias.z);

	    // Convert the raw gyroscope readings to degrees/second after subtracting bias.
	    // These values are integrated over dt for smooth short-term attitude tracking.

	    gxDPS = rawGyroToDPS(sample.gyro.x, gyroBias.x);
	    gyDPS = rawGyroToDPS(sample.gyro.y, gyroBias.y);
	    gzDPS = rawGyroToDPS(sample.gyro.z, gyroBias.z);

	    // Compute the current accelerometer-based attitude estimate.
	    // This estimate does not drift, but it is sensitive to vibration and motion.

	    if (calculateAccelAttitude(axG, ayG, azG, &updatedAccel) != HAL_OK)
	    {
            continue;
	    }

	    // Update the gyro-only estimate used for comparison.
	    // Keeping a pure integrated estimate is useful for debugging drift behavior.

	    if (updateGyroAttitude(
            &gyroOnlyAttitude,
			gxDPS,
			gyDPS,
			gzDPS,
			dt,
			&updatedGyroOnly) != HAL_OK)
	    {
            continue;
	    }

	    // Update the gyro estimate used by the complementary filter.
	    // This second path is the one corrected by the accelerometer over time.

	    if (updateGyroAttitude(
            &gyroAttitude,
			gxDPS,
			gyDPS,
			gzDPS,
			dt,
			&updatedGyro) != HAL_OK)
	    {
            continue;
	    }

        // Fuse the gyro and accelerometer estimates.
        // The gyro contributes smooth short-term motion tracking, while the
        // accelerometer corrects long-term drift in roll and pitch.

        if (complementaryFilter(
            &updatedGyro,
            &updatedAccel,
            COMPLEMENTARY_ALPHA_RTOS,
            &updatedFiltered) != HAL_OK)
        {
            continue;
        }

	    // Protect the shared attitude estimates while updating them.
	    // The telemetry task reads the same global variables, so the mutex keeps
	    // the snapshots consistent and prevents partial updates.

	    if (xSemaphoreTake(attitudeMutex, portMAX_DELAY) == pdTRUE)
	    {
            accelAttitude       = updatedAccel;
            gyroOnlyAttitude    = updatedGyroOnly;
            filteredAttitude    = updatedFiltered;

            // Publish filtered state back into gyroAttitude for next-step integration
            gyroAttitude        = filteredAttitude;

            latestSampleTime_MS = sample.time_MS;
            
            xSemaphoreGive(attitudeMutex);
	    }
	  }
}

void TelemetryTask(void *argument)
{
    Attitude accelSnapshot    = {0};
    Attitude gyroSnapshot     = {0};
    Attitude filteredSnapshot = {0};

    TickType_t lastWakeTime   = xTaskGetTickCount();
    uint32_t timeSnapshot_MS  = 0U;

    (void)argument;

    #if CHECK_STACK_OVERFLOW_DEBUG

    static uint32_t debugCounter = 0U;
    
    #endif

    while (1)
    {
        // Enforce a slower telemetry rate so UART output does not dominate CPU time.
        // The estimator still runs at 100 Hz, but only every tenth sample is printed.

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(TELEMETRY_TASK_PERIOD_MS));

        // Do not send telemetry until startup initialization has completed.
        // This prevents partially initialized values from being transmitted.

        if (imuReady == 0U)
        {
            continue;
        }

        // Copy the shared attitude estimates while holding the mutex.
        // The copies are stored in local variables so the mutex can be released
        // before the slower UART transmission begins.

        if (xSemaphoreTake(attitudeMutex, portMAX_DELAY) == pdTRUE)
        {
            accelSnapshot    = accelAttitude;
            gyroSnapshot     = gyroOnlyAttitude;
            filteredSnapshot = filteredAttitude;
            timeSnapshot_MS  = latestSampleTime_MS;
        
            xSemaphoreGive(attitudeMutex);
        }
        
        #if ENABLE_LEGACY_TELEMETRY
        
        printTelemetryReadable(
            timeSnapshot_MS,
            &accelSnapshot,
            &gyroSnapshot,
            &filteredSnapshot);
        
        #else
        
        // Send one compact CSV telemetry line over UART.

        printTelemetryCSV(
            timeSnapshot_MS,
            &accelSnapshot,
            &gyroSnapshot,
            &filteredSnapshot);
        
        #endif

        #if CHECK_STACK_OVERFLOW_DEBUG

        debugCounter++;
        if (debugCounter >= 50U)    // every 5 seconds at 10 Hz telemetry
        {
            reportStackHighWaterMarks();
            debugCounter = 0U;
        }
        
        #endif

    }
}
