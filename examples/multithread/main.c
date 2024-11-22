#include <stdlib.h>

#include "common/common.h"
#include "common/syscodes.h"
#include "common/shell/shell.h"
#include "common/filemanager/filemanager.h"

#include "config.h"

#define microseconds (1000)
#define milliseconds (1000 * microseconds)
#define seconds (1000 * milliseconds)

#define port 5000

int main()
{
    error err;

    err = sys_setup(port);
    if (err != 0) {
        perror("[MAIN]: Setup failed. Closing program.\n");
        exit(EXIT_FAILURE);
    } 

    while (true) {
        err = sys_update(); // Handles connections, shell I/O and filemanager for new clients.
        if (err != 0) {
            perror("[MAIN]: System update failed. Closing program.\n");
            exit(EXIT_FAILURE);
        }    
        sleep_nano(500 * microseconds);
    }
    return SYSOK;
}