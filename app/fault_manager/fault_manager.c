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

static inline void fault_manager_led1_init(void) {
	D14_D19_DATA_DIRECTION_REG |= LED1_DIRECTION;
}

static inline void fault_manager_led2_init(void) {
	D14_D19_DATA_DIRECTION_REG |= LED2_DIRECTION;
}

static inline void fault_manager_led3_init(void) {
	D14_D19_DATA_DIRECTION_REG |= LED3_DIRECTION;
}

static inline void fault_manager_buzz_init(void) {
	D14_D19_DATA_DIRECTION_REG |= BUZZER_DIRECTION;
}

static inline void fault_manager_led1_pullup(void) {
	D14_D19_DATA_REG |= LED1_OUTPUT;
}

static inline void fault_manager_led2_pullup(void) {
	D14_D19_DATA_REG |= LED2_OUTPUT;
}

static inline void fault_manager_led3_pullup(void) {
	D14_D19_DATA_REG |= LED3_OUTPUT;
}

static inline void fault_manager_buzz_pullup(void) {
	D14_D19_DATA_REG |= BUZZER_OUTPUT;
}

static inline void fault_manager_buzz_pulldown(void) {
	D14_D19_DATA_REG &= ~BUZZER_OUTPUT;
}

static void fault_manager_buzzer(uint16_t duration_s) {
	uint16_t running_time_ms = 0;
	uint16_t duration_ms = duration_s * 1000;
	
	if (duration_ms) {
	
		while (running_time_ms < duration_ms) {
			fault_manager_buzz_pullup();
			scheduler_timer_delay(1);
			fault_manager_buzz_pulldown();
			scheduler_timer_delay(1);
			running_time_ms += 2;
		}
	}
}

f_man_errors fault_manager_ui(ui_errors ui_error_code) {
	display_errors display_error_code = 0;
	fault_manager_led1_init();
	fault_manager_led2_init();
	fault_manager_led3_init();
	fault_manager_buzz_init();
	
	if (ui_error_code > UI_ERR_FAN_DRIVER) {
		return F_MAN_ERR_PARAM;
	}
	
	switch (ui_error_code) {
		
		case UI_ERR_DISPLAY:
			
			display_error_code = display_error_write(SYS_ERR_DISPLAY);
			fault_manager_led1_pullup();
			break;
		
		case UI_ERR_FAN_CURVE:
			
			display_error_code = display_error_write(SYS_ERR_FAN_CURVE);
			fault_manager_led2_pullup();
			break;
		
		case UI_ERR_BME:
			
			display_error_code = display_error_write(SYS_ERR_BME);
			fault_manager_led3_pullup();
			break;
		
		case UI_ERR_FAN_DRIVER:
			
			display_error_code = display_error_write(SYS_ERR_FAN_DRIVER);
			fault_manager_buzzer(BUZZER_DURATION_S);
			break;
		
		default:
			display_error_code = display_error_write(SYS_ERR_OK);
	}
	
	return (!display_error_code) ? F_MAN_ERR_OK : F_MAN_ERR_DISPLAY;
}
