#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "unistd.h"
#include "errno.h"

#define byte uint8_t

typedef ptrdiff_t isize;

typedef int error; 

typedef void (*sleeper)(int64_t nanoseconds);

typedef error (*reader)(byte* buffer, isize* read_len, int fd);
typedef error (*writer)(byte* buffer, isize* written_len, int fd);

typedef struct {
	writer write;
	reader read;
} readwriter_t;

#define zero(x) memset(x, 0, sizeof(*x))