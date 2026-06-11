/**
*   
*   @file display.c
*
*   @brief Implementation for the display module.
*
*   @details Contains the pattern bitmaps generated using
*	the 8x8_bin_to_hex tool.
*
*   Handles digit extraction, character positioning and blinking.
*
*/

#include <stdint.h>
#include <stddef.h>
#include <avr/pgmspace.h>
#include "display.h"
#include "ssd1306.h"
#include "scheduler.h"
#include "config.h"

#define CHAR_PATTERN_BYTES 8
#define SELECT_WORD_LENGTH 6
#define MODE_WORD_LENGTH 4
#define STANDARD_WORD_LENGTH 8
#define ADVANCED_WORD_LENGTH 8
#define TEMP_WORD_LENGTH 14
#define SPEED_WORD_LENGTH 14
#define NODE_WORD_LENGTH 4
#define ERR_WORD_LENGTH 3

#define DISPLAY_MIN_INDEX 1
#define DISPLAY_MAX_INDEX 10

//Value format: 12.3... = 123
#define DISPLAY_MIN_TEMP_C_X10 0
#define DISPLAY_MAX_TEMP_C_X10 999
#define DISPLAY_TEMP_DIGITS 3

#define DISPLAY_MIN_SPEED_RPM 0
#define DISPLAY_MAX_SPEED_RPM 9999
#define DISPLAY_SPEED_DIGITS 4

