# Hardware

## Overview

This document describes the hardware configuration for the STM32 version of the
MPU6050 Attitude Estimator. It covers the target board, peripheral pin assignments,
wiring, I2C address configuration, and system clock settings derived from the
STM32CubeMX `.ioc` file.

---

## Target Board

| Property | Value |
| -------- | ----- |
| Board | NUCLEO-F446RE |
| MCU | STM32F446RET6 |
| Architecture | ARM Cortex-M4 |
| Package | LQFP64 |
| System Clock (SYSCLK) | 84 MHz |
| APB1 Clock (I2C source) | 42 MHz |
| APB2 Clock | 84 MHz |
| Clock Source | HSE → PLL (8 MHz HSE, PLL N=336, P=÷4) |
| Flash Latency | FLASH_LATENCY_2 |
| Debug Interface | SWD (Serial Wire Debug) via on-board ST-LINK/V2 |

---

## Peripheral Pin Assignment

### I2C1 — MPU6050 Sensor Interface

| Nucleo-F446RE Pin | Signal | MPU6050 Pin | Direction | Notes |
| ----------------- | ------ | ----------- | --------- | ----- |
| PB8 | I2C1_SCL | SCL | Output (open-drain) | I2C clock line |
| PB9 | I2C1_SDA | SDA | Bidirectional (open-drain) | I2C data line |

> Pull-up resistors are provided by the MPU6050 breakout module.
> Do not add additional external pull-ups unless using a bare MPU6050 IC.

### USART2 — Telemetry Output

| Nucleo-F446RE Pin | Signal | Destination | Direction | Notes |
| ----------------- | ------ | ----------- | --------- | ----- |
| PA2 | USART2_TX | ST-LINK bridge → USB | Output | Telemetry stream at 115200 baud |
| PA3 | USART2_RX | ST-LINK bridge → USB | Input | Available; not used for telemetry |

> USART2 TX/RX are internally connected to the on-board ST-LINK USB bridge
> on the Nucleo board. The PC sees this as a Virtual COM port. No external
> USB-to-UART adapter is needed.

### SWD Debug Interface

| Nucleo-F446RE Pin | Signal | Notes |
| ----------------- | ------ | ----- |
| PA13 | SWDIO / TMS | Connected to on-board ST-LINK. **Do not reassign.** |
| PA14 | SWCLK / TCK | Connected to on-board ST-LINK. **Do not reassign.** |
| PB3 | SWO | Trace output; supports ITM `printf` debugging |

### GPIO — On-Board Peripherals

| Nucleo-F446RE Pin | Signal | Type | Notes |
| ----------------- | ------ | ---- | ----- |
| PA5 | LD2 (Green LED) | GPIO Output | User LED; available for status indication |
| PC13 | B1 (Blue Button) | GPIO EXTI (falling edge) | User button; available for input |

---

## MPU6050 Module Wiring

### Connection Table

| MPU6050 Pin | Nucleo-F446RE Pin | Wire Color (suggested) | Notes |
| ----------- | ----------------- | ---------------------- | ----- |
| VCC | 3.3V (CN6 pin 4 or CN7 pin 16) | Red | **Use 3.3V only. Do not connect to 5V.** |
| GND | GND (any GND pin) | Black | |
| SDA | PB9 | Blue | I2C1_SDA |
| SCL | PB8 | Yellow | I2C1_SCL |
| AD0 | GND | Black | Tie to GND to set I2C address = 0x68 |
| INT | Not connected | — | Interrupt output; available for future use |
| XDA | Not connected | — | Auxiliary I2C master; not used |
| XCL | Not connected | — | Auxiliary I2C master; not used |

### I2C Address

The AD0 pin controls the least-significant bit of the MPU6050 I2C address:

| AD0 | I2C Address |
| --- | ----------- |
| GND (LOW) | **0x68** ← used in this project |
| VCC (HIGH) | 0x69 |

Most MPU6050 breakout modules pull AD0 to GND internally via an onboard resistor, setting the I2C address to `0x68` without a physical connection to the GND header pin. Verify your module has this pull-down before omitting the connection. If your module lacks it, connect AD0 to GND explicitly.

---

## Wiring Diagram

```
STM32 Nucleo-F446RE                    MPU6050 Module
┌──────────────────────────┐           ┌──────────────────┐
│                          │           │                  │
│  PB8  [I2C1_SCL] ────────┼───────────┼──► SCL           │
│                          │           │                  │
│  PB9  [I2C1_SDA] ────────┼───────────┼──► SDA           │
│                          │           │                  │
│  3.3V ───────────────────┼───────────┼──► VCC           │
│                          │           │                  │
│  GND  ───────────────────┼───────────┼──► GND           │
│                          │           │                  │
│  PA2  [USART2_TX]        │           │   AD0 ──► GND    │
│  PA3  [USART2_RX]        │           │   (addr = 0x68)  │
│       │                  │           └──────────────────┘
│       ▼                  │
│  ST-LINK USB Bridge      │
│  (Virtual COM Port)      │
│                          │
│  PA13 [SWDIO]  ┐         │
│  PA14 [SWCLK]  ├─ ST-LINK│
│  PB3  [SWO]    ┘         │
│                          │
│  PA5  [LD2 LED]          │
│  PC13 [B1 Button]        │
└──────────────────────────┘
```

---

## Power Requirements

| Parameter | Value |
| --------- | ----- |
| Nucleo supply voltage | 5V via USB (CN1) |
| MCU I/O logic level | 3.3V |
| MPU6050 supply voltage | **3.3V** |
| MPU6050 supply current (typical) | ~3.9 mA |

The Nucleo board's on-board 3.3V LDO regulator provides sufficient current
for the MPU6050 module. No external power supply is required.

---

## CubeMX Configuration Reference

The full peripheral and clock configuration is stored in:

```
STM32_Nucleo_F446RE/STM32_Nucleo_F446RE.ioc
```

To inspect or regenerate the HAL initialization:

1. Open STM32CubeIDE.
2. Double-click `STM32_Nucleo_F446RE.ioc` to open CubeMX.
3. Review the **Pinout & Configuration** and **Clock Configuration** tabs.

Do not regenerate unless you intend to update the HAL initialization.
Any code outside `USER CODE BEGIN / END` markers will be overwritten.

---

## References

- [STM32F446RE Datasheet](https://www.st.com/resource/en/datasheet/stm32f446re.pdf)
- [NUCLEO-F446RE User Manual (UM1724)](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf)
- [MPU-6000/MPU-6050 Product Specification](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)
- [MPU-6000/MPU-6050 Register Map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf)
