/**
*
*	@file fault_manager.h
*
*	@brief Public API for the fault_manager module.
*
*	@details Module to report system-wide errors using 3
*	LEDs, 1 buzzer and 1 display.
*
*/

#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#include <stdint.h>
#include "ui.h"

typedef enum {
	FAULT_MANAGER_ERR_OK = 0,
	FAULT_MANAGER_ERR_PARAM,
	FAULT_MANAGER_ERR_DISPLAY
} fault_manager_errors;

/**
*
*	@brief	Reports the UI module error codes.
*
*	@param	ui_error_code					Error code from the UI module
*
*	@retval	FAULT_MANAGER_ERR_OK				If no error occurs
*	@retval	FAULT_MANAGER_ERR_PARAM				If ui_error_code > UI_ERR_FAN_DRIVER
*	@retval	FAULT_MANAGER_ERR_DISPLAY			If an error with the display occurs
*
*	@pre 	ui_error_code <= UI_ERR_FAN_DRIVER
*	@pre 	The scheduler must be activated by calling scheduler_timer_boot
*	@post 	If scheduler_timer_boot has not been called the correct execution is not guaranteed
*	@post 	If passed parameter is invalid no LED or buzzer is activated and the display is left unchanged
*	@post 	On success the display prints the error code and the corresponding LED or buzzer is activated
*
*/
fault_manager_errors fault_manager_ui_report(ui_errors ui_error_code);

#endif
