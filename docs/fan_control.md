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

```text
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
speed_rpm = loaded_curve.nodes[i].speed_rpm + ((dT * dy) / dx);
```

---

## Fan Driving

The fan is driven by `fan_driver`, which receives an updated target speed and performs PWM and feedback operations.

### Boot Delay

The fan is started at 50% duty-cycle for 1500 ms to overcome the initial inertia before checking for missed tachometer readings,
which may not be received in time during boot.

This could be due to a low power supply or a fan with high inertia, and would require the hardware timeout interval to be increased
only for this reason.

### Speed Measurement

The current speed is measured integrating the tachometer feedback, which is captured by using the input capture unit.

In the runtime loop `fan_driver_speed_measure` is called every 300 ms to compute and save the current speed.

#### Tachometer Non-Blocking Hardware Timeout

First, a `response_flag` that only the ISR can set is checked to verify that there has been at least one tachometer reading before calling
`fan_driver_speed_measure`. Then, the current speed is measured from the last captured reading by using the 2 pulses per revolution convention:

```c
captured_speed_rpm = US_PER_MINUTE / (interval_us * TACH_PULSES_PER_REV);
```

and if it is smaller than 90% of the minimum speed previously measured by using `fan_driver_speed_test`, then it is not saved and a tachometer timeout
error is returned.

If the captured speed is bigger than 90% of the measured minimum, then the function proceeds to filter it, and at the end clears the `response_flag`.

This design guarantees a non-blocking tachometer timeout by exploiting the previously measured speed and the `response_flag` global variable.

#### Software and Hardware Speed Filtering

If the tachometer reading is valid, the captured speed is clamped within valid boundaries by `speed_filter` using an estimated acceleration:

```c
#define ACCELERATION_PER_READING_RPM_MS2 ((150UL * SPEED_RANGE_RPM * FAN_DRIVER_SPEED_UPDATE_TIME_MS) / FAN_DRIVER_MAX_SPEED_UPDATE_DELAY_MS) / 100
```

where `SPEED_RANGE_RPM` is the difference between the maximum and minimum speed measured with `fan_driver_speed_test`,
and `FAN_DRIVER_MAX_SPEED_UPDATE_DELAY_MS` is the maximum delay to update the speed from the limit measured values. 

This delay should be measured by using `fan_driver_update_delay_test` for both the update from minimum to maximum and the update from
maximum to minimum speed, then the highest of the two values should be used to maximize speed filtering.

The 150% of the value is considered to account for discarded decimals.

If this is the first capture, the captured speed is saved unfiltered, otherwise is filtered to reduce tachometer noise:

```c
if (!P12MAX_controller.measured_speed_rpm) {
	P12MAX_controller.measured_speed_rpm = captured_speed_rpm;
	return;
}

if (captured_speed_rpm > P12MAX_controller.measured_speed_rpm + ACCELERATION_PER_READING_RPM_MS2) {
	P12MAX_controller.measured_speed_rpm += (uint16_t) ACCELERATION_PER_READING_RPM_MS2;
}
	
else if (captured_speed_rpm + ACCELERATION_PER_READING_RPM_MS2 < P12MAX_controller.measured_speed_rpm) {
	P12MAX_controller.measured_speed_rpm -= (uint16_t) ACCELERATION_PER_READING_RPM_MS2;
}
	
else {
	P12MAX_controller.measured_speed_rpm = captured_speed_rpm;
}
```

and the ATmega328P input noise canceler is activated to integrate an hardware noise reduction:

```c
TIM1_CTRL_REG_B = TIM1_RISING_EDGE_TRIGG | TIM1_INP_CAPT_NOISE_CANC;
```

### Speed Hysteresis

The speed hysteresis value is computed by `fan_controller_init` assuming a linear model:

```c
P12MAX_controller.hysteresis_rpm = ((MIN_DC_REG_STEP * SPEED_RANGE_RPM * 103) / DC_REG_VALUE_RANGE) / 100;
```

The speed variation for the minimum step of the duty-cycle register is estimated by dividing the fan speed range by the total number
of steps of the duty-cycle register. The 103% of this value is computed to account for discarded decimals.

The speed hysteresis is used in `fan_driver_controller_update` to decide whether or not the new target speed should be used to update
the fan controller:

