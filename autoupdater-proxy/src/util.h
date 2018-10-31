#pragma once

#include <stdlib.h>
#include <stdint.h>

void strntr(char* str, size_t len, char a, char b);

#define strtr(str, a, b) \
	strntr(str, strlen(str), a, b);

#define hex_to_nibble(hex) \
	(((hex) >= '0' && (hex) <= '9' ? (uint8_t)((hex) - '0') : \
		(hex) >= 'A' && (hex) <= 'F' ? (uint8_t)((hex) - 'A' + 0xA) : \
			(hex) >= 'a' && (hex) <= 'f' ? (uint8_t)((hex) - 'a' + 0xA) : \
				0 \
	) & 0xF)

#define hex_to_byte(hex) \
	((hex_to_nibble((hex)[0]) << 4) | hex_to_nibble((hex)[1]))

#define ARRAY_SHUFFLE(arr, len) \
	{ \
		typeof((len)) i, j; \
		typeof(*(arr)) tmp; \
		for(i = 0; i + 1 < (len); i++) { \
			j = i + rand() / (RAND_MAX / ((len) - i) + 1); \
			tmp = (arr)[j]; \
			(arr)[j] = (arr)[i]; \
			(arr)[i] = tmp; \
		} \
	}
