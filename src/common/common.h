#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "unistd.h"
#include "errno.h"
#include <stdbool.h>
#include <stdlib.h>

#define byte uint8_t

typedef ptrdiff_t isize;

typedef int error; 

typedef void (*sleeper)(int64_t nanoseconds);

typedef error (*reader)(byte* buffer, isize* read_len);
typedef error (*writer)(byte* data_start, isize data_len, isize* written_len);

typedef struct {
	writer write;
	reader read;
} readwriter_t;

#define zero(x) memset(x, 0, sizeof(*x))