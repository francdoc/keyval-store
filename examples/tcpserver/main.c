#include <stdlib.h>

#include "common/common.h"
#include "common/syscodes.h"
#include "common/shell/shell.h"
#include "common/filemanager/filemanager.h"

#include "config.h"

#define microseconds (1000)
#define milliseconds (1000 * microseconds)
#define seconds (1000 * milliseconds)

#define shellBufferSize 512 // bytes
#define filemanagerBufferSize 512 // bytes

#define port 5000

byte buffer[shellBufferSize];

static bool conn_open = false;

int main()
{
    error err;
    isize totalRead;

    filemanager_t flm;

    flm = new_filemanager("SET", "GET", "DEL", " ", filemanagerBufferSize);
    
    while (true) { // First loop for reconnection mechanism.
        if (conn_open == false) {
            err = sys_setup(port);
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

                char* read_val;
                err = filemanager_process_cmd(flm, buffer, totalRead, read_val);
                
                // TODO: run valgrind.
                
                if (err == SYSOK) { // // If the processed command was valid and successfully handled, close the connection with the client.
                    err = closeconn();
                    if (err != 0) {
                        printf("Failed closing connection with client. Closing program.\n");
                        exit(EXIT_FAILURE);
                    }
                    conn_open = false;
                }
                else if (err == ERFNOTFOUND) {

                }
                else if (err == RETURNVAL) {

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
    return SYSOK;
}