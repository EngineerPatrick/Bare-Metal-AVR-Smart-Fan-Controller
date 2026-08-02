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
	F_MAN_ERR_OK = 0,
	F_MAN_ERR_PARAM,
	F_MAN_ERR_DISPLAY
} f_man_errors;

/**
*
*	@brief	Handles the UI module error codes.
*
*	@param	ui_error_code			Error code from the UI module
*
*	@retval	F_MAN_ERR_OK 			If no error occurs
*	@retval	F_MAN_ERR_PARAM			If ui_error_code > UI_ERR_FAN_DRIVER
*	@retval	F_MAN_ERR_DISPLAY		If an error with the display occurs
*
*	@pre 	ui_error_code <= UI_ERR_FAN_DRIVER
*	@post 	If passed parameter is invalid no LED or buzzer is activated and the display is left unchanged
*	@post 	On success the display prints the error code and the corresponding LED or buzzer is activated
*
*/
f_man_errors fault_manager_ui(ui_errors ui_error_code);

#endif
