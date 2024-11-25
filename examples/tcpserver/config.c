#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <fcntl.h>
#include <sys/select.h>

#include "common/common.h"
#include "common/shell/shell.h"

#include "config.h"

#include "common/syscodes.h"

readwriter_t commandline;
sleeper sleep_nano;

int global_server_sock_fd = 0;

void sleep_nano_linux(int64_t nanoseconds)
{
	double sleep_seconds = (double)(nanoseconds) / 1e9;
	sleep(sleep_seconds);
}

error set_socket_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("[CONFIG]: fcntl F_GETFL");
        return ERRSYS;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("[CONFIG]: fcntl F_SETFL O_NONBLOCK");
        return ERRSYS;
    }
    return SYSOK;
}

// Reader port.
error unix_tcp_read(byte* buffer, isize* read_len, int client_fd)
{
    ssize_t ret = read(client_fd, buffer, *read_len);

	if (ret > 0) { // "ret" equals to the number of bytes that were read.
		*read_len = ret;
		return SYSOK;
	}

	if (errno == EAGAIN || errno == EWOULDBLOCK) { 
		// No data available in non-blocking mode. 
		// Since the socket is configured as non-blocking we let the program continue normally.
		return SYSOK; 
	}

	if (ret == 0) { // Indicates that the client has closed the socket.
		printf("[CONFIG]: Connection lost with client.\n");
		return ERRSYS;
	}

	*read_len = 0;
	return ERRSYS; // If we got here then return error.
}

// Writer port.
error unix_tcp_write(byte* buffer, isize* write_len, int client_fd) {
    ssize_t ret = write(client_fd, buffer, *write_len);

    if (ret > 0) { 
        *write_len = ret;
        return SYSOK;
    }

    *write_len = 0; // Reset the write length on error.
    return ERRSYS; // If we got here then return error.
}

error setup_tcp_server(int port)
{
	struct sockaddr_in server_addr;

	global_server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (global_server_sock_fd < 0) {
		perror("[CONFIG]: Socket creation failed");
		return ERRSYS;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(global_server_sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("[CONFIG]: Socket bind failed");
		close(global_server_sock_fd);
		return ERRSYS;
	}

	if (listen(global_server_sock_fd, 1) < 0) {
		perror("[CONFIG]: Listen failed");
		close(global_server_sock_fd);
		return ERRSYS;
	}
	printf("[CONFIG]: Socket listening.\n");
	return SYSOK;
}

error acceptconn(int* client_fd) {
	socklen_t client_len;
	struct sockaddr_in client_addr;

	client_len = sizeof(client_addr);
	*client_fd = accept(global_server_sock_fd, (struct sockaddr*)&client_addr, &client_len);
	if (*client_fd < 0) {
		perror("[CONFIG]: Accept failed.\n");
		return ERRSYS;
	}
	printf("[CONFIG]: Client connected to server.\n");
	
	return SYSOK;
}

error closeconn(int client_fd) {
	printf("[CONFIG]: Closing connection with client.\n");
	if (close(client_fd) !=0){
		return ERRSYS;
	}
	return SYSOK;
}

error sys_setup(int port)
{
	if (setup_tcp_server(port) < 0) {
		perror("[CONFIG]: Error with setup server.\n");
		return ERRSYS;
	}

	sleep_nano = &sleep_nano_linux;
	commandline.read = unix_tcp_read;
	commandline.write = unix_tcp_write;
	
	return SYSOK;
}