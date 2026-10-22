/**
*
*	@file config.h
*
*	@brief System configuration storage.
*
*	@details Configuration values for all modules of the system.
*
*	If parameters are changed to incorrect values a specific error
*	will be generated during compile-time.
*
*/

#ifndef CONFIG_H
#define CONFIG_H

/**TWI*/
/*
*
*		Bitrate and prescaler values for 100 kHz TWI/I2C communication for the
*		ATmega328P/Arduino UNO R3 (see the frequency formula in twi.h)
*
*/
#define TWI_BITRATE_REG 72
#define TWI_BITRATE_PRESCALER 0
#define TWI_TIMEOUT_MS 400

/**UI*/
/*
*
*		Fixed temperature values for the fan curve in the standard option
*
*		-The valid range for the curve size is [0, 10]
*
*		-The valid range for the temperatures is [BME280_MIN_TEMP_C_X10, BME280_MAX_TEMP_C_X10] (see below)
*
*/
#define UI_STD_CURVE_SIZE 8
#define UI_STD_TEMP_1 200
#define UI_STD_TEMP_2 250
#define UI_STD_TEMP_3 300
#define UI_STD_TEMP_4 350
#define UI_STD_TEMP_5 400
#define UI_STD_TEMP_6 500
#define UI_STD_TEMP_7 600
#define UI_STD_TEMP_8 650
#define UI_STD_TEMP_9 000
#define UI_STD_TEMP_10 000

#define BUTTONS_DEBOUNCE_TIME_MS 250
#define BLINK_TIME_MS 400

/**DISPLAY*/
/*
*
*		Position of every word and digit	 on the display
*
*		-Every position must be set so that every character fits within an 8x16 grid
*
*/
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
/*
*
*		The manufacturer provides the maximum range [-40, 85]°C and the ideal range [0, 65]°C
*
*		-The value format here is tenth of Celsius degrees multiplied by 10: 12.3°C = 123°C
*
*		-The valid range is [0, 85]°C
*
*/
#define BME280_MIN_TEMP_C_X10 0
#define BME280_MAX_TEMP_C_X10 650

/**FAN_DRIVER*/
/*
*
*		Minimum duty-cycle register step
*
*		-Increase MIN_DC_REG_STEP to increase the speed variation during the feedback adjustment,
*		which will finish faster but will be less smooth
*
*		-Decrease MIN_DC_REG_STEP to improve the responsiveness and the approximation of the target speed
*		during the feedback adjustment, which will be slower
*
*		-The valid range is [1, 10]
*
*/
#define MIN_DC_REG_STEP 1
/*
*
*		Hysteresis correction factor to account for rounded decimals and non-liner speed
*
*		-The valid range is [15, 50]
*
*/
#define HYST_CORR_FACT_X1000 15
/*
*
*		Minimum duty-cycle register value to spin the fan tested with a 9 V DC supply
*
*		-The valid range is [0, 40] (half range: the reg top value is 79)
*
*/
#define MIN_DC_REG_VALUE 2
/*
*	
*	  	Approximate MIN-MAX fan speed values for a 9 V DC supply calculated from the tachometer
*	  	pulses measured using the input capture feature
*
*	 	-Tested duty-cycle range is [3.75%, 100%] (2 to 79 duty_cycle reg)
*
*		-The valid range must be bigger than 80 with boundaries within [0, 9999]
*
*		-Keep this range smaller than the real measured one to guarantee the reaching of the target speed
*
*/
#define FAN_DRIVER_MIN_SPEED_RPM 350
#define FAN_DRIVER_MAX_SPEED_RPM 2000

#define FAN_DRIVER_BOOT_DELAY_MS 1000
#define FAN_DRIVER_TIMEOUT_MS 400
#define FAN_DRIVER_UPDATE_TIME_MS 80

/*FAULT_MANAGER*/
#define BUZZER_DURATION_S 20

#endif


/*
*
*		Control logic to generate compile-time errors for incorrect values
*
*/	

/*TWI*/
#if (TWI_BITRATE_REG < 0 || TWI_BITRATE_REG > 255)

	#error "The configured value for the bitrate register is out of range"
	
#elif (TWI_BITRATE_PRESCALER < 0 || TWI_BITRATE_PRESCALER > 3)

	#error "The configured value for the bitrate prescaler register is out of range"
	