/*
*
*	{
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 1, 0, 1, 0, 0, 0},
*		{0, 0, 1, 0, 1, 0, 0, 0},
*		{0, 1, 0, 0, 0, 1, 0, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
static const uint8_t A_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x60, 0x18, 0x16, 0x11, 0x16, 0x18, 0x60, 0x00};

/*
*
*	{
*		{0, 0, 1, 1, 1, 1, 1, 0},
*		{0, 1, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{0, 1, 0, 0, 0, 0, 0, 0},
*		{0, 0, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
static const uint8_t C_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x1C, 0x22, 0x41, 0x41, 0x41, 0x41, 0x41, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 0, 0, 0},
*		{1, 0, 0, 0, 0, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 1, 0, 0},
*		{1, 1, 1, 1, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t D_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x41, 0x41, 0x41, 0x41, 0x22, 0x1C, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t E_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x00};

/*
*
*	{
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t L_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00};

/*
*
*	{
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 0, 0, 0, 1, 1, 0},
*		{1, 0, 1, 0, 1, 0, 1, 0},
*		{1, 0, 0, 1, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t M_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x02, 0x04, 0x08, 0x04, 0x02, 0x7F, 0x00};

/*
*
*	{
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 0, 0, 0, 0, 1, 0},
*		{1, 0, 1, 0, 0, 0, 1, 0},
*		{1, 0, 0, 1, 0, 0, 1, 0},
*		{1, 0, 0, 0, 1, 0, 1, 0},
*		{1, 0, 0, 0, 0, 1, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t N_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x02, 0x04, 0x08, 0x10, 0x20, 0x7F, 0x00};

/*
*
*	{
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t O_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x3E, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3E, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t P_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x09, 0x09, 0x09, 0x09, 0x09, 0x06, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 1, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 1, 0, 0, 0},
*		{1, 0, 0, 0, 0, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t R_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x7F, 0x09, 0x09, 0x09, 0x1B, 0x29, 0x46, 0x00};

/*
*
*	{
*		{0, 1, 1, 1, 1, 1, 1, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t S_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x46, 0x49, 0x49, 0x49, 0x49, 0x49, 0x31, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t T_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x01, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x01, 0x00};

/*
*
*	{
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 0, 0, 0, 1, 0, 0},
*		{0, 1, 0, 0, 0, 1, 0, 0},
*		{0, 0, 1, 0, 1, 0, 0, 0},
*		{0, 0, 1, 0, 1, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t V_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x03, 0x0C, 0x30, 0x40, 0x30, 0x0C, 0x03, 0x00};

/*
*
*	{
*		{0, 0, 1, 1, 1, 0, 0, 0},
*		{0, 1, 0, 0, 0, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 0, 0, 0, 1, 0, 0},
*		{0, 0, 1, 1, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t ZERO_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x1C, 0x22, 0x41, 0x41, 0x41, 0x22, 0x1C, 0x00};

/*
*
*	{
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 1, 1, 0, 0, 0, 0},
*		{0, 1, 0, 1, 0, 0, 0, 0},
*		{1, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t ONE_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x48, 0x44, 0x42, 0x7F, 0x40, 0x40, 0x40, 0x00};

/*
*
*	{
*		{0, 0, 1, 1, 1, 1, 0, 0},
*		{0, 1, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 1, 0, 0},
*		{0, 0, 1, 1, 1, 0, 0, 0},
*		{0, 1, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t TWO_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x44, 0x62, 0x51, 0x51, 0x51, 0x49, 0x46, 0x00};

/*
*
*	{
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 1, 0, 0},
*		{0, 0, 1, 1, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t THREE_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x22, 0x41, 0x49, 0x49, 0x49, 0x55, 0x22, 0x00};

/*
*
*	{
*		{0, 0, 0, 0, 1, 0, 0, 0},
*		{0, 0, 0, 1, 1, 0, 0, 0},
*		{0, 0, 1, 0, 1, 0, 0, 0},
*		{0, 1, 0, 0, 1, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 1, 0, 0, 0},
*		{0, 0, 0, 0, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t FOUR_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x10, 0x18, 0x14, 0x12, 0x7F, 0x10, 0x10, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 0, 1, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t FIVE_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x4F, 0x49, 0x49, 0x49, 0x49, 0x49, 0x31, 0x00};

/*
*
*	{
*		{0, 0, 1, 1, 1, 1, 1, 0},
*		{0, 1, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{1, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t SIX_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x3C, 0x4A, 0x49, 0x49, 0x49, 0x49, 0x31, 0x00};

/*
*
*	{
*		{1, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 1, 0, 0},
*		{0, 0, 0, 0, 1, 0, 0, 0},
*		{0, 0, 0, 1, 0, 0, 0, 0},
*		{0, 0, 1, 0, 0, 0, 0, 0},
*		{0, 1, 0, 0, 0, 0, 0, 0},
*		{1, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t SEVEN_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x41, 0x21, 0x11, 0x09, 0x05, 0x03, 0x01, 0x00};

/*
*
*	{
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t EIGHT_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x36, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00};

/*
*
*	{
*		{0, 1, 1, 1, 1, 1, 0, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{1, 0, 0, 0, 0, 0, 1, 0},
*		{0, 1, 1, 1, 1, 1, 1, 0},
*		{0, 0, 0, 0, 0, 0, 1, 0},
*		{0, 0, 0, 0, 0, 1, 0, 0},
*		{1, 1, 1, 1, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t NINE_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x46, 0x49, 0x49, 0x49, 0x49, 0x29, 0x1E, 0x00};

/*
*
*	{
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 1, 1, 0, 0, 0},
*		{0, 0, 0, 1, 1, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t DOT_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x00, 0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00};

/*
*
*	{
*		{0, 0, 1, 1, 0, 0, 0, 0},
*		{0, 1, 0, 0, 1, 0, 0, 0},
*		{0, 1, 0, 0, 1, 0, 0, 0},
*		{0, 0, 1, 1, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0},
*		{0, 0, 0, 0, 0, 0, 0, 0}
*	};
*
*/
 static const uint8_t DEG_PATTERN[CHAR_PATTERN_BYTES] PROGMEM = {0x00, 0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00};

 static const uint8_t* const DIGITS[10] PROGMEM = {ZERO_PATTERN, ONE_PATTERN, TWO_PATTERN, THREE_PATTERN, FOUR_PATTERN, FIVE_PATTERN, SIX_PATTERN, SEVEN_PATTERN, EIGHT_PATTERN, NINE_PATTERN};

 static const uint8_t* const SELECT[SELECT_WORD_LENGTH] PROGMEM = {S_PATTERN, E_PATTERN, L_PATTERN, E_PATTERN, C_PATTERN, T_PATTERN};

 static const uint8_t* const MODE[MODE_WORD_LENGTH] PROGMEM = {M_PATTERN, O_PATTERN, D_PATTERN, E_PATTERN};

 static const uint8_t* const STANDARD[STANDARD_WORD_LENGTH] PROGMEM = {S_PATTERN, T_PATTERN, A_PATTERN, N_PATTERN, D_PATTERN, A_PATTERN, R_PATTERN, D_PATTERN};

 static const uint8_t* const ADVANCED[ADVANCED_WORD_LENGTH] PROGMEM = {A_PATTERN, D_PATTERN, V_PATTERN, A_PATTERN, N_PATTERN, C_PATTERN, E_PATTERN, D_PATTERN};

 static const uint8_t* const TEMP[TEMP_WORD_LENGTH] PROGMEM = {T_PATTERN, E_PATTERN, M_PATTERN, P_PATTERN, NULL, NULL, NULL, NULL, NULL, NULL, DOT_PATTERN, NULL, DEG_PATTERN, C_PATTERN};

 static const uint8_t* const SPEED[SPEED_WORD_LENGTH] PROGMEM = {S_PATTERN, P_PATTERN, E_PATTERN, E_PATTERN, D_PATTERN, NULL, NULL, NULL, NULL, NULL, NULL, R_PATTERN, P_PATTERN, M_PATTERN};

 static const uint8_t* const NODE[NODE_WORD_LENGTH] PROGMEM = {N_PATTERN, O_PATTERN, D_PATTERN, E_PATTERN};

 static const uint8_t* const ERR[ERR_WORD_LENGTH] PROGMEM = {E_PATTERN, R_PATTERN, R_PATTERN};

