#include "common/common.h"

#define ERSOCK 44       /* Attempting to read from a closed socket */
#define ERSOURCE 45     /* Error during read: Resource temporarily unavailable */

#define ERSYS -1

extern sleeper sleep_nano;

extern readwriter_t commandline;

extern void sleep_nano_linux(int64_t nanoseconds);

extern error unix_tcp_read(byte* buffer, isize* read_len);
extern error set_socket_non_blocking(int sockfd);
extern error setup_tcp_server_config(int port);
extern error closeconn();

extern error setup(int port);
