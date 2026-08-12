/**
*   
*   @file fan_driver.c
*
*   @brief Implementation for the fan_driver module.
*
*   @details Contains the PWM logic and the tachometer feedback controller.
*
*	Integrates the util/atomic library of AVR-libC to access ISR-shared
*	global variables atomically.
*
*/

/*
*
*	For the Arctic P12 MAX the tachometer sends 2 pulses per revolution:
*
*	MIN - MAX SPEED = 400 RPM - 3300 RPM
*
*	3299 RPM - 3300 RPM pulses/s = 109.97 Hz - 110 Hz
*
*	3299 RPM - 3300 RPM time between pulses = 9.093665 ms - 9.090909 ms
*	
*	*Required sensitivity*
*	Smallest change of time between pulses = 2.756 us
*
*	400 RPM pulses/s = 13.33 Hz
*
*	400 RPM time between pulses = 75 ms
*
*	*Required resolution*
*	Biggest time between pulses = 75 ms
*
*/

/*
*
*	For the ATmega328P using Timer2 in Fast PWM mode:
*
*	f = (CPU Clock Frequency) / (prescaler * (1 + top))
*
*	0 <= top value <= 255 (8-bit register)
*
*	prescaler register value -> prescaler value: 1 -> 1, 2 -> 8, 3 -> 32, 4 -> 64, 5 -> 128, 6 -> 256, 7 -> 1024
*
*	For the Arduino UNO R3 the default CPU Clock Frequency = 16 MHz
*
*	*PWM frequency*
*	prescaler value = 8, top value = 79 -> 25 kHz
*
*/

/*
*
*	For the ATmega328P using Timer1 with the input capture unit:
*
*	f = (CPU Clock Frequency) / (prescaler)
*
*	prescaler register value -> prescaler values: 1 -> 1, 2 -> 8, 3 -> 64, 4 -> 256, 5 -> 1024
*
*	For the Arduino UNO R3 the default CPU Clock Frequency = 16 MHz
*
*	*Timer sensitivity*
*	prescaler value = 8 -> MIN time between ticks = 500 ns
*
*	Timer resolution = 500 ns * 65536 = 32.768 ms
*
*	With an 8-bit overflow count variable the timer resolution can be extended to 256 overflows
*
*	*Timer extended resolution*
*	32.768 ms * 256 = 8388.608 ms
*
*/

#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "fan_driver.h"
#include "scheduler.h"
#include "config.h"
#include "pin_map.h"
#include "board.h"

#define TIM2_COUNT_REG TCNT2

#define TIM2_CTRL_REG_A TCCR2A
#define TIM2_FAST_PWM_A (1 << WGM20) | (1 << WGM21)

#define TIM2_CTRL_REG_B TCCR2B
#define TIM2_CLEAR_PIN_ON_COMP (1 << COM2B1)
#define TIM2_FAST_PWM_B (1 << WGM22)
#define TIM2_PRSC 2U

#define TIM2_TOP_REG OCR2A
#define TIM2_TOPVALUE 79U

#define TIM2_DUTY_CYCLE_REG OCR2B
#define TOP_DC_REG_VALUE TIM2_TOPVALUE
#define HALF_DC_REG_VALUE 40U
#define DC_REG_VALUE_RANGE (TOP_DC_REG_VALUE - FAN_DRIVER_MIN_DC_REG_VALUE)
/*
*
*		Smallest possible step of the duty-cycle register
*
*		-This value is the increment/decrement used in the feedback controller
*
*		-The speed for the used fan changes only for a duty-cycle register step of this size
*
*		-This value should be measured by using fan_driver_speed_test
*
*/
#define MIN_DC_REG_STEP 3U

#define TIM1_COUNT_REG TCNT1

#define TIM1_CTRL_REG_B TCCR1B
#define TIM1_PRSC 2U
#define TIM1_RISING_EDGE_TRIGG (1 << ICES1)
#define TIM1_INP_CAPT_NOISE_CANC (1 << ICNC1)

