#include "common/common.h"

typedef struct {
	readwriter_t rw; 
	isize buffsize;
} shell_t;

extern shell_t shell_new(readwriter_t rw, isize buffsize);
extern error shell_read(shell_t* s, byte* buffer, isize* read_len);
