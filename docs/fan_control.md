# Fan Control

## Operations Overview

The firmware performs 5 main tasks to control the fan in the runtime loop:

1. Measures the temperature from the BME280
2. Computes the target speed using linear interpolation
3. Generates a PWM signal with an estimated duty-cycle
4. Measures the current speed using tachometer feedback
5. Adjusts the duty-cycle in a finite number of steps

All these are performed by `bme280`, `fan_curve` and `fan_driver`.

---

## Sensor Live Measurements

The following configuration and formulas refer to the BME280 datasheet.

The BME280 sensor is configured in `bme280` to perform continuous temperature measurements:

```
Humidity Oversampling Skipped
Pressure Oversampling Skipped
Temperature Oversampling x 16
Normal Mode
Standby time = 0.5 ms
```

With these parameters the measurement time and the sampling rate are:

$$
t_{measurement,max} = 1.25 + (2.3 \times T_{oversampling}) = 38.05 ms
$$

$$
Sampling Rate_{min} = \frac {1000} {t_{measurement,max} + t_{standby}} \approx 26 Hz
$$

The raw sampled value is in a 20-bit format distributed in 3 8-bit registers:

```c
temp_adc_value = (temp_msb << 12) | (temp_lsb << 4) | (temp_xlsb >> 4);
```

and its resolution is:

$$
T_{resolution} = 16 + (5 - 1) = 20 bit
$$

To obtain the final temperature measurement, the raw sampled value is first compensated and then clamped within the valid range
by `bme280_temp_compensate`.

The configured range is `[0, 65]` degrees Celsius, which is the operating range for full accuracy reported in the datasheet.

The final temperature value is exposed by `bme280` in tenth of Celsius degrees: `XX.X°C`

---

## Target Speed Computation

The target speed is computed in `fan_curve` with a linear interpolation by `fan_curve_get_speed` using the live temperature
measurement `temp_c` as input:

```c
dx = (loaded_curve.nodes[i + 1].temp_c - loaded_curve.nodes[i].temp_c);
dy = (loaded_curve.nodes[i + 1].speed_rpm - loaded_curve.nodes[i].speed_rpm);
dT = (temp_c - loaded_curve.nodes[i].temp_c);
speed_rpm = loaded_curve.nodes[i].speed_rpm + (uint16_t) ((dT * dy) / dx);
```

---

## Fan Driving

The fan is driven by `fan_driver`, which receives an updated target speed every 80 ms and performs PWM and feedback operations.

### Boot Delay

The fan is started at 50% duty-cycle for 1 s to overcome the initial inertia before using the tachometer hardware timeout,
which may not receive a reading in time during boot.

This could be due to a low power supply or a fan with high inertia, and would require the hardware timeout interval to be changed only for this reason.

### Speed Hysteresis

The speed hysteresis value is computed by `fan_driver_boot` and stored in `hysteresis_rpm` using parameters from `config`:

```c
hysteresis_rpm = ((MIN_DC_REG_STEP * FAN_DRIVER_SPEED_RANGE * HYST_CORR_FACT_X1000 * 1000) / DC_REG_VALUE_RANGE) / 1000;
```

The speed variation for a single step of the duty-cycle register is estimated dividing the fan speed range by the total number
of steps of the duty-cycle register, assuming a linear model.

The result is then multiplied by `HYST_CORR_FACT_X1000` to obtain the smallest speed variation accounting for rounded digits and a non-linear
model, and by `MIN_DC_REG_STEP`, which represents the size of the change of the duty-cycle register for the smallest duty-cycle variation.

This `hysteresis_rpm` value works as long as it is bigger than the biggest possible real speed variation due to the smallest duty-cycle variation:
the real speed variation may increase when the duty-cycle decreases below a certain value, and may decrease or stabilize at a fixed value when the
duty-cycle increases above a certain value, following a non-linear model.

