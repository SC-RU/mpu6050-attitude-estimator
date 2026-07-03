# Testing Results

## Connection Test

The MPU6050 connection was verified by reading the `WHO_AM_I` register.

Observed result:

```text
WHO_AM_I = 0x68
```

Expected result:

```text
0x68
```

Status: PASS

---

## Accelerometer Test

After accelerometer calibration, the IMU was held stationary and level.

Observed readings:

```text
AX (g): 0.000 AY (g): 0.000 AZ (g): 1.000
```

Expected behavior:

- X-axis and Y-axis remain near 0 g
- Z-axis remains near 1 g while level and stationary

Status: PASS

---

## Gyroscope Test

After gyroscope calibration, the IMU was held stationary.

Observed readings:

```text
GX (dps): 0.020 GY (dps): 0.010 GZ (dps): 0.020
```

Expected behavior:

- All axes remain near 0 dps while stationary
- Small residual noise is acceptable

Status: PASS

---

## Attitude Estimation Test

### Accelerometer Estimate

Observed behavior:

- Roll changes during side-to-side tilting
- Pitch changes during forward/backward tilting
- Estimates return near their original values when the board is level again

Status: PASS

### Gyroscope Integration

Observed behavior:

- Roll and pitch change smoothly during motion
- Angle estimates continue changing while the sensor rotates
- Small long-term drift is present, as expected

Status: PASS

### Complementary Filter

Observed behavior:

- Gyroscope contribution provides smooth short-term motion tracking
- Accelerometer contribution corrects long-term drift
- Filtered estimate remains stable while stationary

Status: PASS

---

## FreeRTOS Pipeline Test

The STM32 firmware was rebuilt with the sensor, attitude, and telemetry pipeline split into three FreeRTOS tasks (`SensorTask`, `AttitudeTask`, `TelemetryTask`), coordinated through a queue and a mutex.

### Stack Overflow Detection

An initial run with a 256-word `TelemetryTask` stack triggered `vApplicationStackOverflowHook()`, correctly identifying `TelemetryTask` as the offending task. Increasing `TELEMETRY_TASK_STACK_SIZE` to 512 words resolved the overflow.

Status: PASS

### Stack Headroom

`uxTaskGetStackHighWaterMark()` was used to confirm stack margins after the fix, using the current stack sizes (256 words for `SensorTask` and `AttitudeTask`, 512 words for `TelemetryTask`):

```text
Stack headroom (words) - Sensor: 196, Attitude: 168, Telemetry: 243
```

Expected behavior:

- Each task retains a reasonable margin of free stack under normal operation
- No stack overflow hook is triggered during a sustained run

Status: PASS

### Telemetry Output Under RTOS

A 10-second capture of `TelemetryTask` output was reviewed after the stack fix.

Observed behavior:

- CSV lines are emitted at the expected 10 Hz telemetry rate, with `time_MS` incrementing by 100 ms per line
- Roll and pitch values respond appropriately to manual movement of the board
- Values remain small and stable while the board is stationary
- No stack overflow or malloc failure hooks were triggered during the capture

Status: PASS