```c
if (target_speed_rpm >= P12MAX_controller.target_speed_rpm) {
	speed_variation_rpm = target_speed_rpm - P12MAX_controller.target_speed_rpm;
}
	
else {
	speed_variation_rpm = P12MAX_controller.target_speed_rpm - target_speed_rpm;
}

if (speed_variation_rpm >= P12MAX_controller.hysteresis_rpm) {
	P12MAX_controller.target_speed_rpm = target_speed_rpm;
```

and if the new target speed is used, the duty-cycle is updated to an estimated value assuming again a linear model:

```c
	ds = P12MAX_controller.target_speed_rpm - FAN_DRIVER_MIN_SPEED_RPM;
	dy = DC_REG_VALUE_RANGE;
	dx = SPEED_RANGE_RPM;
	q = FAN_DRIVER_MIN_DC_REG_VALUE;
	TIM2_DUTY_CYCLE_REG = ((( ds * dy * 1000) / dx) + (q * 1000)) / 1000;
```

### Feedback Controller

If `fan_driver_controller_update` is called with a new target speed lying within the hysteresis range, the duty-cycle is updated by implementing a
feedback controller.

#### Response Stability Check

First, `feedback_control` saves the current timestamp and measured speed, then it raises the `first_step_flag` which is then cleared every
time a new target speed is accepted:

```c
if (!P12MAX_controller.feedback.state.first_step_flag) {
	P12MAX_controller.feedback.reference_speed_rpm = P12MAX_controller.measured_speed_rpm;
	P12MAX_controller.feedback.stability_t_0_ms = scheduler_timestamp_capture();
	P12MAX_controller.feedback.state.first_step_flag = 1;
}
```

then, on the same call, `feedback_control` checks whether the current measured speed lies within 5 RPM from the last one for
a minimum stability time period, which is measured from the last timestamp capture:

```c
if (!scheduler_timer_elapsed(P12MAX_controller.feedback.stability_t_0_ms, FAN_DRIVER_SPEED_STABILITY_TIME_MS)) {
		
	if (P12MAX_controller.measured_speed_rpm > P12MAX_controller.feedback.reference_speed_rpm + MAX_STABILITY_DEVIATION_RPM) {
		P12MAX_controller.feedback.reference_speed_rpm = P12MAX_controller.measured_speed_rpm;
		P12MAX_controller.feedback.stability_t_0_ms = scheduler_timestamp_capture();
		return;
	}
		
	if (P12MAX_controller.measured_speed_rpm + MAX_STABILITY_DEVIATION_RPM < P12MAX_controller.feedback.reference_speed_rpm) {
		P12MAX_controller.feedback.reference_speed_rpm = P12MAX_controller.measured_speed_rpm;
		P12MAX_controller.feedback.stability_t_0_ms = scheduler_timestamp_capture();
		return;
	}
		
	return;
}
```

if it is not, then the current timestamp is captured to reset the stability time, and the current measured speed is saved as a reference to check
the next measured speed in the next call of `fan_driver_controller_update`.

If the current measured speed lies within 5 RPM from the last saved reference the function returns, and if `fan_driver_controller_update` is called
after the stability time period has elapsed, the feedback controller advances by using that reference as the stable response.

#### Error Minimization Speed Adjustment

The feedback controller is based on the minimization of the speed delta between the saved target speed and the last stable measured speed.

First, the previous speed delta is saved and the new one is computed:

```c
P12MAX_controller.feedback.prev_speed_delta_rpm = P12MAX_controller.feedback.speed_delta_rpm;
	
if (P12MAX_controller.target_speed_rpm > P12MAX_controller.feedback.reference_speed_rpm) {
	P12MAX_controller.feedback.speed_delta_rpm = P12MAX_controller.target_speed_rpm - P12MAX_controller.feedback.reference_speed_rpm;
}
	
else {
	P12MAX_controller.feedback.speed_delta_rpm = P12MAX_controller.feedback.reference_speed_rpm - P12MAX_controller.target_speed_rpm;
}
```

then, if the last duty-cycle update resulted in a bigger speed delta with respect to the previous one, the `last_step_flag` is raised, which
prevents `feedback_control` to be called again, and the next feedback step reverts the last one, bringing the speed delta to its minimum value:

