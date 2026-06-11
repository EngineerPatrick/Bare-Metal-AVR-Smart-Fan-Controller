# Fan Control

## Operations Overview

The firmware performs 5 main tasks to control the fan in the runtime loop:

1. Measures the current temperature from the BME280
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

The final temperature value is exposed by `bme280` in a single-decimal format: `XX.X°C`

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

## Speed Hysteresis

The minimum speed hysteresis value is estimated by `fan_driver_boot` and stored in `min_pwm_step_rpm`:

```c
min_pwm_step_rpm = (((FAN_DRIVER_MAX_SPEED_RPM - FAN_DRIVER_MIN_SPEED_RPM) * MIN_DC_REG_STEP * 1500 / (TIM2_TOP_REG + 1)) / 1000);
```

The speed variation for a single step of the duty-cycle register is calculated dividing the fan speed range by the total number
of steps of the duty-cycle register.

The result is then multiplied first by 1.5 to obtain the minimum speed variation accounting for rounded digits, and then by `MIN_DC_REG_STEP`,
which represents the number of steps of the duty-cycle register for the minimum duty-cycle variation.

This `min_pwm_step_rpm` value works as long as it is bigger than the biggest real speed variation for a single step of the duty-cycle:
the real speed may vary more when the duty-cycle is small and less as the duty-cycle grows, following a non-linear model.

For the used fan (Arctic P12 MAX) this is exactly the case, as shown on the RPM/PWM non-linear chart reported on the manufacturer website.

### Feedback Controller

When `fan_driver` receives a new target speed `fan_driver_update` calculates the difference between that and the preceding target speed.

If the difference is bigger than `min_pwm_step_rpm` the new target speed is stored in `target_speed_rpm` and the duty-cycle is updated to
an initial estimate:

```c
TIM2_DUTY_CYCLE_REG = (((TIM2_TOP_REG * pwm.target_speed_rpm * 1000) / FAN_DRIVER_MAX_SPEED_RPM ) / 1000);
```

if after 80 ms `fan_driver` receives the same target speed, `fan_driver_update` calls `fan_driver_tach` to adjust the duty-cycle using the
tachometer feedback.

First, it uses an atomic operation to read the time interval between tachometer pulses in microseconds, then it computes the speed in RPM
using the convention of 2 pulses per revolution:

```c
measured_speed_rpm = ((1000000 * 60) / (tach_us * TACH_PULSES_PER_REV));
```

If the deviation between the measured and the target speed is bigger than `min_pwm_step_rpm` it increases/decreases the duty-cycle register
by `MIN_DC_REG_STEP`.

After 80 ms, if the target speed is unchanged, this process is repeated until either the measured speed matches the target speed or the deviation
is less than `min_pwm_step_rpm`.

The main limitation is that by keeping fixed the value of `min_pwm_step_rpm` so that it is bigger than the biggest real speed variation,
which happens at low duty-cycle values, when the target speed and the duty-cycle are high the maximum error increases: a single step of the
duty-cycle would result in a small speed variation, therefore a small improvement in the approximation, but that could be enough to enter
the hysteresis range and stop the approximation process.