For the used fan (Arctic P12 MAX) this is exactly the case, as shown on the RPM/PWM non-linear chart reported on the manufacturer website.

### Feedback Controller

When `fan_driver` receives a new target speed `fan_driver_update` calculates the difference between that and the preceding target speed.

If the difference is bigger than `hysteresis_rpm` the new target speed is stored in `target_speed_rpm` and the duty-cycle is updated to
an initial estimate assuming a linear model:

```c
TIM2_DUTY_CYCLE_REG = ((((target_speed_rpm - FAN_DRIVER_MIN_SPEED_RPM) * DC_REG_VALUE_RANGE * 1000) / FAN_DRIVER_SPEED_RANGE) / 1000) + MIN_DC_REG_VALUE;
```

if after 80 ms `fan_driver` receives the same target speed, `fan_driver_update` calls `fan_driver_tach` to adjust the duty-cycle using the
tachometer feedback.

First, an atomic operation is used to change the `response_received` flag, which gets set to 1 by the ISR every time a tachometer reading is received.

Then, in a 400 ms loop an atomic operation is used again to check the `response_received` flag, and if it is 1 the loop stops.

Finally, an atomic operation is used again to check the `response_received` flag again, so if it is 0 an error is returned, an if it is 1 the tachometer
time interval in microseconds between pulses is stored.

At this point, the time interval between pulses is used to compute the speed in RPM using the convention of 2 pulses per revolution:

```c
measured_speed_rpm = ((1000000 * 60) / (tach_us * TACH_PULSES_PER_REV));
```

and the duty-cycle register is changed of `MIN_DC_REG_STEP` if this change would not break the valid boundaries, if would not bring the duty-cycle
register back to its preceding value, and if `measured_speed_rpm` deviates more than `hysteresis_rpm` from the target speed.

After 80 ms, if the target speed is unchanged, this process is repeated until one of these 3 conditions changes, limiting the possible number of steps.

The main limitation is that by keeping fixed the value of `hysteresis_rpm` so that it is bigger than the biggest real speed variation,
which happens at low duty-cycle values, when the target speed and the duty-cycle are high the maximum error increases: the smallest variation of the
duty-cycle would result in a small speed variation, therefore a small improvement in the approximation, but that could be enough to enter
the hysteresis range and stop the approximation process.

### Choice of Timers

As reported in the comments of `fan_driver`, for the used fan the smallest possible time interval between pulses is:

```
MIN - MAX SPEED = 400 RPM - 3300 RPM

3299 RPM - 3300 RPM pulses/s = 109.97 Hz - 110 Hz

3299 RPM - 3300 RPM time between pulses = 9.093665 ms - 9.090909 ms
	
*Required resolution*
Smallest change of time between pulses = 2.756 us
```

The ATmega328P provides Timer1, which is a 16-bit timer resulting in a perfect resolution for this task:

```
f = (CPU Clock Frequency) / (prescaler)

prescaler register value -> prescaler values: 1 -> 1, 2 -> 8, 3 -> 64, 4 -> 256, 5 -> 1024

For the Arduino UNO R3 the default CPU Clock Frequency = 16 MHz

*Timer resolution*
prescaler value = 8 -> MIN time between ticks = 500 ns
```

Timer2 has been used in FastPWM mode for the PWM generation, since Timer1 and Timer0 where already employed for other tasks, and it is capable of
producing a PWM signal having exactly the required frequency of 25 kHz (outside the human audible band):

```
f = (CPU Clock Frequency) / (prescaler * (1 + top))

0 <= top value <= 255 (8-bit register)

prescaler register value -> prescaler value: 1 -> 1, 2 -> 8, 3 -> 32, 4 -> 64, 5 -> 128, 6 -> 256, 7 -> 1024

For the Arduino UNO R3 the default CPU Clock Frequency = 16 MHz

*Frequency*
prescaler value = 8, top value = 79 -> 25 kHz
```
