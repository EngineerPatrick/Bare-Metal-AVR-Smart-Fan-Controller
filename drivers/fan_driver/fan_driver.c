/**
*   
*   @file fan_driver.c
*
*   @brief Implementation for the fan_driver module.
*
*   @details Contains the PWM logic and the 
*	fan tachometer feedback controller.
*
*	Integrates the util/atomic library of avr-libc to
*	read ISR shared volatile variables atomically.
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
*	*Required resolution*
*	Smallest change of time between pulses = 2.756 us
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
*	*Frequency*
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
*	*Timer resolution*
*	prescaler value = 8 -> MIN time between ticks = 500 ns
*
*/

#include <stdint.h>
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
#define DC_REG_VALUE_RANGE (TOP_DC_REG_VALUE - MIN_DC_REG_VALUE)

#define TIM1_COUNT_REG TCNT1

#define TIM1_CTRL_REG_B TCCR1B
#define TIM1_PRSC 2U
#define TIM1_RISING_EDGE_TRIGG (1 << ICES1)

#define TIM1_INPUT_CAPTR_REG ICR1

#define TIM1_INT_MASK_REG TIMSK1
#define TIM1_INPUT_CAPTR_INT (1 << ICIE1)
#define TIM1_OVERFLOW_INT (1 << TOIE1)

#define TIM1_INT_FLAG_REG TIFR1
#define TIM1_OVERFLOW_FLAG (1 << TOV1)

#define TIM1_RANGE_TICKS 65536U
#define TICK_PERIOD_NS 500U
#define TIM1_MAX_OVERFLOWS 65536U

#define FAN_DRIVER_SPEED_RANGE (FAN_DRIVER_MAX_SPEED_RPM - FAN_DRIVER_MIN_SPEED_RPM)

#define TACH_PULSES_PER_REV 2U

typedef struct {
	uint16_t measured_speed_rpm;
	uint16_t target_speed_rpm;
	uint16_t hysteresis_rpm;
	uint8_t prev_dc_reg_value;
	uint8_t init_flag;
} fan_controller;

typedef struct {
	uint32_t interval_us;
	uint32_t interval_ticks;
	uint32_t interval_overflows;
	uint16_t prev_interval_ticks;
	uint16_t prev_interval_overflows;
	uint16_t overflow_count;
	uint8_t response_received;
} tach_reading;

static fan_controller fan = {0};

static volatile tach_reading tach = {0};

ISR(TIMER1_CAPT_vect) {
	uint16_t local_ovf_count = tach.overflow_count;
	tach.response_received = 1;

	/* 
	*
	*		In the ATmega328P the TIM1 CAPT interrupt has higher priority then the TIM1 OVF, therefore in the case they both happen
	*		at the same time the overflow count needs to be updated in this ISR
	*
	*/
	if ((TIM1_INT_FLAG_REG & TIM1_OVERFLOW_FLAG) && TIM1_INPUT_CAPTR_REG < TIM1_RANGE_TICKS / 2) {
        local_ovf_count++;
    }
	
	if (local_ovf_count != tach.prev_interval_overflows) {
		tach.interval_ticks = (uint32_t) TIM1_RANGE_TICKS - tach.prev_interval_ticks + TIM1_INPUT_CAPTR_REG;

		if (local_ovf_count > tach.prev_interval_overflows) {
			tach.interval_overflows = local_ovf_count - tach.prev_interval_overflows - 1;
		}

		else {
			tach.interval_overflows = (uint32_t) TIM1_MAX_OVERFLOWS - tach.prev_interval_overflows + local_ovf_count - 1;
		}
	}

	else {
		tach.interval_ticks = (uint32_t) TIM1_INPUT_CAPTR_REG - tach.prev_interval_ticks;
		tach.interval_overflows = 0;
	}
	
	tach.interval_us = ((tach.interval_ticks + ((uint32_t) tach.interval_overflows * TIM1_RANGE_TICKS)) * TICK_PERIOD_NS) / 1000;
	tach.prev_interval_ticks = TIM1_INPUT_CAPTR_REG;
	tach.prev_interval_overflows = local_ovf_count;
}

