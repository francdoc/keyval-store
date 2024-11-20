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

byte bufferRead[shellBufferSize];
byte bufferWrite[shellBufferSize];

static bool conn_open = false;

int main()
{
    error err;
    isize totalRead;

    filemanager_t flm;

    flm = new_filemanager("SET", "GET", "DEL", " ", filemanagerBufferSize);
    
    err = sys_setup(port);
    if (err != 0) {
        printf("Setup failed. Closing program.\n");
        exit(EXIT_FAILURE);
    } 

    while (true) { // First loop for reconnection mechanism.
        if (conn_open == false) {
            err = acceptconn();
            if (err != 0) {
                printf("Accept connection failed. Closing program.\n");
                exit(EXIT_FAILURE);
            }
            conn_open = true;
        }
        
        shell_t s = shell_new(commandline, sizeof(bufferRead));
        printf("Shell ready.\n");

        while (true) { // Second loop for non-blocking reading.
            totalRead = shellBufferSize; // Buffer must be set for each read cycle.
            
            err = shell_read(&s, bufferRead, &totalRead);

            if (totalRead > 0) {
                printf("Received cmd: %s\n", bufferRead);

                char read_val[filemanagerBufferSize] = {0};
                err = filemanager_process_cmd(flm, bufferRead, totalRead, read_val);
                                
                if (err == SYSOK) { // If the processed command was valid and successfully handled, close the connection with the client.
                    snprintf((char*) bufferWrite, sizeof(bufferWrite), "OK\n");
                    isize write_len = strlen((char*)bufferWrite);
                    shell_write(&s, bufferWrite, &write_len);
                }
                else if (err == ERFNOTFOUND) {
                    snprintf((char*)bufferWrite, sizeof(bufferWrite), "NOTFOUND\n");
                    isize write_len = strlen((char*)bufferWrite);
                    shell_write(&s, bufferWrite, &write_len);
                }
                else if (err == RETURNVAL) {
                    size_t fixed_overhead = strlen("OK\n") + strlen("\n") + 1; // To avoid warning related to truncated writing.
                    size_t max_read_val_length = sizeof(bufferWrite) - fixed_overhead;
                    snprintf((char*)bufferWrite, sizeof(bufferWrite), "OK\n%.*s\n", (int)max_read_val_length, read_val);
                    isize write_len = strlen((char*)bufferWrite);
                    shell_write(&s, bufferWrite, &write_len);
                }
                else if (err == ERSYS) {
                    printf("Command not found, ending program.\n");
                    exit(EXIT_FAILURE);                
                }
                err = closeconn();
                if (err != 0) {
                    printf("Failed closing connection with client. Closing program.\n");
                    exit(EXIT_FAILURE);
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
    return SYSOK;
}