static display_errors display_pattern_write(const uint8_t* pattern_add, uint8_t row, uint8_t column) {
	ssd_errors error_code = 0;
	uint8_t pattern[CHAR_PATTERN_BYTES] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	if (pattern_add != NULL) {
	
		for (uint8_t i = 0; i < 8; i++) {
			pattern[i] = pgm_read_byte(pattern_add + i);
		}
	}
	
	error_code = ssd1306_8x8char_write(pattern, row, column);
	
	if (error_code != SSD_ERR_OK) {
		return DISPLAY_ERR_SSD;
	}
	
	return DISPLAY_ERR_OK;
}

static display_errors display_blank_write(uint8_t length, uint8_t row, uint8_t column) {
	display_errors error_code = 0;
	
	for (uint8_t i = 0; i < length; i++) {
		error_code = display_pattern_write(NULL, row, (column + i));
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}
	
	return DISPLAY_ERR_OK;
}

static display_errors display_word_write(const uint8_t* const* word, uint8_t length, uint8_t row, uint8_t column) {
	display_errors error_code = 0;
	
	for (uint8_t i = 0; i < length; i++) {
		error_code = display_pattern_write(pgm_read_ptr_near(word + i), row, (column + i));
	
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}
	
	return DISPLAY_ERR_OK;
}

static void display_digits_extract(uint16_t full_value, uint8_t* digits, uint8_t length) {
	uint16_t divisor = 1;
	
	for (uint8_t i = 1; i < length; i++) {
		divisor *= 10;
	}
	
	for (uint8_t i = 0; i < length; i++) {
		*(digits + i) = (!i) ? full_value / divisor : (full_value / divisor) % 10;
		divisor /= 10;
	}
}