#define TIM1_INPUT_CAPTR_REG ICR1

#define TIM1_INT_MASK_REG TIMSK1
#define TIM1_INPUT_CAPTR_INT (1 << ICIE1)
#define TIM1_OVERFLOW_INT (1 << TOIE1)

#define TIM1_INT_FLAG_REG TIFR1
#define TIM1_OVERFLOW_FLAG (1 << TOV1)

#define TIM1_RANGE_TICKS 65536UL
#define TICK_PERIOD_NS 500U
#define TIM1_MAX_OVERFLOWS 256U

#define SPEED_RANGE_RPM (FAN_DRIVER_MAX_SPEED_RPM - FAN_DRIVER_MIN_SPEED_RPM)

#define US_PER_MINUTE 60000000UL
#define TACH_PULSES_PER_REV 2U

#define ACCELERATION_PER_READING_RPM_MS2 ((150UL * SPEED_RANGE_RPM * FAN_DRIVER_SPEED_UPDATE_TIME_MS) / FAN_DRIVER_MAX_SPEED_UPDATE_DELAY_MS) / 100
#define MAX_STABILITY_DEVIATION_RPM 5
#define MIN_SAFE_INTERVAL_US 458

typedef struct {
	uint8_t init_flag;
	uint8_t boot_flag;
} PWM_state;

typedef struct {
	uint8_t first_step_flag;
	uint8_t last_step_flag;
} feedback_state;

typedef struct {
	feedback_state state;
	uint16_t reference_speed_rpm;
	uint16_t speed_delta_rpm;
	uint16_t prev_speed_delta_rpm;
	uint16_t stability_t_0_ms;
} feedback_controller;

typedef struct {
	volatile uint16_t tick_count;
	volatile uint8_t overflow_count;
	volatile uint8_t overflow_capture;
	volatile uint16_t prev_tick_count;
	volatile uint8_t prev_overflow_count;
	volatile uint8_t response_flag;
} tachometer_reading;

typedef struct {
	PWM_state PWM;
	feedback_controller feedback;
	tachometer_reading tachometer;
	uint16_t target_speed_rpm;
	uint16_t measured_speed_rpm;
	uint16_t hysteresis_rpm;
} fan_controller;

static fan_controller P12MAX_controller = {0};

ISR(TIMER1_CAPT_vect) {

	/* 
	*
	*		In the ATmega328P the TIM1 CAPT interrupt has higher priority then the TIM1 OVF, therefore in the case they both happen
	*		at the same time the overflow count needs to be updated in this ISR and the TIM1 OVF flag need to be cleared
	*
	*/
	if ((TIM1_INT_FLAG_REG & TIM1_OVERFLOW_FLAG) && TIM1_INPUT_CAPTR_REG < TIM1_RANGE_TICKS / 2) {
        P12MAX_controller.tachometer.overflow_capture++;
        TIM1_INT_FLAG_REG = TIM1_OVERFLOW_FLAG;
    }
    
	P12MAX_controller.tachometer.prev_tick_count = P12MAX_controller.tachometer.tick_count;
	P12MAX_controller.tachometer.tick_count = TIM1_INPUT_CAPTR_REG;
	P12MAX_controller.tachometer.prev_overflow_count = P12MAX_controller.tachometer.overflow_count;
	P12MAX_controller.tachometer.overflow_count = P12MAX_controller.tachometer.overflow_capture;
	P12MAX_controller.tachometer.response_flag = 1;
}

ISR(TIMER1_OVF_vect) {
	P12MAX_controller.tachometer.overflow_capture++;
}

static inline void tim2_pins_init(void) {
	D0_D7_DATA_DIRECTION_REG |= FAN_PWM_DIRECTION;
}

static inline void tim2_mode_config(void) {
	TIM2_CTRL_REG_A = TIM2_CLEAR_PIN_ON_COMP | TIM2_FAST_PWM_A;
	TIM2_CTRL_REG_B = TIM2_FAST_PWM_B;
	TIM2_TOP_REG = TIM2_TOPVALUE;
	TIM2_DUTY_CYCLE_REG = HALF_DC_REG_VALUE;
	TIM2_COUNT_REG = 0;
}

