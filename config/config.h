/**
*
*	@file config.h
*
*	@brief System configuration storage.
*
*	@details Configuration values for all modules of the system.
*
*	If incorrect changes are made the system will return a
*	specific error code.
*
*/

#ifndef CONFIG_H
#define CONFIG_H

/**TWI*/
#define TWI_BITRATE_REG 72
#define TWI_BITRATE_PRESCALER 0
#define TWI_TIMEOUT_MS 400

/**UI*/
#define UI_STD_CURVE_SIZE 8
#define UI_STD_TEMP_1 200
#define UI_STD_TEMP_2 300
#define UI_STD_TEMP_3 400
#define UI_STD_TEMP_4 500
#define UI_STD_TEMP_5 600
#define UI_STD_TEMP_6 700
#define UI_STD_TEMP_7 800
#define UI_STD_TEMP_8 900
#define UI_STD_TEMP_9 000
#define UI_STD_TEMP_10 000

#define BUTTONS_DEBOUNCE_TIME_MS 250
#define BLINK_TIME_MS 400

/**DISPLAY*/
#define FIRST_ROW 3
#define FIRST_COLUMN 3

#define SELECT_WORD_ROW FIRST_ROW
#define SELECT_WORD_COLUMN FIRST_COLUMN
#define MODE_WORD_ROW SELECT_WORD_ROW
#define MODE_WORD_COLUMN SELECT_WORD_COLUMN + 7
#define STANDARD_WORD_ROW FIRST_ROW + 3
#define STANDARD_WORD_COLUMN FIRST_COLUMN
#define ADVANCED_WORD_ROW FIRST_ROW + 5
#define ADVANCED_WORD_COLUMN FIRST_COLUMN

#define NODE_WORD_ROW FIRST_ROW - 2
#define NODE_WORD_COLUMN FIRST_COLUMN
#define NODE_DIGITS_ROW NODE_WORD_ROW
#define NODE_DIGITS_COLUMN NODE_WORD_COLUMN + 6

#define TEMP_WORD_ROW FIRST_ROW
#define TEMP_WORD_COLUMN FIRST_COLUMN
#define TEMP_DIGITS_ROW TEMP_WORD_ROW
#define TEMP_DIGITS_COLUMN TEMP_WORD_COLUMN + 8

#define SPEED_WORD_ROW FIRST_ROW + 3
#define SPEED_WORD_COLUMN FIRST_COLUMN
#define SPEED_DIGITS_ROW SPEED_WORD_ROW
#define SPEED_DIGITS_COLUMN SPEED_WORD_COLUMN + 7

#define ERR_WORD_ROW FIRST_ROW + 1
#define ERR_WORD_COLUMN FIRST_COLUMN
#define ERR_DIGITS_ROW ERR_WORD_ROW
#define ERR_DIGITS_COLUMN ERR_WORD_COLUMN + 5

/**BME280*/
//The manufacturer provides maximum range from -40°C to 85°C and ideal range from 0°C to 65°C
//Value format: 12.3... = 123
#define BME280_MIN_TEMP_C_X10 0
#define BME280_MAX_TEMP_C_X10 650

/**FAN_DRIVER*/
#define FAN_DRIVER_TIMEOUT_MS 400
#define FAN_DRIVER_UPDATE_TIME_MS 80

#define TACH_PULSES_PER_REV 2

/*FAULT_MANAGER*/
#define BUZZER_DURATION_S 20

#endif
