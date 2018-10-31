#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t fetch_flag;

struct fetch_state {
	struct {
		fetch_flag spider:1;
	} flags;
	bool success;
};

int get_url(char* url);
int spider_url(char* url);

#define FETCH_UC_TO_STATE(uc) \
	((struct fetch_state*)(uc)->priv)