ISR(TIMER1_OVF_vect) {
	tach.overflow_count++;
}

static inline void fan_driver_pins_init(void) {
	D0_D7_DATA_DIRECTION_REG |= FAN_PWM_DIRECTION;
	D8_D13_DATA_DIRECTION_REG &= ((uint8_t) ~FAN_TACH_DIRECTION);
	D8_D13_DATA_REG &= ((uint8_t) ~FAN_TACH_PULLUP_R);
}

static inline void fan_driver_tim2_fastpwm_init(void) {
	TIM2_CTRL_REG_A = TIM2_CLEAR_PIN_ON_COMP | TIM2_FAST_PWM_A;
	TIM2_CTRL_REG_B = TIM2_FAST_PWM_B;
	TIM2_TOP_REG = TIM2_TOPVALUE;
	TIM2_DUTY_CYCLE_REG = HALF_DC_REG_VALUE;
}

static inline void fan_driver_tim2_prsc_init(void) {
	TIM2_CTRL_REG_B |= TIM2_PRSC;
}

static inline void fan_driver_tim2_stop(void) {
	TIM2_CTRL_REG_B = 0;
	TIM2_CTRL_REG_A = 0;
}

static inline void fan_driver_tim1_inp_captr_init(void) {
	TIM1_CTRL_REG_B = TIM1_RISING_EDGE_TRIGG;
	TIM1_INT_MASK_REG = TIM1_INPUT_CAPTR_INT | TIM1_OVERFLOW_INT;
}

static inline void fan_driver_tim1_prsc_init(void) {
	TIM1_CTRL_REG_B |= TIM1_PRSC;
}

static inline void fan_driver_tim1_stop(void) {
	TIM1_CTRL_REG_B = 0;
	TIM1_INT_MASK_REG= 0;
}

void fan_driver_boot(void) {
	cli();
	fan_driver_pins_init();
	fan_driver_tim2_fastpwm_init();
	TIM2_COUNT_REG = 0;
	fan_driver_tim2_prsc_init();
	
	/*
	*
	*		Speed hysteresis to prevent continuous target speed update in fan_driver_update
	*			
	*		-If TIM2_DUTY_CYCLE_REG changes by MIN_DC_REG_STEP, the speed is estimated to change of
	*		MIN_DC_REG_STEP *(FAN_DRIVER_SPEED_RANGE / DC_REG_VALUE_RANGE)
	*
	*		-HYST_CORR_FACT_X1000 is used to account for discarded decimals and non-linear speed
	*
	*/
	fan.hysteresis_rpm = (uint16_t) ((((uint32_t) MIN_DC_REG_STEP * FAN_DRIVER_SPEED_RANGE * HYST_CORR_FACT_X1000 * 1000) / DC_REG_VALUE_RANGE) / 1000);
	
	fan_driver_tim1_inp_captr_init();
	TIM1_COUNT_REG = 0;
	fan_driver_tim1_prsc_init();
	fan.init_flag = 1;
	sei();
}