display_errors display_intro_write(void) {
	ssd_errors ssd_error_code = 0;
	display_errors display_error_code = 0;
	
	ssd_error_code = ssd1306_data_reset();
	
	if (ssd_error_code != SSD_ERR_OK) {
		return DISPLAY_ERR_SSD;
	}
	
	display_error_code = display_word_write(SELECT, SELECT_WORD_LENGTH, SELECT_WORD_ROW, SELECT_WORD_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}
	
	display_error_code = display_word_write(MODE, MODE_WORD_LENGTH, MODE_WORD_ROW, MODE_WORD_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}
	
	ssd_error_code = ssd1306_display_on();
	
	if (ssd_error_code != SSD_ERR_OK) {
		return DISPLAY_ERR_SSD;
	}
	
	return DISPLAY_ERR_OK;
}

display_errors display_std_write(void) {
	return display_word_write(STANDARD, STANDARD_WORD_LENGTH, STANDARD_WORD_ROW, STANDARD_WORD_COLUMN);
}

display_errors display_adv_write(void) {
	return display_word_write(ADVANCED, ADVANCED_WORD_LENGTH, ADVANCED_WORD_ROW, ADVANCED_WORD_COLUMN);
}

display_errors display_index_write(uint8_t index) {
	display_errors error_code = 0;
	
	if (index < DISPLAY_MIN_INDEX || index > DISPLAY_MAX_INDEX) {
		return DISPLAY_ERR_PARAM;
	}
	
	error_code = display_word_write(NODE, NODE_WORD_LENGTH, NODE_WORD_ROW, NODE_WORD_COLUMN);
	
	if (error_code != DISPLAY_ERR_OK) {
		return error_code;
	}
	
	if (index < DISPLAY_MAX_INDEX) {
		error_code = display_blank_write(1, NODE_DIGITS_ROW, NODE_DIGITS_COLUMN);
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
		
		error_code = display_pattern_write(pgm_read_ptr_near(DIGITS + index), NODE_DIGITS_ROW, NODE_DIGITS_COLUMN + 1);
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}
	
	else if (index == DISPLAY_MAX_INDEX) {
		
		error_code = display_pattern_write(pgm_read_ptr_near(DIGITS + 1), NODE_DIGITS_ROW, NODE_DIGITS_COLUMN);
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
		
		error_code = display_pattern_write(pgm_read_ptr_near(DIGITS), NODE_DIGITS_ROW, NODE_DIGITS_COLUMN + 1);
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}
	
	return DISPLAY_ERR_OK;
}

display_errors display_units_write(void) {
	ssd_errors ssd_error_code = 0;
	display_errors display_error_code = 0;
	
	ssd_error_code = ssd1306_data_reset();
	
	if (ssd_error_code != SSD_ERR_OK) {
		return DISPLAY_ERR_SSD;
	}
	
	display_error_code = display_word_write(TEMP, TEMP_WORD_LENGTH, TEMP_WORD_ROW, TEMP_WORD_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}
	
	display_error_code = display_word_write(SPEED, SPEED_WORD_LENGTH, SPEED_WORD_ROW, SPEED_WORD_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}

	return DISPLAY_ERR_OK;
}

display_errors display_temp_write(uint16_t temp_c_x10) {
	display_errors error_code = 0;
	uint8_t digits[DISPLAY_TEMP_DIGITS] = {0, 0, 0};
	
	if (temp_c_x10 > DISPLAY_MAX_TEMP_C_X10 || temp_c_x10 < DISPLAY_MIN_TEMP_C_X10) {
		return DISPLAY_ERR_PARAM;
	}
	
	display_digits_extract(temp_c_x10, digits, DISPLAY_TEMP_DIGITS);
	
	for (uint8_t i = 0; i < DISPLAY_TEMP_DIGITS; i++) {
		error_code = display_pattern_write(pgm_read_ptr_near(DIGITS + digits[i]), TEMP_DIGITS_ROW, TEMP_DIGITS_COLUMN + i + (i / 2));
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}

	return DISPLAY_ERR_OK;
}

