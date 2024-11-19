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
        if (buffer[*read_len - 1] == '\n') {
            buffer[*read_len - 1] = '\0'; // If last read character is '\n', replace it with a null terminator.
            (*read_len)--; // Adjust length according to previous mod.
        }
        printf("Successful readout, null terminating the buffer.\n");
    }

    return err;
}