static fan_driver_errors fan_driver_tach(void) {
	uint32_t interval_us = 0;
	uint16_t t_zero_ms = 0;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		tach.response_received = 0;
	}
	
	t_zero_ms = scheduler_timer_get_timestamp();
		
	while (!scheduler_timer_poll(&t_zero_ms, FAN_DRIVER_TIMEOUT_MS)) {
			
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		
			if (tach.response_received) {
				break;
			}
		}
	}
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		
		if (!tach.response_received) {
			return FAN_DRIVER_ERR_TACH;
		}
		
		else {
			interval_us = tach.interval_us;
		}
	}
	
	fan.measured_speed_rpm = (uint16_t) (((uint32_t) 1000000 * 60) / (interval_us * TACH_PULSES_PER_REV));

	if (fan.measured_speed_rpm == fan.target_speed_rpm) {
		return FAN_DRIVER_ERR_OK;
	}

	else if (fan.measured_speed_rpm < fan.target_speed_rpm - fan.hysteresis_rpm) {

		if ((TIM2_DUTY_CYCLE_REG + MIN_DC_REG_STEP <= TOP_DC_REG_VALUE) && (TIM2_DUTY_CYCLE_REG + MIN_DC_REG_STEP != fan.prev_dc_reg_value)) {
			fan.prev_dc_reg_value = TIM2_DUTY_CYCLE_REG;
			TIM2_DUTY_CYCLE_REG += MIN_DC_REG_STEP;
		}
	}
	
	else if (fan.measured_speed_rpm > fan.target_speed_rpm + fan.hysteresis_rpm) {

		if ((TIM2_DUTY_CYCLE_REG - MIN_DC_REG_VALUE >= MIN_DC_REG_STEP) && (TIM2_DUTY_CYCLE_REG - MIN_DC_REG_STEP != fan.prev_dc_reg_value)) {
			fan.prev_dc_reg_value = TIM2_DUTY_CYCLE_REG;
			TIM2_DUTY_CYCLE_REG = (uint8_t) (TIM2_DUTY_CYCLE_REG - MIN_DC_REG_STEP);
		}
	}
	
	return FAN_DRIVER_ERR_OK;
}

fan_driver_errors fan_driver_update(uint16_t target_speed_rpm, uint16_t* measured_speed_rpm) {
	fan_driver_errors error_code = FAN_DRIVER_ERR_OK;
	uint16_t speed_variation_rpm = 0;
	
	if (!fan.init_flag) {
		fan_driver_boot();
	}

	if (fan.target_speed_rpm != target_speed_rpm) {
		
		if (target_speed_rpm > fan.target_speed_rpm) {
			speed_variation_rpm = target_speed_rpm - fan.target_speed_rpm;
			
			if (speed_variation_rpm >= fan.hysteresis_rpm) {
				fan.target_speed_rpm = (target_speed_rpm >= FAN_DRIVER_MAX_SPEED_RPM) ? FAN_DRIVER_MAX_SPEED_RPM : target_speed_rpm;
				TIM2_DUTY_CYCLE_REG = (uint8_t) (((((uint32_t) (fan.target_speed_rpm - FAN_DRIVER_MIN_SPEED_RPM) * DC_REG_VALUE_RANGE * 1000) / FAN_DRIVER_SPEED_RANGE) / 1000) + MIN_DC_REG_VALUE);
			}
		}		

		else {
			speed_variation_rpm = fan.target_speed_rpm - target_speed_rpm;

			if (speed_variation_rpm >= fan.hysteresis_rpm) {
				fan.target_speed_rpm = (target_speed_rpm <= FAN_DRIVER_MIN_SPEED_RPM) ? FAN_DRIVER_MIN_SPEED_RPM : target_speed_rpm;
				TIM2_DUTY_CYCLE_REG = (uint8_t) (((((uint32_t) (fan.target_speed_rpm - FAN_DRIVER_MIN_SPEED_RPM) * DC_REG_VALUE_RANGE * 1000) / FAN_DRIVER_SPEED_RANGE) / 1000) + MIN_DC_REG_VALUE);
			}
		}
	}
	
	else {
		error_code = fan_driver_tach();
	}
	
	*measured_speed_rpm = fan.measured_speed_rpm;
	return error_code;
}

void fan_driver_stop(void) {	
	fan_driver_tim1_stop();
	fan_driver_tim2_stop();
	tach.interval_us = 0;
	tach.interval_ticks = 0;
	tach.interval_overflows = 0;
	tach.prev_interval_ticks = 0;
	tach.prev_interval_overflows = 0;
	tach.overflow_count = 0;
	tach.response_received = 0;
	fan.measured_speed_rpm = 0;
	fan.target_speed_rpm = 0;
	fan.hysteresis_rpm = 0;
	fan.prev_dc_reg_value = 0;
	fan.init_flag = 0;
}
