# Contributing

This document describes the development environment, build process, coding
conventions, and contribution workflow for this project.

---

## Development Environment

### STM32 Version

| Tool | Version |
| ---- | ------- |
| STM32CubeIDE | 1.15.x or later |
| STM32CubeMX | Included with CubeIDE |
| STM32Cube FW_F4 | V1.28.3 |
| Compiler | GCC ARM Embedded (arm-none-eabi-gcc) |
| Debug interface | ST-LINK/V2 (on-board, Nucleo) |
| Target board | NUCLEO-F446RE (STM32F446RETx, LQFP64) |

To regenerate the HAL initialization code, open
`STM32_Nucleo_F446RE/STM32_Nucleo_F446RE.ioc` in STM32CubeMX.
Do not manually edit any file in `Core/Src/` or `Core/Inc/` that contains
a `USER CODE BEGIN / END` comment block unless your changes are placed
strictly inside those markers — CubeMX will overwrite anything outside them.

### Arduino Version

| Tool | Version |
| ---- | ------- |
| PlatformIO | Core 6.x or later |
| IDE | VS Code with PlatformIO extension |
| Platform | Atmel AVR (Arduino Nano Every / ATmega4809) |

Open the `Arduino/` directory as a PlatformIO project.

```
Build:          pio run
Upload:         pio run --target upload
Serial monitor: pio device monitor --baud 115200
```

---

## Hardware Setup

### STM32 Version

| MPU6050 Pin | Nucleo-F446RE Pin | Notes |
| ----------- | ----------------- | ----- |
| VCC | 3.3V (CN6 or CN7) | Do not use 5V |
| GND | GND | Any GND pin |
| SDA | PB9 (I2C1_SDA) | |
| SCL | PB8 (I2C1_SCL) | |
| AD0 | GND | Sets I2C address to 0x68 |

Connect the Nucleo to the PC via the on-board ST-LINK USB connector (CN1).
Telemetry is output on USART2 (PA2 TX / PA3 RX), which routes through the
ST-LINK bridge and appears as a Virtual COM port. Open at 115200 baud.

See `STM32_Nucleo_F446RE/docs/hardware.md` for the full wiring diagram and
pin reference.

### Arduino Version

| MPU6050 Pin | Arduino Nano Every Pin | Notes |
| ----------- | ---------------------- | ----- |
| VCC | 3.3V | |
| GND | GND | |
| SDA | SDA (A4) | |
| SCL | SCL (A5) | |

---

## Building and Flashing

### STM32 Version

1. Open STM32CubeIDE.
2. Import the project: **File → Import → Existing Projects into Workspace**,
   navigate to `STM32_Nucleo_F446RE/`.
3. Select the **Debug** build configuration.
4. Build: **Project → Build All** (`Ctrl+B`).
5. Flash: **Run → Debug** (`F11`) or **Run → Run** (`Ctrl+F11`).
6. Open a serial terminal (e.g., PuTTY, CoolTerm) at **115200 baud** on the
   Virtual COM port assigned to the Nucleo to view the telemetry stream.

### Arduino Version

See the Arduino environment table above. Use `pio run --target upload`.

---

## Coding Conventions

### Naming

| Construct | Convention | Example |
| --------- | ---------- | ------- |
| Functions | camelCase | `readRawAccel()` |
| Types / structs | PascalCase | `AccelData`, `GyroData` |
| Macros / constants | UPPER_SNAKE_CASE | `MPU6050_ADDR`, `WHO_AM_I` |
| Local variables | snake_case | `raw_accel`, `sample_count` |
| File names (STM32) | snake_case `.c/.h` | `mpu6050.c`, `calibration.h` |

### File Headers

All `.c` and `.h` files use the following Doxygen-style header:

```c
/******************************************************************************
 * @file    filename.c
 * @brief   One-line description of this module.
 *
 * @details Expanded description of purpose, responsibilities, and any
 *          important implementation notes.
 *
 * @author  Sumedh Camarushi
 * @date    Month DD, YYYY
 ******************************************************************************/
```

### Return Values

All functions that perform I2C transactions must return `HAL_StatusTypeDef`
(STM32) or `bool` (Arduino). Callers must check return values. Do not
silently discard error returns.

### Null Pointer Checks

Any function that accepts a pointer parameter must validate the pointer
before use:

```c
if ((hi2c == NULL) || (data == NULL))
{
    return HAL_ERROR;
}
```

### Formatting

- Indentation: 4 spaces (no tabs).
- Brace style: Allman (opening brace on its own line).
- Maximum line length: 80 characters where practical.
- One blank line between function definitions.
- Group related register writes with a preceding comment block explaining
  the purpose of the sequence.

---

## Commit Messages

Use the imperative mood and a short subject line (72 characters or fewer).
Prefix with the subsystem or file scope when helpful:

```
stm32: add HAL I2C register write function
stm32: implement MPU6050 initialization sequence
arduino: fix gyroscope bias sign convention
docs: add STM32 calibration procedure
readme: update wiring table for STM32 version
```

Avoid vague messages such as `fix`, `update`, or `changes`.

---

## Testing

Before committing changes to any sensor driver function:

1. Verify the `WHO_AM_I` register returns `0x68`.
2. Confirm stationary accelerometer Z-axis reads `1.00 g ± 0.02 g`.
3. Confirm stationary gyroscope reads are within `± 0.05 dps` on all axes.
4. Confirm the complementary filter output is stable (less than 0.1° variation)
   while the sensor is stationary.
5. Document results in `docs/testing.md` with the date and firmware version.

---

## Repository Layout

```
Arduino/
  docs/          — architecture, calibration, attitude estimation, testing, roadmap
  include/       — .h header files
  src/           — .cpp source files
  platformio.ini

STM32_Nucleo_F446RE/
  Core/
    Inc/         — .h header files
    Src/         — .c source files
  Drivers/       — STM32 HAL (generated, do not edit)
  docs/          — architecture, calibration, attitude estimation, testing,
                   roadmap, hardware
  STM32_Nucleo_F446RE.ioc  — CubeMX configuration

README.md
CONTRIBUTING.md
.gitignore
```

---

## Questions

Open an issue or contact the author at sumedh.camarushi@gmail.com.
