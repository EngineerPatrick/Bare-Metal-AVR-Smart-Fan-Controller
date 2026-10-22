# Firmware Architecture

## Modules Overview

```
app/
    UI logic and runtime loop

assets/
    Picture of the physical setup

bsp/
    Board-specific pin mapping

config/
    System-wide configuration macros

docs/
    Project documentation

drivers/
    Low-level drivers

services/
    Auxiliary reusable modules

tools/
    Modules for development support
```

## Repository Structure

```
project/
│
├── app/
│   │
│   ├── fault_manager/
│   │   ├── fault_manager.c
│   │   └── fault_manager.h
│   │
│   ├── ui/
│   │   ├── ui.c
│   │   └── ui.h
│   │
│   └── main.c
│
├── assets/
│   └── setup.jpg
│
├── bsp/
│   ├── board.h
│   └── pin_map.h
│
├── config/
│   └── config.h
│
├── docs/
│   ├── Doxyfile
│   ├── architecture.md
│   ├── fan_control.md
│   ├── memory_usage.md
│   └── wiring.md
│
├── drivers/
│   │
│   ├── bme280/
│   │   ├── bme_280.c
│   │   └── bme_280.h
│   │
│   ├── fan_driver/
│   │   ├── fan_driver.c
│   │   └── fan_driver.h
│   │
│   ├── ssd1306/
│   │   ├── ssd_1306.c
│   │   └── ssd_1306.h
│   │
│   └── twi/
│       ├── twi.c
│       └── twi.h
│
├── services/
│   │
│   ├── crc/
│   │   ├── crc.c
│   │   └── crc.h
│   │
│   ├── display/
│   │   ├── display.c
│   │   └── display.h
│   │
│   ├── fan_curve/
│   │   ├── fan_curve.c
│   │   └── fan_curve.h
│   │
│   └── scheduler/
│       ├── scheduler.c
│       └── scheduler.h
│
├── tools/
│   ├── 8x8_bin_to_hex.c
│   └── hex_to_8x8_bin.c
│
├── LICENSE
├── Makefile
└── README.md
```

## Layered Design

The firmware is divided into four main layers and the dependency direction goes downward:

```
Application Layer
        ↓
Service Layer
        ↓
Driver Layer
        ↓
MCU Registers / External Hardware
```

For example:

```
ui.c
└── display.c
    └── ssd_1306.c
        └── twi.c
            └── TWI registers
```

---

## Application Layer

### `main`

`main.c` owns the top-level firmware flow which calls the application layer. 

Responsibilities:

- Initializes and stops the system scheduler
- Starts the configuration phase
- Advances to the runtime loop
- In case of error calls the fault manager and blocks the system

Dependency:

```
main.c -> ui.c/scheduler.c/fault_manager.c
```

### `ui`

This module owns the application main functions.

Responsibilities:

- Performs the 10 nodes fan-curve configuration and storage in EEPROM
- Coordinates real-time sensor-actuator operations in the runtime loop
- Manages the GPIO and input devices integrating 250 ms software-debounce
- Controls the display by calling the display service API

Dependency:

```
ui.c -> display.c/scheduler.c/fan_curve.c/fan_driver.c/bme280.c
```

### `fault_manager`

This module owns the system faults reporting.

Responsibilities:

- Centralizes system-wide error handling
- Reports faults on the display showing "Err x"
- Lights up fault-LEDs
- Generates a tone with the buzzer

Dependency:

```
fault_manager.c -> display.c/scheduler.c
```

---

## Service Layer

### `display`

This module owns a high-level interface to control the display.

Responsibilities:

- Contains the character-bitmaps stored in 32 kB Flash
- Writes predefined words
- Writes temperature and speed values
- Handles blinking for words and digits

Dependency:

```
display.c -> ssd1306.c/scheduler.c
```

### `fan_curve`

This module owns the management of the temp/speed values.

Responsibilities:

- Stores and loads fan-curve nodes in EEPROM
- Validates fan-curve data using CRC
- Calculates the target speed using linear interpolation

Dependency:

```
fan_curve.c -> crc.c
```

### `scheduler`

This module owns the tools for system timing.

Responsibilities:

- Configures Timer0 for 1 ms overflow
- Keeps a millisecond timestamp
- Provides non-blocking polling and blocking delaying

Interrupts:

Owns the Timer0 compare match interrupt to measure 1 ms.

Dependency:

