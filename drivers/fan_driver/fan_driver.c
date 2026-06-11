/**
*   
*   @file fan_driver.c
*
*   @brief Implementation for the fan_driver module.
*
*   @details Contains the PWM control logic and the 
*	fan tachometer interrupt-based measurement.
*
*	Integrates the util/atomic library of avr-libc to
*	read interrupt-sensible variables atomically.
*
*/

/*
*
*	For the Arctic P12 MAX the tachometer sends 2 pulses per revolution:
*
*	MIN - MAX SPEED = 400RPM - 3300RPM
*
*	3299RPM - 3300RPM pulses/s = 109.97Hz - 110Hz
*
*	3299RPM - 3300RPM time between pulses = 9.093665ms - 9.090909ms
*	
*	*Required resolution*
*	Smallest change of time between pulses = 2.756us
*
*/

/*
*
*	For the ATmega328P using 8-bit timer 2 in Fast PWM mode:
*
*	f = (CPU Clock Frequency) / (prescaler * (1 + top))
*
*	0 <= top value <= 255 (8-bit register)
*
*	prescaler register value -> prescaler value: 1 -> 1, 2 -> 8, 3 -> 32, 4 -> 64, 5 -> 128, 6 -> 256, 7 -> 1024
*
*	For the Arduino UNO R3 the default CPU Clock Frequency = 16MHz
*
*	*Frequency*
*	prescaler value = 8, top value = 79 -> 25kHz
*
*/

/*
*
*	For the ATmega328P using 16-bit timer 1 with the Input Capture Unit:
*
*	f = (CPU Clock Frequency) / (prescaler)
*
*	prescaler register value -> prescaler values: 1 -> 1, 2 -> 8, 3 -> 64, 4 -> 256, 5 -> 1024
*
*	For the Arduino UNO R3 the default CPU Clock Frequency = 16MHz
*
*	*Timer resolution*
*	prescaler value = 8 -> MIN time between ticks 500ns
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
#define TIM2_PRSC 2

#define TIM2_TOP_REG OCR2A
#define TIM2_TOPVALUE 79

#define TIM2_DUTY_CYCLE_REG OCR2B
#define MIN_DC_REG_VALUE 2
/*
*
*		-Increase MIN_DC_REG_STEP to make the PWM adjust faster to the target speed.
*
*		-Decrease MIN_DC_REG_STEP to make the PWM more precise and responsive to changes of the target speed.
*
*/
#define MIN_DC_REG_STEP 1

#define TIM1_COUNT_REG TCNT1

#define TIM1_CTRL_REG_B TCCR1B
#define TIM1_PRSC 2
#define TIM1_RISING_EDGE_TRIGG (1 << ICES1)

#define TIM1_INPUT_CAPTR_REG ICR1

#define TIM1_INT_MASK_REG TIMSK1
#define TIM1_INPUT_CAPTR_INT (1 << ICIE1)
#define TIM1_OVERFLOW_INT (1 << TOIE1)

#define TIM1_INT_FLAG_REG TIFR1
#define TIM1_OVERFLOW_FLAG (1 << TOV1)

#define TIM1_RANGE_TICKS 65536
#define TICK_PERIOD_NS 500
#define TIM1_MAX_OVERFLOWS 65536

typedef struct {
	uint32_t tach_us;
	uint32_t interval_ticks;
	uint32_t interval_overflows;
	uint16_t prev_ticks;
	uint16_t prev_overflows;
	uint16_t overflow_count;
	uint16_t measured_speed_rpm;
	uint16_t target_speed_rpm;
	uint16_t min_pwm_step_rpm;
	uint8_t init_flag;
} pwm_params;

static volatile pwm_params pwm = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

