#include <stdio.h>
#include <stdint.h>

static const uint8_t PATTERN[8] = {0x00, 0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00};

void pattern_convert(const uint8_t* hex_char){
	uint8_t binary_char[8][8] = {
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0}
	};
	
	for (uint8_t j = 0; j < 8; j++) {
		
		for (uint8_t i = 0; i < 8; i++) {
			binary_char[i][j] = (hex_char[j] & (1 << i)) ? 1 : 0;
		}	
	}
	
	printf("/*\n*\n*	{\n");
	
	for (uint8_t i = 0; i < 7; i++) {
		printf("*		{");
		
		for (uint8_t j = 0; j < 7; j++) {
			printf("%u, ", binary_char[i][j]);
		}
		
		printf("%u},\n", binary_char[i][7]);
	}
	
	
	printf("*		{");
		
	for (uint8_t j = 0; j < 7; j++) {
		printf("%u, ", binary_char[7][j]);
	}
	
	printf("%u}\n", binary_char[7][7]);
	printf("*	};\n*\n*/\n\n");
}

int main(void) {
	
	pattern_convert(PATTERN);
	
	return 0;
}
