#pragma once

#define SYSOK            0           
#define RETURNVAL        1  /* Return value from file. */

#define ERRSYS          -1  /* General program error.*/
#define ERRSOCK         -2  /* Attempting to read from a closed socket */
#define ERRSOURCE       -3  /* Error during read: Resource temporarily unavailable */
#define ERRFILE         -4  /* Error during read: File not found.*/