static inline void tim2_boot(void) {
	TIM2_CTRL_REG_B |= TIM2_PRSC;
}

static inline void tim2_stop(void) {
	TIM2_CTRL_REG_B = 0;
	TIM2_CTRL_REG_A = 0;
	TIM2_TOP_REG = 0;
	TIM2_DUTY_CYCLE_REG = 0;
	TIM2_COUNT_REG = 0;
	D0_D7_DATA_DIRECTION_REG |= FAN_PWM_DIRECTION;
}

static inline void tim1_pins_init(void) {
	D8_D13_DATA_DIRECTION_REG &= ((uint8_t) ~FAN_TACH_DIRECTION);
	D8_D13_DATA_REG &= ((uint8_t) ~FAN_TACH_PULLUP_R);
}

static inline void tim1_mode_config(void) {
	TIM1_CTRL_REG_B = TIM1_RISING_EDGE_TRIGG | TIM1_INP_CAPT_NOISE_CANC;
	TIM1_INT_FLAG_REG = 0xFF;
	TIM1_COUNT_REG = 0;
	TIM1_INT_MASK_REG = TIM1_INPUT_CAPTR_INT | TIM1_OVERFLOW_INT;
}

static inline void tim1_boot(void) {
	TIM1_CTRL_REG_B |= TIM1_PRSC;
}

static inline void tim1_stop(void) {
	TIM1_CTRL_REG_B = 0;
	TIM1_INT_MASK_REG = 0;
	TIM1_COUNT_REG = 0;
}

static void fan_controller_init(void) {
	
	if (P12MAX_controller.PWM.init_flag) {
		return;
	}
	
	tim2_pins_init();
	tim1_pins_init();
	tim2_mode_config();
	tim1_mode_config();
		
	/*
	*
	*		Speed hysteresis to prevent continuous target speed update in fan_driver_update
	*			
	*		-A linear model is used to approximate the speed variation for every change of the duty-cycle register value
	*		MIN_DC_REG_STEP * (SPEED_RANGE_RPM / DC_REG_VALUE_RANGE)
	*
	*		-The 103% is used to account for discarded decimals
	*
	*/
	P12MAX_controller.hysteresis_rpm = (uint16_t) ((((uint32_t) MIN_DC_REG_STEP * SPEED_RANGE_RPM * 103) / DC_REG_VALUE_RANGE) / 100);
	P12MAX_controller.PWM.init_flag = 1;
}

void fan_driver_controller_boot(void) {
	
	if (!P12MAX_controller.PWM.init_flag) {
		fan_controller_init();
	}
	
	if (P12MAX_controller.PWM.boot_flag) {
		return;
	}
	
	tim2_boot();
	tim1_boot();
	P12MAX_controller.PWM.boot_flag = 1;
}

static void speed_filter(uint16_t captured_speed_rpm) {
	
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
}

