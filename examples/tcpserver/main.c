#include <stdlib.h>

#include "common/common.h"
#include "common/shell/shell.h"

#include "config.h"

#define microseconds (1000)
#define milliseconds (1000 * microseconds)
#define seconds (1000 * milliseconds)

#define shellBufferSize 512 // bytes

#define port 5000

byte buffer[shellBufferSize];

static bool conn_open = false;

int main()
{
    error err;
    isize totalRead;

    while (true) { // First loop for reconnection mechanism.
        if (conn_open == false) {
            err = setup(port);
            if (err != 0) {
                printf("Setup failed. Closing program.\n");
                exit(1);
            } 
        }
        conn_open = true;
        
        shell_t s = shell_new(commandline, sizeof(buffer));
        printf("Shell ready.\n");

        while (true) { // Second loop for non-blocking reading.
            totalRead = shellBufferSize; // Buffer must be set for each read cycle.
            
            err = shell_read(&s, buffer, &totalRead);

            if (totalRead > 0) {
                printf("Received %ld bytes: %.*s\n", totalRead, (int)totalRead, buffer);
                                
                // TODO: close conn only if successfull interaction with client.
                err = closeconn();
                if (err != 0) {
                    printf("Failed closing connection with client. Closing program.\n");
                    exit(1);
                }
                conn_open = false;
            }
            
            #ifdef CHECK_NONBLOCK
                printf("Check non-blocking.\n");
            #endif

            if (conn_open == false) {
                printf("Reopening server.\n");
                break; // Once we closed the connection with the client we break this loop and reopen the server.
            }

            sleep_nano(500 * microseconds);
        }
    }
    return 0;
}