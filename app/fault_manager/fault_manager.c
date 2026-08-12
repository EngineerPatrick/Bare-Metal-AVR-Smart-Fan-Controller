/**
*   
*   @file fault_manager.c
*
*   @brief Implementation for the fault_manager module.
*
*   @details Handles hardware operations for each error case.
*
*/

#include <stdint.h>
#include "ui.h"
#include "fault_manager.h"
#include "display.h"
#include "scheduler.h"
#include "config.h"
#include "pin_map.h"
#include "board.h"

static inline void led1_pin_init(void) {
	D14_D19_DATA_DIRECTION_REG |= LED1_DIRECTION;
}

static inline void led2_pin_init(void) {
	D14_D19_DATA_DIRECTION_REG |= LED2_DIRECTION;
}

static inline void led3_pin_init(void) {
	D14_D19_DATA_DIRECTION_REG |= LED3_DIRECTION;
}

static inline void buzz_pin_init(void) {
	D14_D19_DATA_DIRECTION_REG |= BUZZER_DIRECTION;
}

static inline void led1_pin_pull_up(void) {
	D14_D19_DATA_REG |= LED1_OUTPUT;
}

static inline void led2_pin_pull_up(void) {
	D14_D19_DATA_REG |= LED2_OUTPUT;
}

static inline void led3_pin_pull_up(void) {
	D14_D19_DATA_REG |= LED3_OUTPUT;
}

static inline void buzz_pin_pull_up(void) {
	D14_D19_DATA_REG |= BUZZER_OUTPUT;
}

static inline void buzz_pin_pull_down(void) {
	D14_D19_DATA_REG &= ((uint8_t) ~BUZZER_OUTPUT);
}

static void buzzer_activate(uint16_t duration_s) {
	uint16_t running_time_ms = 0;
	uint16_t duration_ms = duration_s * 1000;
	
	if (duration_ms) {
	
		while (running_time_ms < duration_ms) {
			buzz_pin_pull_up();
			scheduler_timer_delay(1);
			buzz_pin_pull_down();
			scheduler_timer_delay(1);
			running_time_ms += 2;
		}
	}
}

fault_manager_errors fault_manager_ui_report(ui_errors ui_error_code) {
	display_errors display_error_code = 0;
	led1_pin_init();
	led2_pin_init();
	led3_pin_init();
	buzz_pin_init();
	
	if (ui_error_code > UI_ERR_FAN_DRIVER) {
		return FAULT_MANAGER_ERR_PARAM;
	}
	
	switch (ui_error_code) {
		
		case UI_ERR_DISPLAY:
			
			display_error_code = display_error_write(SYS_ERR_DISPLAY);
			led1_pin_pull_up();
			break;
		
		case UI_ERR_FAN_CURVE:
			
			display_error_code = display_error_write(SYS_ERR_FAN_CURVE);
			led2_pin_pull_up();
			break;
		
		case UI_ERR_BME:
			
			display_error_code = display_error_write(SYS_ERR_BME);
			led3_pin_pull_up();
			break;
		
		case UI_ERR_FAN_DRIVER:
			
			display_error_code = display_error_write(SYS_ERR_FAN_DRIVER);
			buzzer_activate(FAULT_MANAGER_BUZZER_DURATION_S);
			break;
		
		default:
			display_error_code = display_error_write(SYS_ERR_OK);
	}
	
	return (!display_error_code) ? FAULT_MANAGER_ERR_OK : FAULT_MANAGER_ERR_DISPLAY;
}