```
scheduler.c -> Timer registers
```

### `crc`

This module owns CRC-16 calculation for data validation.

Responsibilities:

- Computes CRC over byte arrays

This is a pure logic module with no dependencies.

---

## Driver Layer

### `twi`

This module owns the ATmega328P TWI/I2C driver.

Responsibilities:

- Configures and starts the TWI state-machine
- Performs 100 kHz I2C communications
- Transmits and receives data from/to external buffers
- Reports TWI-based errors and integrates a 400 ms hardware timeout

Interrupts:

Owns the TWI interrupt used to advance the state-machine.

Dependency:

```
twi.c -> TWI registers/scheduler.c
```

### `bme280`

This module owns the BME280 sensor driver.

Responsibilities:

- Contains the commands sequence stored in 32 kB Flash
- Configures the sensor for temperature live readings
- Reads calibration parameters and uses them for ADC value compensation
- Exposes temperature values in tenth of Celsius degrees
- Reads the chip ID for testing purposes

Dependency:

```
bme280.c -> BME280 registers/twi.c
```

### `ssd1306`

This module owns the SSD1306 display driver.

Responsibilities:

- Contains the commands sequence stored in 32 kB Flash
- Configures the SSD1306 in horizontal addressing mode
- Sends display data and clears the display
- Writes 8x8 characters on a 8x16 grid

Dependency:

```
ssd_1306.c -> SSD1306 registers/twi.c
```

### `fan_driver`

This module owns the PWM generation and tachometer feedback.

Responsibilities:

- Configures Timer2 to generate a 25 kHz PWM
- Uses speed hysteresis to smooth responsiveness
- Measures tachometer pulses to compute the speed
- Adjusts the duty-cycle with a finite number of steps
- Detects missing tachometer input with a 400 ms hardware timeout

Interrupts:

Owns the Timer1 input capture and overflow interrupts to measure tachometer pulses.

Dependency:

```
fan_driver.c -> Timer registers/scheduler.c
```

---

## BSP Layer

Responsibilities:

- Map board pins to ATmega328P pins
- Map peripherals connections to board pins

Example board.h:

```c
#define DR_D14 (1 << PORTC0)
#define DR_D15 (1 << PORTC1)
#define DR_D16 (1 << PORTC2)
#define DR_D17 (1 << PORTC3)
```

Example pin_map.h:

```c
#define LED1_OUTPUT DR_D14
#define LED2_OUTPUT DR_D15
#define LED3_OUTPUT DR_D16
#define BUZZER_OUTPUT DR_D17
```

---

## Configuration Layer

Responsibilities:

- Define TWI configuration
- Define standard option temp values
- Define timing values
- define display positions
- define temperature limits

Configuration values are modifiable for development purposes.

---

## Main Runtime Flow

The firmware follows this high-level runtime loop:

```
1. Perform temperature live readings
2. Computes the target speed
3. Updates the PWM duty-cycle
4. Adjusts the duty-cycle using tachometer feedback
5. Displays measured temp/speed values
6. Checks for user input
```

Simplified call flow:

```
ui_system_update()
├── bme_280_temp_read()
├── fan_curve_get_speed()
├── fan_driver_update()
│   └── fan_driver_tach()
├── display_temp_write()
└── display_speed_write()
```

---

## Interrupt Usage

The firmware uses interrupts for time-critical or asynchronous events.

Required header:

```c
#include <avr/interrupt.h>
```

General ISR rules used for thread-safety:

- Use `volatile` for ISR-shared global variables
- Use atomic operations to read ISR-shared global variables
- Avoid long blocking operations inside ISRs

---

## Error Handling Strategy

All modules return enum-based centralized error codes that propagates to the top-level flow.

Example pattern:

```
TWI driver error -> display service error -> UI error -> top-level flow
```

This allows the application layer to decide how to handle each error.

Error categories:

- Parameter errors
- Hardware timeout errors
- Communication errors
- EEPROM/CRC errors

---

## Future Improvements

Possible architecture improvements:

- Develop a full HAL instead of spreading register interactions in every layer
- Hide the TWI configuration parameters from the TWI API
- Add a dedicated `post` service for power-on self-test
- Add a dedicated `watchdog` service for reset recovery
- Add a clearer safe-mode policy after repeated watchdog resets
- Add host-side unit tests for CRC and fan-curve interpolation
- Add a logging service if SD-card support is added