fan_driver_errors fan_driver_speed_measure(uint16_t* measured_speed_rpm) {
	uint8_t response_flag = 0;
	uint16_t tick_count = 0;
	uint16_t prev_tick_count = 0;
	uint32_t interval_ticks = 0;
	uint8_t overflow_count = 0;
	uint8_t prev_overflow_count = 0;
	uint8_t interval_overflows = 0;
	uint32_t interval_us = 0;
	uint16_t captured_speed_rpm = 0;
	
	if (measured_speed_rpm == NULL) {
		return FAN_DRIVER_ERR_PARAM;
	}
	
	if (!P12MAX_controller.PWM.boot_flag) {
		fan_driver_controller_boot();
		return FAN_DRIVER_ERR_OK;
	}
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		response_flag = P12MAX_controller.tachometer.response_flag;
		tick_count = P12MAX_controller.tachometer.tick_count;
		prev_tick_count = P12MAX_controller.tachometer.prev_tick_count;
		overflow_count = P12MAX_controller.tachometer.overflow_count;
		prev_overflow_count = P12MAX_controller.tachometer.prev_overflow_count;
	}
	
	if (!response_flag) {
		return FAN_DRIVER_ERR_TACH;
	}	
	
	if (overflow_count != prev_overflow_count) {
		
		if (overflow_count > prev_overflow_count) {
			interval_overflows = overflow_count - prev_overflow_count;
			
		}
		
		else {
			interval_overflows = (uint8_t) (TIM1_MAX_OVERFLOWS - prev_overflow_count) + overflow_count;
		}
		
		interval_ticks = (TIM1_RANGE_TICKS - prev_tick_count) + tick_count;
		interval_us = ((TIM1_RANGE_TICKS * (interval_overflows - 1) + interval_ticks) * TICK_PERIOD_NS) / 1000;
	}
	
	else {
		interval_overflows = 0;
		interval_ticks = tick_count - prev_tick_count;
		interval_us = (interval_ticks * TICK_PERIOD_NS) / 1000;
	}
	
	if (!interval_us || interval_us < MIN_SAFE_INTERVAL_US) {
		return FAN_DRIVER_ERR_INTERNAL;
	}
	
	captured_speed_rpm = (uint16_t) (US_PER_MINUTE / (interval_us * TACH_PULSES_PER_REV));
	
	if (captured_speed_rpm < (FAN_DRIVER_MIN_SPEED_RPM * 90) / 100) {
		return FAN_DRIVER_ERR_TACH;
	}
	
	speed_filter(captured_speed_rpm);
	*measured_speed_rpm = P12MAX_controller.measured_speed_rpm;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		P12MAX_controller.tachometer.response_flag = 0;
	}
	
	return FAN_DRIVER_ERR_OK;
}

static void feedback_control(void) {

	if (!P12MAX_controller.feedback.state.first_step_flag) {
		P12MAX_controller.feedback.reference_speed_rpm = P12MAX_controller.measured_speed_rpm;
		P12MAX_controller.feedback.stability_t_0_ms = scheduler_timestamp_capture();
		P12MAX_controller.feedback.state.first_step_flag = 1;
	}

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

	if (P12MAX_controller.feedback.reference_speed_rpm == P12MAX_controller.target_speed_rpm) {
		return;
	}
	
	P12MAX_controller.feedback.prev_speed_delta_rpm = P12MAX_controller.feedback.speed_delta_rpm;
	
	if (P12MAX_controller.target_speed_rpm > P12MAX_controller.feedback.reference_speed_rpm) {
		P12MAX_controller.feedback.speed_delta_rpm = P12MAX_controller.target_speed_rpm - P12MAX_controller.feedback.reference_speed_rpm;
	}
	
	else {
		P12MAX_controller.feedback.speed_delta_rpm = P12MAX_controller.feedback.reference_speed_rpm - P12MAX_controller.target_speed_rpm;
	}
		
	if (P12MAX_controller.feedback.speed_delta_rpm >= P12MAX_controller.feedback.prev_speed_delta_rpm && P12MAX_controller.feedback.prev_speed_delta_rpm > 0) {
		P12MAX_controller.feedback.state.last_step_flag = 1;
		P12MAX_controller.feedback.state.first_step_flag = 0;
	}
		
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
}

