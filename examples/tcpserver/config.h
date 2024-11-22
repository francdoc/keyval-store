#include "common/common.h"

extern sleeper sleep_nano;

extern readwriter_t commandline;

extern void sleep_nano_linux(int64_t nanoseconds);

extern error unix_tcp_read(byte* buffer, isize* read_len);
extern error unix_tcp_write(byte* buffer, isize* write_len);
extern error set_socket_non_blocking(int sockfd);
extern error setup_tcp_server(int port);
extern error closeconn();
extern error acceptconn();

extern error sys_setup(int port);