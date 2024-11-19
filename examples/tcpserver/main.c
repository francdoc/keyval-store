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
                exit(EXIT_FAILURE);
            } 
        }
        conn_open = true;
        
        shell_t s = shell_new(commandline, sizeof(buffer));
        printf("Shell ready.\n");

        while (true) { // Second loop for non-blocking reading.
            totalRead = shellBufferSize; // Buffer must be set for each read cycle.
            
            err = shell_read(&s, buffer, &totalRead);

            if (totalRead > 0) {
                printf("Received cmd: %s\n", buffer);

                err = process_cmd(buffer, totalRead);
                
                // TODO: add answer to client.
                // TODO: check if exit(EXIT_FAILURE) is valid to end program.
                
                if (err == 0) { // // If the processed command was valid and successfully handled, close the connection with the client.
                    err = closeconn();
                    if (err != 0) {
                        printf("Failed closing connection with client. Closing program.\n");
                        exit(EXIT_FAILURE);
                    }
                    conn_open = false;
                }
                else {
                    printf("Command not found, ending program.\n");
                    exit(EXIT_FAILURE);
                }
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