void fan_driver_controller_update(uint16_t target_speed_rpm) {
	uint16_t speed_variation_rpm = 0;
	uint16_t dy = 0;
	uint16_t dx = 0;
	uint16_t ds = 0;
	uint8_t q = 0;
	
	if (!P12MAX_controller.PWM.boot_flag) {
		fan_driver_controller_boot();
	}
	
	if (target_speed_rpm >= FAN_DRIVER_MAX_SPEED_RPM) {
		target_speed_rpm = FAN_DRIVER_MAX_SPEED_RPM;
	}
	
	else if (target_speed_rpm <= FAN_DRIVER_MIN_SPEED_RPM) {
		target_speed_rpm = FAN_DRIVER_MIN_SPEED_RPM;
	}
	
	if (target_speed_rpm >= P12MAX_controller.target_speed_rpm) {
		speed_variation_rpm = target_speed_rpm - P12MAX_controller.target_speed_rpm;
	}
	
	else {
		speed_variation_rpm = P12MAX_controller.target_speed_rpm - target_speed_rpm;
	}
	
	if (speed_variation_rpm >= P12MAX_controller.hysteresis_rpm) {
		P12MAX_controller.target_speed_rpm = target_speed_rpm;
		ds = P12MAX_controller.target_speed_rpm - FAN_DRIVER_MIN_SPEED_RPM;
		dy = DC_REG_VALUE_RANGE;
		dx = SPEED_RANGE_RPM;
		q = FAN_DRIVER_MIN_DC_REG_VALUE;
		TIM2_DUTY_CYCLE_REG = (uint8_t) (((((uint32_t) ds * dy * 1000) / dx) + (q * 1000)) / 1000);
		P12MAX_controller.feedback.speed_delta_rpm = 0;
		P12MAX_controller.feedback.prev_speed_delta_rpm = 0;
		P12MAX_controller.feedback.state.first_step_flag = 0;
		P12MAX_controller.feedback.state.last_step_flag = 0;
	}
	
	else {
		
		if (!P12MAX_controller.feedback.state.last_step_flag) {
			feedback_control();
		}
	}
}

void fan_driver_controller_stop(void) {	
	tim1_stop();
	tim2_stop();
	P12MAX_controller.PWM.init_flag = 0;
	P12MAX_controller.PWM.boot_flag = 0;
	P12MAX_controller.feedback.state.first_step_flag = 0;
	P12MAX_controller.feedback.state.last_step_flag = 0;
	P12MAX_controller.feedback.reference_speed_rpm = 0;
	P12MAX_controller.feedback.speed_delta_rpm = 0;
	P12MAX_controller.feedback.prev_speed_delta_rpm = 0;
	P12MAX_controller.feedback.stability_t_0_ms = 0;
	P12MAX_controller.tachometer.tick_count = 0;
	P12MAX_controller.tachometer.overflow_count = 0;
	P12MAX_controller.tachometer.overflow_capture = 0;
	P12MAX_controller.tachometer.prev_tick_count = 0;
	P12MAX_controller.tachometer.prev_overflow_count = 0;
	P12MAX_controller.tachometer.response_flag = 0;
	P12MAX_controller.target_speed_rpm = 0;
	P12MAX_controller.measured_speed_rpm = 0;
	P12MAX_controller.hysteresis_rpm = 0;
}

/*
*
*		Functions for speed and delay testing, used only for calibration during development
*
*/
/*
uint16_t fan_driver_speed_test(void) {
	uint16_t measured_speed_rpm = 0;

	fan_driver_controller_stop();
	scheduler_timer_delay(7000);
	fan_driver_controller_boot();
	TIM2_DUTY_CYCLE_REG = 79;
	scheduler_timer_delay(10000);
	fan_driver_speed_measure(&measured_speed_rpm);
	fan_driver_controller_stop();
	return measured_speed_rpm;
}

uint16_t fan_driver_update_delay_test(void) {
	uint16_t measured_speed_rpm = 0;
	uint16_t t_0_ms = 0;
	uint16_t t_f_ms = 0;
	
	fan_driver_controller_stop();
	scheduler_timer_delay(7000);
	fan_driver_controller_boot();
	TIM2_DUTY_CYCLE_REG = 2;
	scheduler_timer_delay(10000);
	t_0_ms = scheduler_timestamp_capture();
	TIM2_DUTY_CYCLE_REG = 79;
	   
	do {
		fan_driver_speed_measure(&measured_speed_rpm);
	}
		   
	while (measured_speed_rpm < 1800);
		          
	t_f_ms = scheduler_timestamp_capture();
	fan_driver_controller_stop();
	return t_f_ms - t_0_ms;
}
*/
