#include <stdio.h>
#include <stdint.h>

static const uint8_t PATTERN[8][8] = {
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0}
};

void pattern_convert(const uint8_t (*binary_char)[8]){
	uint8_t hex_char[] = {0, 0, 0, 0, 0, 0, 0, 0};
	
	for (uint8_t j = 0; j < 8; j++) {
		
		for (uint8_t i = 0; i < 8; i++) {
			hex_char[j] |= (binary_char[i][j] << (i));
		}	
	}
	
	printf("\n{");
	
	for (uint8_t i = 0; i < 7; i++) {
		printf("0x%X, ", hex_char[i]);
	}
	
	printf("0x00};\n\n");
}

int main(void) {
	
	pattern_convert(PATTERN);
	
	return 0;
}
