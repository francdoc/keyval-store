#include "common/common.h"
#include "common/shell/shell.h"
#include "common/syscodes.h"

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
    *read_len = s->buffsize;
    
    error err = s->rw.read(buffer, read_len);

    if (err != SYSOK) {
        printf("[SHELL]: Error with reader port.\n");
        return ERRSYS;
    }

    if (*buffer == '\n' | buffer[0] == ' ') {
        *read_len = 0; // If message is empty then we ignore it.
        return SYSOK;
    }

    // If the data read equals/exceeds the buffer size, it may indicate data loss or buffer overflow.
    if (*read_len >= s->buffsize) {
        return ERRSYS;
    }

    if (*read_len > 0) { // We access the value pointed to by read_len.
        buffer[*read_len - 1] = '\0'; // Replace last character with a null terminator.
        (*read_len)--; // Adjust length according to previous modification.
        return SYSOK;
    }

    *read_len = 0;
    return ERRSYS; // If we got here then return error.
}

error shell_write(shell_t* s, byte* buffer, isize* write_len) {
    if (!buffer || !write_len || *write_len <= 0) {
        printf("[SHELL]: Invalid write attempt: buffer is null or write length is zero.\n");
        return ERRSYS;
    }

    error err = s->rw.write(buffer, write_len);

    if (err == SYSOK) {
        printf("[SHELL]: Shell write successful. Bytes written: %ld\n", *write_len);
        return SYSOK;
    }

    *write_len = 0;
    printf("[SHELL]: Shell write failed. No bytes written.\n");
    return ERRSYS;
}
