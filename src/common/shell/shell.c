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
    // TODO: handle case where read_len exceeds buffsize.
    error err = s->rw.read(buffer, read_len);

    if (*buffer == '\n' | buffer[0] == ' ') {
        *read_len = 0; // If message is empty then we ignore it.
    }    

    if (*read_len > 0) { // We access the value pointed to by read_len.
        if (buffer[*read_len - 1] == '\n') {
            buffer[*read_len - 1] = '\0'; // If last read character is '\n', replace it with a null terminator.
            (*read_len)--; // Adjust length according to previous mod.
        }
    }

    return err;
}

error shell_write(shell_t* s, byte* buffer, isize* write_len) {
    if (!buffer || !write_len || *write_len <= 0) {
        return ERRSYS;
    }

    ssize_t ret = s->rw.write(buffer, write_len);

    if (ret > 0) {
        *write_len = ret;
        return SYSOK;
    }

    *write_len = 0;
    return ERRSYS;
}