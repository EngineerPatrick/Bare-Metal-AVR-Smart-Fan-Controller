# Memory Usage

The firmware is designed to respect memory constraints in an embedded context. Dynamic memory and recursive functions have been completely avoided.

## Resources Overview

The main memory resources are:

```
Flash:   32 KB
SRAM:     2 KB
EEPROM:   1 KB
```

## Checking Memory Usage

After building the firmware memory usage can be checked with:

```bash
avr-size --mcu=atmega328p --format=avr build/launcher.elf
```

or through the Makefile:

```bash
make all
```

Typical output format:

```bash
AVR Memory Usage
----------------
Device: atmega328p

Program:    XXXX bytes (YY.Y% Full)
(.text + .data + .bootloader)

Data:       XXXX bytes (YY.Y% Full)
(.data + .bss + .noinit)

EEPROM:     XXXX bytes (YY.Y% Full)
(.eeprom)
```

`Program` corresponds mainly to Flash usage.

`Data` corresponds to SRAM usage.

## Memory Report

The current firmware outputs the following report:

```bash
AVR Memory Usage
----------------
Device: atmega328p

Program:   10296 bytes (31.4% Full)
(.text + .data + .bootloader)

Data:        564 bytes (27.5% Full)
(.data + .bss + .noinit)

EEPROM:       43 bytes (4.2% Full)
(.eeprom)
```

---

## Memory API

Avr-LibC 2.2.0 provides the API implemented to access EEPROM, Flash and to perform atomic operations.

The version matters since newer versions provide different functions.

### Flash Storage

Constant global variables and structures will be automatically stored in SRAM unless explicitly specified using Avr-LibC API.

Required header:

```c
#include <avr/pgmspace.h> 
```

To store constant data in Flash memory it needs to be stored in the `Program` space by using the `PROGMEM` attribute in the declaration:

```c
uint8_t var_name PROGMEM = 0;
```

to use that data in functions it needs to be loaded on the stack by using different macros.

To load 8-bit variables:

```c
pgm_read_byte(__addr)
```
To load a pointer:

```c
pgm_read_ptr_near(__addr)
```

### EEPROM Storage

To store data in EEPROM the following header is required:

```c
#include <avr/eeprom.h>
```

it can be done by using the `EEMEM` attribute in the declaration:

```c
struct my_struct struct_name EEMEM = {0};
```

to modify EEPROM data the "update" functions are recommended by Avr-LibC.

The firmware uses the "block" functions to store a data structure in EEPROM:

```c
void eeprom_update_block (const void *__src, void *__dst, size_t __n)
```

and to load the data on the stack to use it in functions:

```c
void eeprom_read_block (void *__dst, const void *__src, size_t __n)
```

### SRAM Volatile Variables

The firmware implements interrupts in multiple modules with ISR-shared global variables, where `volatile` and atomic operations are required to 
enforce thread-safety.

Required header:

```c
#include <util/atomic.h>
```

The firmware creates an atomic block to store the value on a separate variable by using:

```c
ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    var = volatile_var;
}
```

---

## Program Space Usage

### `display`

All the character-bitmaps are saved in arrays of 8 `uint8_t` to produce 8x8 characters, this results in an 8-byte structure created by
`display` and a 9-byte structure created by `ssd1306`, both stored on the stack.

The dimensions of the characters are kept small on purpose to prevent SRAM overflow: if there was a buffer as large as the entire
128x64 screen there would be a 1024-bytes structure created by `display` and a 1025-bytes structure created by `ssd1306`, which would overflow
the 2 kB SRAM by themselves.

All the patterns bitmaps are stored in the `Program` space:

```c
static const uint8_t A_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x60, 0x18, 0x16, 0x11, 0x16, 0x18, 0x60, 0x00};
```

these are used by `display_pattern_write`, which takes a pointer to the first byte as a parameter (for example the letter-array name `A_PATTERN`),
and iteratively uses `pgm_byte_read` to copy all the 8 bytes to an array on the stack.

The order of the letters of each word is saved in word-arrays, that contain the names of each letter-array:

```c
static const uint8_t* const SELECT[SELECT_WORD_LENGTH] PROGMEM = {S_PATTERN, E_PATTERN, L_PATTERN, E_PATTERN, C_PATTERN, T_PATTERN};
```

these are used by `display_word_write`, which takes a pointer to the first letter-array name as a parameter (for example the word-array name `SELECT`),
iteratively uses it to load each letter-array name from Flash with `pgm_read_ptr_near` (the name of a letter-array is a pointer to its first byte),
and to pass it as a parameter to `display_pattern_write`.

Similarly, the names of the 10 digit-arrays are saved in another array:

```c
static const uint8_t* const DIGITS[10] PROGMEM = {ZERO_PATTERN, ONE_PATTERN, TWO_PATTERN, THREE_PATTERN, ...};
```

where the index of any digit-array name is equal to the number represented by that digit-array. This is used by `display_temp_write`
and `display_speed_write` to load the correct digit-array name from Flash with `pgm_read_ptr_near`, and to pass it as a parameter
to `display_pattern_write`.

### `bme280`/`ssd1306`

The command sequences for both the BME280 and the SSD1306 programmable external peripherals are constants, therefore are stored in the `Program` space.

For the BME280:

```c
static const uint8_t CONFIG[CONFIG_LENGTH] PROGMEM = {CTRL_HUM_ADD, CTRL_HUM_SETTINGS, CONFIG_ADD, ...};
```

For the SSD1306:

```c
static const uint8_t TURNON[TURNON_LENGTH] PROGMEM = {COMMAND_BYTE, CHARGE_PUMP_SETTING, COMMAND_BYTE, CHARGE_PUMP_ENABLE, ...};
```

And both the modules copy all the bytes in arrays on the stack by iteratively using `pgm_byte_read` repeating the same logic
of `display_pattern_write`.

## EEPROM Storage Usage

The values of the fan-curve nodes are stored in EEPROM in `fan_curve`. Initially, a zero-curve is stored in EERPOM by using the `EEMEM` attribute:

```c
static fan_curve ee_fan_curve EEMEM = {0};
```

then `fan_curve_create` creates another `fan_curve` object on the stack, changes its values using the passed parameters and saves
it in EEPROM by updating the existing block with `eeprom_update_block`.

Finally, it loads it back, recalculates the CRC-16 and verifies it is the same as the stored one.

The object on the stack gets deleted once `fan_curve_create` returns, but the EEPROM object persists and is used by `fan_curve_get_speed`,
which creates another `fan_curve` object on the stack and copies on it the EEPROM `fan_curve` object with `eeprom_read_block`.

Before using the copied values, it calculates the CRC-16 and verifies it is the same as the stored one.

## SRAM Volatile Usage

In `twi` the entire `twi_state_machine` object is declared `volatile`, and the flag `done` is read by using atomic operations to
determine if the communication has finished.

Similarly, in `fan_driver` the entire `pwm_params` object is declared `volatile`, and the `tach_us` variable representing the microseconds
between tachometer pulses is read by using an atomic operation.

The system scheduler repeats the same logic in `scheduler`, where the `timer_params` object is declared `volatile` and the `current_ms` variable
representing the current time-stamp in milliseconds is read by using an atomic operation.
