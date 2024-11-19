#include "common/common.h"
#include "common/shell/shell.h"

shell_t shell_new(readwriter_t rw, isize buffsize)
{
	shell_t s;
	zero(&s);
	s.buffsize = buffsize;
	s.rw = rw;
	return s;
}

error shell_read(shell_t* s, byte* buffer, isize* read_len)
{
    // TODO: implement logic to handle case where read_len exceeds buffsize (some kind of accumulator).
    error err = s->rw.read(buffer, read_len);

    if (*buffer == '\n' | buffer[0] == ' ') {
        *read_len = 0; // If message is empty then we ignore it.
    }    

    if (*read_len > 0) { // We access the value pointed to by read_len.
        printf("Successful readout, null terminating the buffer.\n");
        buffer[*read_len] = '\0'; // Null-terminate the buffer.
    }

    return err;
}
