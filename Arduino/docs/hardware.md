# Hardware

## Overview

This document describes the hardware configuration for the Arduino version of the
MPU6050 Attitude Estimator. It covers the target board, peripheral pin assignments,
wiring, I2C address configuration, and system clock settings.

---

## Target Board

| Property | Value |
| -------- | ----- |
| Board | Arduino Nano Every |
| MCU | ATmega4809 |
| Architecture | AVR (8-bit) |
| Clock Speed | 20 MHz (internal oscillator) |
| Operating Voltage | 5V |
| I/O Logic Level | 5V |
| Flash Memory | 48 KB |
| SRAM | 6 KB |
| EEPROM | 256 bytes |
| USB Interface | USB via SAMD11 bridge (Virtual COM port) |

---

## Peripheral Pin Assignment

### I2C (Wire) — MPU6050 Sensor Interface

| Nano Every Pin | Label | MPU6050 Pin | Direction | Notes |
| -------------- | ----- | ----------- | --------- | ----- |
| A4 | SDA | SDA | Bidirectional (open-drain) | I2C data line |
| A5 | SCL | SCL | Output (open-drain) | I2C clock line |

> The Nano Every maintains the original Arduino Nano's I2C pin positions:
> A4 = SDA, A5 = SCL. These pins are shared with the analog input function
> but cannot be used as analog inputs when I2C is active.
> Pull-up resistors are provided by the MPU6050 breakout module.

### Serial (UART0) — Telemetry Output

| Nano Every Pin | Label | Destination | Direction | Notes |
| -------------- | ----- | ----------- | --------- | ----- |
| D1 | TX | USB bridge → PC | Output | Telemetry stream at 115200 baud via `Serial` |
| D0 | RX | USB bridge → PC | Input | Available; not used for telemetry output |

> On the Nano Every, the `Serial` object routes through the on-board SAMD11
> USB bridge chip, appearing on the PC as a Virtual COM port. No external
> USB-to-UART adapter is needed. TX and RX here refer to the ATmega4809's
> UART0, which is internally connected to the SAMD11 bridge.

### On-Board Peripherals

| Nano Every Pin | Signal | Notes |
| -------------- | ------ | ----- |
| D13 | LED_BUILTIN | On-board orange LED; available for status indication |
| RST | RESET | Active-low reset; exposed on header pin |

---

## MPU6050 Module Wiring

### Connection Table

| MPU6050 Pin | Nano Every Pin | Wire Color (suggested) | Notes |
| ----------- | -------------- | ---------------------- | ----- |
| VCC | 3.3V | Red | **Use 3.3V, not 5V.** Most MPU6050 breakout modules accept 3.3V–5V on VCC but the sensor IC itself runs at 3.3V internally via an onboard regulator. Check your specific module. |
| GND | GND | Black | Any GND pin |
| SDA | A4 | Blue | I2C data |
| SCL | A5 | Yellow | I2C clock |
| AD0 | Not connected | — | Pulled to GND internally on most breakout modules; sets I2C address to 0x68 |
| INT | Not connected | — | Interrupt output; available for future use |
| XDA | Not connected | — | Auxiliary I2C master; not used |
| XCL | Not connected | — | Auxiliary I2C master; not used |

> **Important — VCC voltage:** The Nano Every operates at 5V logic. Most MPU6050
> breakout modules include a 3.3V onboard regulator and 5V-tolerant level shifting,
> meaning you can safely connect VCC to either the 3.3V or 5V pin on the Nano Every.
> However, powering from the 3.3V pin is preferred to reduce thermal load. Verify
> your specific MPU6050 module's datasheet or silkscreen before wiring.

### I2C Address

The AD0 pin controls the least-significant bit of the MPU6050 I2C address:

| AD0 | I2C Address |
| --- | ----------- |
| GND (LOW) | **0x68** ← used in this project |
| VCC (HIGH) | 0x69 |

Most MPU6050 breakout modules pull AD0 to GND internally via an onboard
pull-down resistor, setting the address to 0x68 without a physical connection
to the GND header pin. The firmware confirms this by reading the `WHO_AM_I`
register, which returns `0x68` on a successful transaction.

If your module does not have an internal pull-down, connect AD0 explicitly
to GND on the breadboard.

---

## Wiring Diagram

```
Arduino Nano Every                    MPU6050 Module
┌──────────────────────────┐           ┌──────────────────┐
│                          │           │                  │
│  A5  [SCL] ──────────────┼───────────┼──► SCL           │
│                          │           │                  │
│  A4  [SDA] ──────────────┼───────────┼──► SDA           │
│                          │           │                  │
│  3.3V ───────────────────┼───────────┼──► VCC           │
│                          │           │                  │
│  GND  ───────────────────┼───────────┼──► GND           │
│                          │           │                  │
│  D1   [TX] ──────┐       │           │   AD0 (internal  │
│  D0   [RX] ──────┤       │           │   pull-down to   │
│                  ▼       │           │   GND on module) │
│  SAMD11 USB Bridge       │           └──────────────────┘
│  (Virtual COM Port)      │
│                          │
│  D13  [LED_BUILTIN]      │
└──────────────────────────┘
```

---

## Power Requirements

| Parameter | Value |
| --------- | ----- |
| Nano Every supply voltage | 5V via USB |
| MCU operating voltage | 5V |
| 3.3V output current (max) | 50 mA |
| MPU6050 supply current (typical) | ~3.9 mA |

The Nano Every's 3.3V output pin provides sufficient current for the MPU6050
module. No external power supply is required when powering via USB.

---

## Clock Reference

The ATmega4809 on the Nano Every runs at **20 MHz** using its internal oscillator.
Unlike the STM32 version, there is no external crystal on the Nano Every — the
20 MHz clock is generated internally. This is accurate enough for I2C communication
and UART at 115200 baud without issues.

The Wire library (I2C) runs at 100 kHz (standard mode) by default. The Serial
(UART0) telemetry output runs at 115200 baud as configured in `setup()`.

---

## PlatformIO Configuration Reference

The full board and build configuration is stored in:

```
Arduino/platformio.ini
```

Key settings:

```ini
[env:nano_every]
platform  = atmelavr
board     = nano_every
framework = arduino
monitor_speed = 115200
```

To build and upload:

```
pio run                          ; build only
pio run --target upload          ; build and flash
pio device monitor --baud 115200 ; open serial monitor
```

---

## References

- [Arduino Nano Every Datasheet (ABX00028)](https://docs.arduino.cc/resources/datasheets/ABX00028-datasheet.pdf)
- [Arduino Nano Every Full Pinout (ABX00028)](https://docs.arduino.cc/resources/pinouts/ABX00028-full-pinout.pdf)
- [ATmega4809 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/ATmega4808-4809-Data-Sheet-DS40002173A.pdf)
- [MPU-6000/MPU-6050 Product Specification](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)
- [MPU-6000/MPU-6050 Register Map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf)