display_errors display_speed_write(uint16_t speed_rpm) {
	display_errors error_code = 0;
	uint8_t digits[DISPLAY_SPEED_DIGITS] = {0, 0, 0, 0};
	
	if (speed_rpm > DISPLAY_MAX_SPEED_RPM || speed_rpm < DISPLAY_MIN_SPEED_RPM) {
		return DISPLAY_ERR_PARAM;
	}
	
	display_digits_extract(speed_rpm, digits, DISPLAY_SPEED_DIGITS);
	
	for (uint8_t i = 0; i < DISPLAY_SPEED_DIGITS; i++) {
		error_code = display_pattern_write(pgm_read_ptr_near(DIGITS + digits[i]), SPEED_DIGITS_ROW, SPEED_DIGITS_COLUMN + i);
		
		if (error_code != DISPLAY_ERR_OK) {
			return error_code;
		}
	}
	
	return DISPLAY_ERR_OK;
}

display_errors display_error_write(system_errors error_code) {
	display_errors display_error_code = 0;
	ssd_errors ssd_error_code = 0;
	
	if (error_code > SYS_ERR_FAN_DRIVER) {
		return DISPLAY_ERR_PARAM;
	}

	ssd_error_code = ssd1306_data_reset();
	
	if (ssd_error_code != SSD_ERR_OK) {
		return DISPLAY_ERR_SSD;
	}
	
	display_error_code = display_word_write(ERR, ERR_WORD_LENGTH, ERR_WORD_ROW, ERR_WORD_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}
	
	display_error_code = display_pattern_write(pgm_read_ptr_near(DIGITS + error_code), ERR_DIGITS_ROW, ERR_DIGITS_COLUMN);
	
	if (display_error_code != DISPLAY_ERR_OK) {
		return display_error_code;
	}
	
	return DISPLAY_ERR_OK;
}

typedef struct {
	uint16_t target_duration;
	uint16_t t_zero;
	blink_options option;
	uint8_t state;
	uint8_t digit;
	uint8_t init_flag;
} blink_params;

static blink_params blink = {0, 0, 0, 0, 0, 0};

static display_errors display_word_blink_switch(blink_options option, uint8_t state) {
	display_errors error_code = 0;
	
	if (option == STANDARD_WORD) {
		error_code = state ? display_blank_write(STANDARD_WORD_LENGTH, STANDARD_WORD_ROW, STANDARD_WORD_COLUMN) : display_std_write();
	}
		
	else if (option == ADVANCED_WORD) {
		error_code = state ? display_blank_write(ADVANCED_WORD_LENGTH, ADVANCED_WORD_ROW, ADVANCED_WORD_COLUMN) : display_adv_write();
	}
	
	return error_code;
}
		
static display_errors display_digit_blink_switch(blink_options option, uint8_t state, uint8_t digit) {
	display_errors error_code = 0;
		
	switch (option) {
		
		case TEMP_FIRST_DIGIT:
		
			error_code = state ? display_blank_write(1, TEMP_DIGITS_ROW, TEMP_DIGITS_COLUMN) : display_pattern_write(pgm_read_ptr_near(DIGITS + digit), TEMP_DIGITS_ROW, TEMP_DIGITS_COLUMN);
			break;
		
		case TEMP_SECOND_DIGIT:
		
			error_code = state ? display_blank_write(1, TEMP_DIGITS_ROW, TEMP_DIGITS_COLUMN + 1) : display_pattern_write(pgm_read_ptr_near(DIGITS + digit), TEMP_DIGITS_ROW, TEMP_DIGITS_COLUMN + 1);
			break;
		
		case TEMP_THIRD_DIGIT:
		
			error_code = state ? display_blank_write(1, TEMP_DIGITS_ROW, TEMP_DIGITS_COLUMN + 3) : display_pattern_write(pgm_read_ptr_near(DIGITS + digit), TEMP_DIGITS_ROW, TEMP_DIGITS_COLUMN + 3);
			break;
		
		case SPEED_FIRST_DIGIT:
		
			error_code = state ? display_blank_write(1, SPEED_DIGITS_ROW, SPEED_DIGITS_COLUMN) : display_pattern_write(pgm_read_ptr_near(DIGITS + digit), SPEED_DIGITS_ROW, SPEED_DIGITS_COLUMN);
			break;
		
		case SPEED_SECOND_DIGIT:
		
			error_code = state ? display_blank_write(1, SPEED_DIGITS_ROW, SPEED_DIGITS_COLUMN + 1) : display_pattern_write(pgm_read_ptr_near(DIGITS + digit), SPEED_DIGITS_ROW, SPEED_DIGITS_COLUMN + 1);
			break;
		
		case SPEED_THIRD_DIGIT:
		
			error_code = state ? display_blank_write(1, SPEED_DIGITS_ROW, SPEED_DIGITS_COLUMN + 2) : display_pattern_write(pgm_read_ptr_near(DIGITS + digit), SPEED_DIGITS_ROW, SPEED_DIGITS_COLUMN + 2);
			break;
		
		default:
			
			error_code = DISPLAY_ERR_PARAM;
			break;
	}

	return error_code;
}