#elif TWI_TIMEOUT_MS < 0

	#error "The configured value for the hardware timeout of the TWI is out of range"
	
#endif

/*UI*/
#if (UI_STD_CURVE_SIZE < 0 || UI_STD_CURVE_SIZE > 10)

	#error "The configured size for the standard curve is out of range"
	
#elif (UI_STD_TEMP_1 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_1 > BME280_MAX_TEMP_C_X10)

	#error "The configured temperature for node 1 of the standard curve is out of range"
	
#elif (UI_STD_TEMP_2 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_2 > BME280_MAX_TEMP_C_X10)

	#error "The configured temperature for node 2 of the standard curve is out of range"
	
#elif (UI_STD_TEMP_3 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_3 > BME280_MAX_TEMP_C_X10)

	#error "The configured temperature for node 3 of the standard curve is out of range"
	
#elif (UI_STD_TEMP_4 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_4 > BME280_MAX_TEMP_C_X10)

	#error "The configured temperature for node 4 of the standard curve is out of range"
	
#elif (UI_STD_TEMP_5 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_5 > BME280_MAX_TEMP_C_X10)

	#error "The configured temperature for node 5 of the standard curve is out of range"
	
#elif (UI_STD_TEMP_6 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_6 > BME280_MAX_TEMP_C_X10)

	#error "The configured temperature for node 6 of the standard curve is out of range"
	
#elif (UI_STD_TEMP_7 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_7 > BME280_MAX_TEMP_C_X10)

	#error "The configured temperature for node 7 of the standard curve is out of range"
	
#elif (UI_STD_TEMP_8 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_8 > BME280_MAX_TEMP_C_X10)

	#error "The configured temperature for node 8 of the standard curve is out of range"
	
#elif (UI_STD_TEMP_9 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_9 > BME280_MAX_TEMP_C_X10)

	#error "The configured temperature for node 9 of the standard curve is out of range"
	
#elif (UI_STD_TEMP_10 < BME280_MIN_TEMP_C_X10 || UI_STD_TEMP_10 > BME280_MAX_TEMP_C_X10)

	#error "The configured temperature for node 10 of the standard curve is out of range"
	
#elif BUTTONS_DEBOUNCE_TIME_MS < 0

	#error "The configured value for the buttons software-debounce is out of range"
	
#elif BLINK_TIME_MS < 0

	#error "The configured value for the blinking time is out of range"

#endif

/*DISPLAY*/
#if (SELECT_WORD_ROW < 1 || SELECT_WORD_ROW > 8)

	#error "The configured row for the SELECT word is out of range"
	
#elif (SELECT_WORD_COLUMN < 1 || SELECT_WORD_COLUMN > 11)

	#error "The configured column for the SELECT word is out of range"

#elif (MODE_WORD_ROW < 1 || MODE_WORD_ROW > 8)

	#error "The configured row for the MODE word is out of range"
	
#elif (MODE_WORD_COLUMN < 1 || MODE_WORD_COLUMN > 13)

	#error "The configured column for the MODE word is out of range"

#elif (STANDARD_WORD_ROW < 1 || STANDARD_WORD_ROW > 8)

	#error "The configured row for the STANDARD word is out of range"
	
#elif (STANDARD_WORD_COLUMN < 1 || STANDARD_WORD_COLUMN > 9)

	#error "The configured column for the STANDARD word is out of range"

#elif (ADVANCED_WORD_ROW < 1 || ADVANCED_WORD_ROW > 8)

	#error "The configured row for the ADVANCED word is out of range"
	
#elif (ADVANCED_WORD_COLUMN < 1 || ADVANCED_WORD_COLUMN > 9)

	#error "The configured column for the ADVANCED word is out of range"

#elif (NODE_WORD_ROW < 1 || NODE_WORD_ROW > 8)

	#error "The configured row for the NODE word is out of range"
	
#elif (NODE_WORD_COLUMN < 1 || NODE_WORD_COLUMN > 13)

	#error "The configured column for the NODE word is out of range"

#elif (NODE_DIGITS_ROW < 1 || NODE_DIGITS_ROW > 8)

	#error "The configured row for the node digits is out of range"
	
#elif (NODE_DIGITS_COLUMN < 1 || NODE_DIGITS_COLUMN > 15)

	#error "The configured column for the node digits is out of range"

#elif (TEMP_WORD_ROW < 1 || TEMP_WORD_ROW > 8)

	#error "The configured row for the TEMP word is out of range"
	