```c
if (P12MAX_controller.feedback.speed_delta_rpm > P12MAX_controller.feedback.prev_speed_delta_rpm && P12MAX_controller.feedback.prev_speed_delta_rpm > 0) {
	P12MAX_controller.feedback.state.last_step_flag = 1;
	P12MAX_controller.feedback.state.first_step_flag = 0;
}
```

The duty-cycle is updated in every feedback step by safely checking the duty-cycle register boundaries, and the stability time period is reset:

```c
if (P12MAX_controller.feedback.reference_speed_rpm < P12MAX_controller.target_speed_rpm) {		
		
	if (TIM2_DUTY_CYCLE_REG + MIN_DC_REG_STEP <= TOP_DC_REG_VALUE) {
		TIM2_DUTY_CYCLE_REG += MIN_DC_REG_STEP;
		P12MAX_controller.feedback.stability_t_0_ms = scheduler_timestamp_capture();
	}
}
	
else if (P12MAX_controller.feedback.reference_speed_rpm > P12MAX_controller.target_speed_rpm) {
	
	if (TIM2_DUTY_CYCLE_REG - FAN_DRIVER_MIN_DC_REG_VALUE >= MIN_DC_REG_STEP) {
		TIM2_DUTY_CYCLE_REG = (uint8_t) (TIM2_DUTY_CYCLE_REG - MIN_DC_REG_STEP);
		P12MAX_controller.feedback.stability_t_0_ms = scheduler_timestamp_capture();
	}
}
```

finally, if a new target speed is accepted all the flags and deltas are cleared:

```c
if (speed_variation_rpm >= P12MAX_controller.hysteresis_rpm) {
		...
		...
		...
		P12MAX_controller.feedback.speed_delta_rpm = 0;
		P12MAX_controller.feedback.prev_speed_delta_rpm = 0;
		P12MAX_controller.feedback.state.first_step_flag = 0;
		P12MAX_controller.feedback.state.last_step_flag = 0;
	}
```

This design ensures response stabilization to enforce validity of the value for the feedback, and accounts for the previous speed delta to minimize
the error with the target speed.

### Choice of Timers

As reported in the comments of `fan_driver`, for the used fan the smallest and biggest possible time intervals between pulses are:

```text
MIN - MAX SPEED = 400 RPM - 3300 RPM

3299 RPM - 3300 RPM pulses/s = 109.97 Hz - 110 Hz

3299 RPM - 3300 RPM time between pulses = 9.093665 ms - 9.090909 ms
	
*Required sensitivity*
Smallest change of time between pulses = 2.756 us

400 RPM pulses/s = 13.33 Hz

400 RPM time between pulses = 75 ms

*Required resolution*
Biggest time between pulses = 75 ms
```

The ATmega328P provides Timer1, which is a 16-bit timer resulting in a perfect sensitivity for this task.
The resolution is extended by using an `uint8_t` variable as an overflow counter:

```text
f = (CPU Clock Frequency) / (prescaler)

prescaler register value -> prescaler values: 1 -> 1, 2 -> 8, 3 -> 64, 4 -> 256, 5 -> 1024

For the Arduino UNO R3 the default CPU Clock Frequency = 16 MHz

*Timer sensitivity*
prescaler value = 8 -> MIN time between ticks = 500 ns

Timer resolution = 500 ns * 65536 = 32.768 ms

With an 8-bit overflow count variable the timer resolution can be extended to 256 overflows

*Timer extended resolution*
32.768 ms * 256 = 8388.608 ms
```

Timer2 has been used in FastPWM mode for the PWM generation, since Timer1 and Timer0 where already employed for other tasks,
and it is capable of producing a PWM signal having exactly the required frequency of 25 kHz (outside the human audible band):

```text
f = (CPU Clock Frequency) / (prescaler * (1 + top))

0 <= top value <= 255 (8-bit register)

prescaler register value -> prescaler value: 1 -> 1, 2 -> 8, 3 -> 32, 4 -> 64, 5 -> 128, 6 -> 256, 7 -> 1024

For the Arduino UNO R3 the default CPU Clock Frequency = 16 MHz

*PWM frequency*
prescaler value = 8, top value = 79 -> 25 kHz
```
