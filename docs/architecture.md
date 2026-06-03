# Software Architecture

The project is divided into three modules:

## MPU6050

Responsible for:

- Register-level communication
- Device initialization
- Sensor configuration
- Raw data acquisition

Files:

- MPU6050.h
- MPU6050.cpp

## Calibration

Responsible for:

- Accelerometer bias estimation
- Gyroscope bias estimation
- Unit conversion

Files:

- Calibration.h
- Calibration.cpp

## Main Application

Responsible for:

- System startup
- Calibration execution
- Sensor polling
- Data output

Files:

- main.cpp