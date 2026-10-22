# Wiring

## Hardware Overview

The target board is an Arduino Uno R3 based on the ATmega328P. The firmware uses:

- 1 BME280 temperature sensor over I2C/TWI
- 1 SSD1306 display over I2C/TWI
- 1 4-wire PWM fan
- 3 push buttons
- 1 rotary encoder
- 3 fault LEDs
- 1 buzzer

## Board/Pin Map

| Hardware           | Board Pin | MCU Pin  | Direction     | Notes                                      |
| ------------------ | --------- | -------- | ------------- | ------------------------------------------ |
| Rotary Encoder DT  | D2        | PD2      | Input         | Internal pull-up enabled                   |
| Fan PWM            | D3        | PD3/OC2B | Output        | PWM signal                                 |
| Rotary Encoder CLK | D4        | PD4      | Input         | Internal pull-up enabled                   |
| Confirm Button     | D5        | PD5      | Input         | Internal pull-up enabled                   |
| Select Button      | D6        | PD6      | Input         | Internal pull-up enabled                   |
| Rate Button        | D7        | PD7      | Input         | Internal pull-up enabled                   |
| Fan Tachometer     | D8        | PB0/ICP1 | Input         | Feedback signal, internal pull-up disabled |
| Fault LED 1        | D14/A0    | PC0      | Output        | Display fault indication                   |
| Fault LED 2        | D15/A1    | PC1      | Output        | Fan-curve fault indication                 |
| Fault LED 3        | D16/A2    | PC2      | Output        | BME280 fault indication                    |
| Buzzer             | D17/A3    | PC3      | Output        | Fan driver fault indication                |
| BME280/SSD1306     | D18/A4    | PC4/SDA  | Bidirectional | SDA shared by BME280 and SSD1306           |
| BME280/SSD1306     | D19/A5    | PC5/SCL  | Output        | SCL shared by BME280 and SSD1306           |

---

## BME280/SSD1306 Wiring

| BME280/SSD1306 Pin | Board Pin | Notes                  |
| ------------------ | --------- | ---------------------- |
| VCC/VIN            | 3.3/5 V   | Check datasheet        |
| GND                | Board GND | Common ground required |
| SDA                | D18       | I2C data               |
| SCL                | D19       | I2C clock              |
| ADDR/SDO           | Board GND | Not always present     |

---

## Fan Wiring

| Fan Pin     | Board Pin | External Supply | Notes                    |
| ----------- | --------- | --------------- | -------------------------|
| GND         | Board GND | GND             | Common ground required   |
| +V          |           | +V              | Usually 12 V for PC fans |
| Tachometer  | D8 // 5 V |                 | Through a 4.7 kΩ R       |
| PWM control | D3        |                 | Directly to the board    |

The 4.7 kΩ external resistor is used to replace the board internal pull-up 20-50 kΩ resistor to get a ~1 mA current insted of a
100-250 μA which produces cleaner measurements.

---

## Push Buttons Wiring

| Button Pin | Board Pin |
| ---------- | --------- |
| Signal     | D5/D6/D7  |
| GND        | GND       |

---

## Rotary Encoder Wiring

| Rotary Encoder Pin | Board Pin |
| ------------------ | --------- |
| CLK                | D4        |
| DT                 | D2        |
| +V                 | 5 V       |
| GND                | GND       |

---

## Fault LEDs and Buzzer

| LED Pin | Board Pin   | Notes                 |
| ------- | ----------- | --------------------- |
| Anode   | D14/D15/D16 | Directly to the board |
| Cathode | GND         | Through a 220 Ω       |

| Buzzer Pin | Board Pin |
| ---------- | --------- |
| GND        | GND       |
| +V         | 5 V       |
| Signal     | D17       |