#elif (TEMP_WORD_COLUMN < 1 || TEMP_WORD_COLUMN > 3)

	#error "The configured column for the TEMP word is out of range"

#elif (TEMP_DIGITS_ROW < 1 || TEMP_DIGITS_ROW > 8)

	#error "The configured row for the temperature digits is out of range"
	
#elif (TEMP_DIGITS_COLUMN < 1 || TEMP_DIGITS_COLUMN > 11)

	#error "The configured column for the temperature digits is out of range"

#elif (SPEED_WORD_ROW < 1 || SPEED_WORD_ROW > 8)

	#error "The configured row for the SPEED word is out of range"
	
#elif (SPEED_WORD_COLUMN < 1 || SPEED_WORD_COLUMN > 3)

	#error "The configured column for the SPEED word is out of range"

#elif (SPEED_DIGITS_ROW < 1 || SPEED_DIGITS_ROW > 8)

	#error "The configured row for the speed digits is out of range"
	
#elif (SPEED_DIGITS_COLUMN < 1 || SPEED_DIGITS_COLUMN > 10)

	#error "The configured column for the speed digits is out of range"

#elif (ERR_WORD_ROW < 1 || ERR_WORD_ROW > 8)

	#error "The configured row for the ERR word is out of range"
	
#elif (ERR_WORD_COLUMN < 1 || ERR_WORD_COLUMN > 14)

	#error "The configured column for the ERR word is out of range"

#elif (ERR_DIGITS_ROW < 1 || ERR_DIGITS_ROW > 8)

	#error "The configured row for the error digits is out of range"
	
#elif (ERR_DIGITS_COLUMN < 1 || ERR_DIGITS_COLUMN > 16)

	#error "The configured column for the error digits is out of range"

#endif

/*BME280*/
#if (BME280_MIN_TEMP_C_X10 < 0 || BME280_MIN_TEMP_C_X10 > 850)

	#error "The configured minimum value for the temperature is out of range"
	
#elif (BME280_MAX_TEMP_C_X10 < 0 || BME280_MAX_TEMP_C_X10 > 850)

	#error "The configured maximum value for the temperature is out of range"
	
#elif BME280_MIN_TEMP_C_X10 > BME280_MAX_TEMP_C_X10

	#error "The configured minimum value for the temperature is higher than the configured maximum value"

#endif

/*FAN_DRIVER*/
#if (MIN_DC_REG_STEP < 1 || MIN_DC_REG_STEP > 10)

	#error "The configured size of the change of the duty-cycle register for the smallest duty-cycle variation is out of range"

#elif (HYST_CORR_FACT_X1000 < 15 || HYST_CORR_FACT_X1000 > 50)

	#error "The configured value of the hysteresis correction factor is out of range"

#elif (MIN_DC_REG_VALUE < 0 || MIN_DC_REG_VALUE > 40)

	#error "The configured minimum value of the duty-cycle register is out of range"
	
#elif (FAN_DRIVER_MIN_SPEED_RPM < 0 || FAN_DRIVER_MIN_SPEED_RPM > 9919)

	#error "The configured minimum value of the speed is out of range"
	
#elif (FAN_DRIVER_MAX_SPEED_RPM < 80 || FAN_DRIVER_MAX_SPEED_RPM > 9999)

	#error "The configured maximum value of the speed is out of range"
	
#elif FAN_DRIVER_MIN_SPEED_RPM > FAN_DRIVER_MAX_SPEED_RPM

	#error "The configured minimum value for the speed is higher than the configured maximum value"
	
#elif (FAN_DRIVER_MAX_SPEED_RPM - FAN_DRIVER_MIN_SPEED_RPM) < 80

	#error "The configured speed range is smaller than 80"
	
#elif FAN_DRIVER_BOOT_DELAY_MS < 0

	#error "The configured value for the fan boot delay is out of range"
	
#elif FAN_DRIVER_TIMEOUT_MS < 0

	#error "The configured value for the hardware timeout of the fan tachometer is out of range"
	
#elif FAN_DRIVER_UPDATE_TIME_MS < 0

	#error "The configured value for the PWM target update is out of range"
	
#endif

/*FAULT_MANAGER*/
#if BUZZER_DURATION_S < 0

	#error "The configured duration for the buzzer sound is out of range"
	
#endif
