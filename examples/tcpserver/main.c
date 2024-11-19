#include <stdlib.h>

#include "common/common.h"
#include "common/shell/shell.h"

#include "config.h"

#define errInvalidArg 40

#define microseconds (1000)
#define milliseconds (1000 * microseconds)
#define seconds (1000 * milliseconds)

#define shellBufferSize 512 // bytes

byte buffer[shellBufferSize];

int main()
{
    int port = 5000;

    error err;
    isize totalRead;

    err = setup(port);
    if (err != 0) {
        printf("Setup failed. Exiting program.\n");
        exit(1);
    }

    shell_t s = shell_new(commandline, sizeof(buffer));
    printf("Shell ready on port %d...\n", port);

    while (true) {
        totalRead = shellBufferSize; // Buffer must be set for each read cycle.
        err = shell_read(&s, buffer, &totalRead);
        if (totalRead > 0) {
            printf("Received %ld bytes: %.*s\n", totalRead, (int)totalRead, buffer);
        }
        sleep_nano(500 * microseconds);
    }

    return 0;
}