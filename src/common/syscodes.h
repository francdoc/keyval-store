#pragma once

#define SYSOK 0           
#define RETURNVAL 1    /* Return value from file. */

#define ERSYS       -1  /* General program error.*/
#define ERSOCK      -2  /* Attempting to read from a closed socket */
#define ERSOURCE    -3  /* Error during read: Resource temporarily unavailable */
#define ERFNOTFOUND -4  /* Error during read: File not found.*/
