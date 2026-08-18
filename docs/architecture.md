# Firmware Architecture

## Modules Overview

```text
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

```text
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
│   ├── setup.jpg
│   └── setup2.jpg
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
│   ├── twi_state-machine.md
│   └── wiring.md
│
├── drivers/
│   │
│   ├── bme280/
│   │   ├── bme280.c
│   │   └── bme280.h
│   │
│   ├── fan_driver/
│   │   ├── fan_driver.c
│   │   └── fan_driver.h
│   │
│   ├── ssd1306/
│   │   ├── ssd1306.c
│   │   └── ssd1306.h
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
│   │
│   ├── speed_test/
│   │   ├── main.c
│   │   └── measurements.txt
│   │
│   └── pattern_bitmaps/
│       ├── 8x8_bin_to_hex.c
│       └── hex_to_8x8_bin.c
│
├── LICENSE
├── Makefile
└── README.md
```

## Layered Design

The firmware is divided into four main layers and the dependency direction goes downward:

```text
Application Layer
        ↓
Service Layer
        ↓
Driver Layer
        ↓
MCU Registers / External Hardware
```

For example:

```text
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
- Activates the global interrupt flag
- Starts the configuration phase
- Advances to the runtime loop
- In case of error calls the fault manager and blocks the system

Dependency:

```text
main.c -> ui.c/scheduler.c/fault_manager.c
```

### `ui`

This module owns the application main functions.

Responsibilities:

- Handles the UI using the display service, 3 buttons and 1 rotary encoder
- Manages the GPIO integrating 250 ms software-debounce
- Performs up to 10 nodes fan-curve configuration and storage in EEPROM
- Boots the fan at 50% duty-cycle for 1500 ms to overcome inertia before checking for missing tachometer readings
- Reads the temperature from the BME280 every 10 ms and uses it to obtain the target speed
- Reads the fan speed every 300 ms
- Updates the PWM duty-cycle with the new target speed or to use the feedback controller
- Updates the display every 30 ms to show the real-time temperature and speed readings

Dependency:

```text
ui.c -> GPIO registers/display.c/scheduler.c/fan_curve.c/bme280.c/fan_driver.c
```

### `fault_manager`

This module owns the system faults reporting.

Responsibilities:

- Centralizes system-wide error handling
- Reports 4 faults each for a different device
- Uses the display to show "Err x" for each fault
- Lights up a different LED or the buzzer for each fault
- Generates a 500 Hz tone with the buzzer for fan related faults

Dependency:

```text
fault_manager.c -> GPIO registers/display.c/scheduler.c
```

---

## Service Layer

### `display`

This module owns a high-level interface to control the display.

Responsibilities:

- Stores the pattern bitmaps for 25 8x8-pixel characters in 32 kB Flash
- Writes 8 predefined words
- Writes temperature and speed values
- Handles 400 ms blinking for words and digits

Dependency:

```text
display.c -> ssd1306.c/scheduler.c
```

### `fan_curve`

This module owns the management of the 10 fan-curve nodes.

Responsibilities:

- Stores and loads 10 fan-curve nodes in EEPROM
- Validates fan-curve data using CRC-16
- Calculates the target speed using linear interpolation and exposes it

Dependency:

```text
fan_curve.c -> crc.c
```

### `scheduler`

This module owns the tools for system timing.

Responsibilities:

- Configures Timer0 to generate 1 ms ticks
- Keeps a timestamp in milliseconds
- Provides non-blocking polling to check the amount of passed time
- Provides blocking delaying

Interrupts:

Owns the Timer0 compare match interrupt to measure 1 ms.

Dependency:

```text
scheduler.c -> Timer registers
```

### `crc`

This module owns CRC-16 calculation for data validation.

Responsibilities:

- Computes CRC-16 over byte arrays

---

## Driver Layer

### `twi`

This module owns the ATmega328P TWI/I2C driver.

Responsibilities:

