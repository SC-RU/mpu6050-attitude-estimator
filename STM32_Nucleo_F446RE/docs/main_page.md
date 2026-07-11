# MPU6050 Attitude Estimator

STM32 FreeRTOS firmware for IMU-based attitude estimation.

This project implements register-level MPU6050 bring-up, calibration, sensor data acquisition, and roll/pitch attitude estimation, without relying on third-party MPU6050 libraries. All sensor communication, configuration, calibration, and attitude estimation are built from scratch using direct I2C register access and the MPU6050 datasheet.

The firmware runs on FreeRTOS, with sensor acquisition, attitude estimation, and telemetry output implemented as separate tasks. Roll and pitch are estimated from accelerometer measurements and combined with integrated gyroscope readings through a complementary filter, giving short-term stability from the gyroscope and long-term drift correction from the accelerometer. Startup calibration collects stationary samples for both the accelerometer and gyroscope to compute per-axis bias offsets before normal operation begins.

Browse the **Files** tab for the complete driver, calibration, attitude-estimation, and task documentation. For hardware wiring, build instructions, telemetry format, and project history, see the [full README](https://github.com/SC-RU/mpu6050-attitude-estimator) on GitHub.

*This page is a work in progress and will be expanded as the documentation effort continues.*