ISR(TIMER1_CAPT_vect) {
	uint16_t local_ovf_count = pwm.overflow_count;

	/* 
	*
	*	-In the ATmega328P the TIM1 CAPT interrupt has higher priority then the TIM1 OVF, therefore in the case they both happen
	*	at the same time the overflow count needs to be updated in this ISR
	*
	*/
	if ((TIM1_INT_FLAG_REG & TIM1_OVERFLOW_FLAG) && TIM1_INPUT_CAPTR_REG < TIM1_RANGE_TICKS / 2) {
        local_ovf_count++;
    }
	
	if (local_ovf_count != pwm.prev_overflows) {
		pwm.interval_ticks = (uint32_t) TIM1_RANGE_TICKS - pwm.prev_ticks + TIM1_INPUT_CAPTR_REG;

		if (local_ovf_count > pwm.prev_overflows) {
			pwm.interval_overflows = local_ovf_count - 1 - pwm.prev_overflows;
		}

		else {
			pwm.interval_overflows = (uint32_t) TIM1_MAX_OVERFLOWS - pwm.prev_overflows + local_ovf_count - 1;
		}
	}

	else {
		pwm.interval_ticks = (uint32_t) TIM1_INPUT_CAPTR_REG - pwm.prev_ticks;
		pwm.interval_overflows = 0;
	}
	
	pwm.tach_us = ((pwm.interval_ticks + ((uint32_t) pwm.interval_overflows * TIM1_RANGE_TICKS)) * TICK_PERIOD_NS) / 1000;
	pwm.prev_ticks = TIM1_INPUT_CAPTR_REG;
	pwm.prev_overflows = local_ovf_count;
}

ISR(TIMER1_OVF_vect) {
	pwm.overflow_count++;
}

static inline void fan_driver_pins_init(void) {
	D0_D7_DATA_DIRECTION_REG |= FAN_PWM_DIRECTION;
	D8_D13_DATA_DIRECTION_REG &= ~FAN_TACH_DIRECTION;
	D8_D13_DATA_REG &= ~FAN_TACH_PULLUP_R;
}

static inline void fan_driver_tim2_fastpwm_init(void) {
	TIM2_CTRL_REG_A = TIM2_CLEAR_PIN_ON_COMP | TIM2_FAST_PWM_A;
	TIM2_CTRL_REG_B = TIM2_FAST_PWM_B;
	TIM2_TOP_REG = TIM2_TOPVALUE;
	TIM2_DUTY_CYCLE_REG = MIN_DC_REG_VALUE;
}

static inline void fan_driver_tim2_prsc_init(void) {
	TIM2_CTRL_REG_B |= TIM2_PRSC;
}

static inline void fan_driver_tim2_stop(void) {
	TIM2_CTRL_REG_B = 0;
	TIM2_CTRL_REG_A = 0;
}

static inline void fan_driver_tim1_incaptr_init(void) {
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
	*		-To prevent continuous target speed update in fan_driver_update, and to prevent continuous speed oscillations in fan_driver_tach.
	*			
	*		-If TIM2_DUTY_CYCLE_REG changes by MIN_DC_REG_STEP the speed would change of (FAN_DRIVER_MAX_SPEED_RPM - FAN_DRIVER_MIN_SPEED_RPM) * (MIN_DC_REG_STEP / (TIM2_TOP_REG + 1)),
	*		1500 is used instead of 1000 to account for discarded decimals.
	*
	*/
	pwm.min_pwm_step_rpm = (uint16_t) (((uint32_t) (FAN_DRIVER_MAX_SPEED_RPM - FAN_DRIVER_MIN_SPEED_RPM) * MIN_DC_REG_STEP * 1500 / (TIM2_TOP_REG + 1)) / 1000);
	
	fan_driver_tim1_incaptr_init();
	TIM1_COUNT_REG = 0;
	fan_driver_tim1_prsc_init();
	pwm.init_flag = 1;
	sei();
}