- Configures 7 states of the TWI state-machine and boots it
- Performs 100 kHz I2C communications
- Transmits and receives data from/to external buffers
- Reports TWI-based errors and integrates a 10 ms hardware timeout

Interrupts:

Owns the TWI interrupt used to automatically advance the state-machine.

Dependency:

```text
twi.c -> TWI registers/scheduler.c
```

### `bme280`

This module owns the BME280 sensor driver.

Responsibilities:

- Stores the commands sequences in 32 kB Flash
- Configures the sensor for temperature live readings
- Loads calibration parameters and uses them for temperature compensation
- Exposes temperature values in tenth of Celsius degrees

Dependency:

```text
bme280.c -> BME280 registers/twi.c
```

### `ssd1306`

This module owns the SSD1306 display driver.

Responsibilities:

- Stores the commands sequences in 32 kB Flash
- Configures the SSD1306 in horizontal addressing mode
- Writes 8x8-pixel patterns on an 8x16 grid
- Resets and turns on the display

Dependency:

```text
ssd1306.c -> SSD1306 registers/twi.c
```

### `fan_driver`

This module owns the PWM generation and the feedback controller.

Responsibilities:

- Configures Timer2 to generate a 25 kHz PWM
- Reads tachometer pulses using Timer1 input capture to measure the speed
- Detects missing tachometer readings
- Uses the noise canceler and an estimated acceleration to filter the measured speed
- Calculates the speed hysteresis assuming a linear model and uses it to smooth responsiveness to new targets
- Updates the duty-cycle with a new target speed by estimating a duty-cycle assuming a linear model
- Waits for the measured speed to stabilize in a range of 5 RPM for at least 2 s
- Adjusts the duty-cycle with a finite number of steps by minimizing the speed error after each duty-cycle update

Interrupts:

Owns the Timer1 input capture and overflow interrupts to measure tachometer pulses.

Dependency:

```text
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
- Define standard option temperature values
- Define display positions
- Define temperature and speed limits
- Define fan related measured values
- Define all timing values

Configuration values are modifiable for development purposes, and a specific error will be
generated at compile-time for each incorrect value.

---

## Main Runtime Flow

At startup the firmware enters the fan-curve configuration phase:

```text
1. Shows the standard and the advanced mode selection
2. Checks for user input to toggle or confirm the selected mode
3. Shows the first node with the corrisponding temperature and speed
3. Checks for user input to increase/decrease or change the selected digit
4. Checks for user input to move to the next node or confirm the fan-curve
5. Updates EEPROM with the new fan-curve
```

Once the fan-curve is correctly uploaded in EEPROM, the firmware follows this high-level runtime loop:

```text
1. Performs temperature live readings
2. Computes the target speed for the new temperature
3. Updates the PWM duty-cycle with the new target speed
4. Adjusts the duty-cycle using the feedback controller
5. Displays the measured temperature and speed values
6. Checks for user input to go back to the fan-curve configuration phase
```

Simplified call flow for the runtime loop:

```c
ui_system_runtime_loop()
├── bme280_temp_capture()
├── fan_curve_target_speed_compute()
├── fan_driver_speed_measure()
├── fan_driver_controller_update()
│   └── feedback_control()
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

```text
TWI driver error -> display service error -> UI error -> top-level flow
```

This allows the application layer to decide how to handle each error.

Error categories:

- Parameter errors
- Hardware timeout errors
- I2C communication errors
- EEPROM/CRC errors

---

## Future Improvements

Possible architecture improvements:

- Develop a full HAL instead of spreading register interactions in every layer
- Hide the TWI configuration parameters from the TWI API
- Add a dedicated `post` service for power-on self-test
- Add a dedicated `watchdog` service for reset recovery
- Add a clearer safe-mode policy after repeated watchdog resets
- Add host-side unit tests for CRC-16 and fan-curve interpolation
- Add a logging service if SD-card support is added