static void display_blink_reset(void) {
	blink.target_duration = 0;
	blink.t_zero = 0;
	blink.option = 0;
	blink.state = 0;
	blink.digit = 0;
	blink.init_flag = 0;
}

display_errors display_word_blink(uint16_t target_duration, blink_options option) {
	
	if (!target_duration || option > ADVANCED_WORD || option < STANDARD_WORD) {
		return DISPLAY_ERR_PARAM;
	}
	
	display_blink_reset();
	blink.target_duration = target_duration;
	blink.t_zero = scheduler_timer_get_timestamp();
	blink.option = option;
	blink.init_flag = 1;
	return DISPLAY_ERR_OK;
}

display_errors display_digit_blink(uint16_t target_duration, blink_options option, uint16_t full_value) {
	uint8_t digits [DISPLAY_SPEED_DIGITS] = {0, 0, 0, 0};
	
	if (target_duration == 0 || option < TEMP_FIRST_DIGIT || option > SPEED_THIRD_DIGIT) {
		return DISPLAY_ERR_PARAM;
	}
	
	if (option < SPEED_FIRST_DIGIT && (full_value > DISPLAY_MAX_TEMP_C_X10 || full_value < DISPLAY_MIN_TEMP_C_X10)) {
		return DISPLAY_ERR_PARAM;
	}
	
	if (option > TEMP_THIRD_DIGIT && (full_value > DISPLAY_MAX_SPEED_RPM || full_value < DISPLAY_MIN_SPEED_RPM)) {
		return DISPLAY_ERR_PARAM;
	}
	
	display_blink_reset();
	blink.target_duration = target_duration;
	blink.t_zero = scheduler_timer_get_timestamp();
	blink.option = option;
	blink.init_flag = 1;
	
	if (option < SPEED_FIRST_DIGIT) {
		display_digits_extract(full_value, digits, DISPLAY_TEMP_DIGITS);
		blink.digit = !(option - TEMP_FIRST_DIGIT) ? digits[0] : (!(option - TEMP_SECOND_DIGIT) ? digits[1] : digits[2]);
	}
	
	else {
		display_digits_extract(full_value, digits, DISPLAY_SPEED_DIGITS);
		blink.digit = !(option - SPEED_FIRST_DIGIT) ? digits[0] : (!(option - SPEED_SECOND_DIGIT) ? digits[1] : digits[2]);
	}
	
	return DISPLAY_ERR_OK;
}

display_errors display_blink_switch(void) {
	display_errors error_code = 0;
	
	if (	!blink.init_flag) {
		return DISPLAY_ERR_PARAM;
	}
	
	if (scheduler_timer_poll(&(blink.t_zero), blink.target_duration)) {
		error_code = (blink.option < TEMP_FIRST_DIGIT) ? display_word_blink_switch(blink.option, blink.state) : display_digit_blink_switch(blink.option, blink.state, blink.digit);
		blink.state = !blink.state;
	}
	
	return error_code;
}