static fan_driver_errors fan_driver_tach(void) {
	uint32_t tach_us = 0;
	uint32_t prev_tach_us = 0;
	uint8_t response_received = 0;
	uint16_t t_zero_ms = 0;
	uint16_t speed_deviation_rpm = 0;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		prev_tach_us = pwm.tach_us;
	}
	
	t_zero_ms = scheduler_timer_get_timestamp();
		
	while (!scheduler_timer_poll(&t_zero_ms, FAN_DRIVER_TIMEOUT_MS)) {
			
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			tach_us = pwm.tach_us;
		}
			
		if (tach_us != prev_tach_us) {
			response_received = 1;
			break;
		}
	}
	
	if (!response_received) {
		return FAN_DRIVER_ERR_TACH;
	}
	
	pwm.measured_speed_rpm = (uint16_t) (((uint32_t) 1000000 * 60) / (tach_us * TACH_PULSES_PER_REV));

	if (pwm.measured_speed_rpm == pwm.target_speed_rpm) {
		return FAN_DRIVER_ERR_OK;
	}

	else if (pwm.measured_speed_rpm < pwm.target_speed_rpm) {
		speed_deviation_rpm = pwm.target_speed_rpm - pwm.measured_speed_rpm;

		if (speed_deviation_rpm >= pwm.min_pwm_step_rpm && (TIM2_DUTY_CYCLE_REG + MIN_DC_REG_STEP <= TIM2_TOP_REG)) {
			TIM2_DUTY_CYCLE_REG += MIN_DC_REG_STEP;
		}
	}
	
	else {
		speed_deviation_rpm = pwm.measured_speed_rpm - pwm.target_speed_rpm;

		if (speed_deviation_rpm >= pwm.min_pwm_step_rpm && (TIM2_DUTY_CYCLE_REG >= MIN_DC_REG_VALUE + MIN_DC_REG_STEP)) {
			TIM2_DUTY_CYCLE_REG -= MIN_DC_REG_STEP;
		}
	}
	
	return FAN_DRIVER_ERR_OK;
}

fan_driver_errors fan_driver_update(uint16_t target_speed_rpm) {
	fan_driver_errors error_code = FAN_DRIVER_ERR_OK;
	uint16_t speed_variation_rpm = 0;
	
	if (!pwm.init_flag) {
		fan_driver_boot();
	}

	if (pwm.target_speed_rpm != target_speed_rpm) {
		
		if (target_speed_rpm > pwm.target_speed_rpm) {
			speed_variation_rpm = target_speed_rpm - pwm.target_speed_rpm;
			
			if (speed_variation_rpm >= pwm.min_pwm_step_rpm) {
				pwm.target_speed_rpm = (target_speed_rpm >= FAN_DRIVER_MAX_SPEED_RPM) ? FAN_DRIVER_MAX_SPEED_RPM : target_speed_rpm;
			}
		}		

		else {
			speed_variation_rpm = pwm.target_speed_rpm - target_speed_rpm;

			if (speed_variation_rpm >= pwm.min_pwm_step_rpm) {
				pwm.target_speed_rpm = (target_speed_rpm <= FAN_DRIVER_MIN_SPEED_RPM) ? FAN_DRIVER_MIN_SPEED_RPM : target_speed_rpm;
			}
		}
		
		TIM2_DUTY_CYCLE_REG = (uint8_t) (((uint32_t) TIM2_TOP_REG * pwm.target_speed_rpm * 1000) / ((uint32_t) FAN_DRIVER_MAX_SPEED_RPM * 1000));
	}
	
	else {
		error_code = fan_driver_tach();
	}
	
	return error_code;
}

void fan_driver_stop(void) {	
	fan_driver_tim1_stop();
	fan_driver_tim2_stop();
	pwm.tach_us = 0;
	pwm.interval_ticks = 0;
	pwm.interval_overflows = 0;
	pwm.prev_ticks = 0;
	pwm.prev_overflows = 0;
	pwm.overflow_count = 0;
	pwm.measured_speed_rpm = 0;
	pwm.target_speed_rpm = 0;
	pwm.min_pwm_step_rpm = 0;
	pwm.init_flag = 0;
}
