/******************************************************************************
 *
 * @file    main.cpp
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
 * @date    June 10, 2026
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <Wire.h>

#include "MPU6050.h"
#include "Calibration.h"
#include "Attitude.h"

// -----------------------------------------------------------------------------
// Calibration data
// -----------------------------------------------------------------------------

AccelBias accelBias = {};     ///< Accelerometer bias values
GyroBias gyroBias   = {};     ///< Gyroscope bias values

// -----------------------------------------------------------------------------
// Attitude estimates
// -----------------------------------------------------------------------------

Attitude accelAttitude    = {};     ///< Accelerometer-based attitude estimate
Attitude gyroAttitude     = {};     ///< Gyroscope-based attitude estimate
Attitude filteredAttitude = {};     ///< Complementary filtered attitude estimate

// -----------------------------------------------------------------------------
// Hardware initialization constants
// -----------------------------------------------------------------------------

constexpr int32_t BAUD_RATE           = 115200;   ///< Serial communication baud rate (bits per second)

// -----------------------------------------------------------------------------
// Timing and filter configuration
// -----------------------------------------------------------------------------

constexpr float MICROSECONDS_PER_SECOND = 1000000.0f;     /// Number of microseconds in one second (used for time conversion)
constexpr uint32_t LOOP_PERIOD_US       = 500000;         ///< Loop period in microseconds (500 ms = 2 Hz)
constexpr float COMPLEMENTARY_ALPHA     = 0.98f;          ///< Complementary filter weighting coefficient (0.98 = 98% gyro, 2% accel)

uint32_t lastLoopTime                   = 0;              ///< Timestamp of the last loop iteration (microseconds)

/**
 * @brief Performs one-time system initialization.
 *
 * Initializes serial communication, I2C communication,
 * configures the MPU6050, performs accelerometer and
 * gyroscope calibration, and initializes the attitude
 * estimator.
 */
void setup()
{
  // -------------------------------------------------------------------------
  // Hardware initialization
  // -------------------------------------------------------------------------
  
  // Set baud rate for serial communication and initialize I2C bus.

  Serial.begin(BAUD_RATE);
  Wire.begin();

  while (!Serial)
  {
  };

  // -------------------------------------------------------------------------
  // Sensor initialization
  // -------------------------------------------------------------------------

  // Initialize MPU6050 IMU and calibrate accelerometer and gyroscope.

  Serial.println("Initializing MPU6050...");
  initializeMPU6050();

  Serial.println("Calibrating accelerometer...");
  accelBias = calibrateAccel();

  Serial.println("Calibrating gyroscope...");
  gyroBias = calibrateGyro();

  // -------------------------------------------------------------------------
  // Attitude initialization
  // -------------------------------------------------------------------------

  // Estimate the initial sensor orientation
  // from the calibrated accelerometer data
  // and initialize loop timing.
  
  accelAttitude       = initializeAttitude(accelBias);
  gyroAttitude        = accelAttitude;
  filteredAttitude    = complementaryFilter(gyroAttitude, accelAttitude, COMPLEMENTARY_ALPHA);

  Serial.println("Calibration complete.");

  lastLoopTime = micros();

  Serial.println("MPU6050 IMU awake.\n");
}

// bool hasRan = false;

/**
 * @brief Executes the main attitude estimation loop.
 *
 * At a fixed sample rate:
 *  - Reads raw sensor data
 *  - Converts measurements into physical units
 *  - Computes accelerometer attitude
 *  - Integrates gyroscope attitude
 *  - Applies complementary filtering
 *  - Outputs telemetry
 */
void loop()
{
  // -------------------------------------------------------------------------
  // Timing control
  // -------------------------------------------------------------------------
  
  // if (hasRan)
  // {
  //   return;
  // }

  // Track current time in us and skip loop if sample period has not elapsed.

  uint32_t currentTime = micros();

  if (currentTime - lastLoopTime < LOOP_PERIOD_US)
  {
    return;
  }

  // Convert elapsed time to seconds.
  // Gyroscope integration requires dt in seconds because
  // angular velocity is expressed in degrees per second.

  float dt = (currentTime - lastLoopTime) / MICROSECONDS_PER_SECOND;

  // Update timestamp for the next iteration.

  lastLoopTime = currentTime;

  // -------------------------------------------------------------------------
  // Sensor acquisition & unit conversion
  // -------------------------------------------------------------------------

  // Read raw sensor data and convert to physical units.

  AccelData accel = readRawAccel();
  GyroData gyro = readRawGyro();

  float axG = rawAccelToG(accel.x, accelBias.x);
  float ayG = rawAccelToG(accel.y, accelBias.y);
  float azG = rawAccelToG(accel.z, accelBias.z);

  float gxDPS = rawGyroToDPS(gyro.x, gyroBias.x);
  float gyDPS = rawGyroToDPS(gyro.y, gyroBias.y);
  float gzDPS = rawGyroToDPS(gyro.z, gyroBias.z);

  // -------------------------------------------------------------------------
  // Attitude estimation
  // -------------------------------------------------------------------------

  // Compute attitude estimates from accelerometer and gyroscope data, then
  // apply complementary filter to combine the estimates.

  accelAttitude     = calculateAccelAttitude(axG, ayG, azG);
  gyroAttitude      = updateGyroAttitude(gyroAttitude, gxDPS, gyDPS, gzDPS, dt);
  filteredAttitude  = complementaryFilter(gyroAttitude, accelAttitude, COMPLEMENTARY_ALPHA);
  
  // Feed the corrected estimate back into the gyroscope
  // attitude estimate to reduce long-term drift.

  gyroAttitude = filteredAttitude;

  // -------------------------------------------------------------------------
  // Telemetry output
  // -------------------------------------------------------------------------

  // Output the current attitude estimates.
  //
  // Multiple telemetry formats are available.
  // Additional debug output can be enabled
  // by uncommenting the sections below.

  // Serial.print("AX (g): ");
  // Serial.print(axG);

  // Serial.print(" AY (g): ");
  // Serial.print(ayG);

  // Serial.print(" AZ (g): ");
  // Serial.println(azG);

  // Serial.print("GX (dps): ");
  // Serial.print(gxDPS);

  // Serial.print(" GY (dps): ");
  // Serial.print(gyDPS);

  // Serial.print(" GZ (dps): ");
  // Serial.println(gzDPS);
  
  Serial.print("Accel Roll: ");
  Serial.print(accelAttitude.roll);

  Serial.print(" Gyro Roll: ");
  Serial.print(gyroAttitude.roll);

  Serial.print(" Filtered Roll: ");
  Serial.println(filteredAttitude.roll);

  Serial.print("Accel Pitch: ");
  Serial.print(accelAttitude.pitch);

  Serial.print(" Gyro Pitch: ");
  Serial.print(gyroAttitude.pitch);

  Serial.print(" Filtered Pitch: ");
  Serial.println(filteredAttitude.pitch);

  // hasRan = true;
}