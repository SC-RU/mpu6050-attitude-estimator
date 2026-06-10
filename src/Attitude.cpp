/******************************************************************************
 * @file    Attitude.cpp
 * @brief   Implementation of attitude estimation and sensor fusion.
 *
 * @details This module computes sensor orientation using
 *          both accelerometer and gyroscope measurements.
 *
 *          The implementation includes:
 *
 *          - Accelerometer-based roll and pitch estimation
 *          - Gyroscope attitude integration
 *          - Complementary filter sensor fusion
 *          - Startup attitude initialization
 * 
 * @author  Sumedh Camarushi
 * @date    June 10, 2026
 ******************************************************************************/

#include "Attitude.h"
#include "MPU6050.h"

// -----------------------------------------------------------------------------
// Accelerometer attitude estimation
// -----------------------------------------------------------------------------

Attitude calculateAccelAttitude(float ax, float ay, float az)
{
    Attitude attitude;
    
    // These equations estimate orientation
    // from the direction of the measured
    // gravity vector.
    //
    // The accelerometer provides an
    // absolute reference that does not
    // accumulate drift.

    attitude.roll = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG;

    attitude.pitch = atan2(ay, az) * RAD_TO_DEG;

    return attitude;
}

// -----------------------------------------------------------------------------
// Gyroscope attitude estimation
// -----------------------------------------------------------------------------

Attitude updateGyroAttitude(
    Attitude previous,
    float gx,
    float gy,
    float gz,
    float dt)
{
    // Integrate angular velocity over time.
    //
    // angle = previous_angle + rate * dt
    //
    // The current implementation estimates
    // only roll and pitch.

    previous.roll += gy * dt;
    previous.pitch += gx * dt;

    // gz is currently unused because
    // yaw estimation is not implemented.
    //
    // The parameter is retained so that
    // the interface naturally extends to
    // future three-axis attitude estimation.

    (void)gz;

    return previous;
}

// -----------------------------------------------------------------------------
// Sensor fusion
// -----------------------------------------------------------------------------

Attitude complementaryFilter(
    Attitude gyro,
    Attitude accel,
    float alpha)
{
    Attitude filtered;

    // Combine the short-term stability
    // of the gyroscope with the long-term
    // reference provided by the accelerometer.

    filtered.roll = alpha * gyro.roll + (1.0f - alpha) * accel.roll;
    filtered.pitch = alpha * gyro.pitch + (1.0f - alpha) * accel.pitch;

    return filtered;
}

// -----------------------------------------------------------------------------
// Startup attitude initialization
// -----------------------------------------------------------------------------

Attitude initializeAttitude(AccelBias accelBias)
{
    // Average multiple accelerometer
    // samples to reduce startup noise.

    float axSum = 0.0f;
    float aySum = 0.0f;
    float azSum = 0.0f;

    for (uint16_t i = 0; i < STARTUP_SAMPLES; i++)
    {
        AccelData accel = readRawAccel();

        axSum += rawAccelToG(accel.x, accelBias.x);
        aySum += rawAccelToG(accel.y, accelBias.y);
        azSum += rawAccelToG(accel.z, accelBias.z);

        // Space measurements evenly
        // during initialization.

        delay(ATTITUDE_INIT_DELAY_MS);
    }

    // Compute the average gravity vector
    // and estimate the initial orientation.

    return calculateAccelAttitude(
        axSum / STARTUP_SAMPLES,
        aySum / STARTUP_SAMPLES,
        azSum / STARTUP_SAMPLES);
}