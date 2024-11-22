#include "common/common.h"

typedef struct {
	readwriter_t rw; 
	isize buffsize;
	int fd;
} shell_t;

extern shell_t shell_new(readwriter_t rw, isize buffsize, int fd);
extern error shell_read(shell_t* s, byte* buffer, isize* read_len);
extern error shell_write(shell_t* s, byte* buffer, isize* write_len);