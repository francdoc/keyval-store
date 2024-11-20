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
int global_client_sock_fd = 0;

void sleep_nano_linux(int64_t nanoseconds)
{
	double sleep_seconds = (double)(nanoseconds) / 1e9;
	sleep(sleep_seconds);
}

error set_socket_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return ERSYS;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        return ERSYS;
    }
    return SYSOK;
}

error unix_tcp_read(byte* buffer, isize* read_len)
{
    ssize_t ret = read(global_client_sock_fd, buffer, *read_len);

	if (ret > 0) { // "ret" equals to the number of bytes that were read.
		*read_len = ret;
		return SYSOK;
	}

	if (ret < 0) { 
		// An error occurred during the read operation.
		// *read_len = 0 is to prevent main event loop from printing useless data.
		// This ensures no spamming occurs in the event loop if the client does not send any data.
		*read_len = 0;
	}

	if (ret == 0) { // Indicates that the client has closed the socket.
		printf("Connection lost with client, closing program.\n");
		exit(EXIT_FAILURE);
	}

	if (errno == EAGAIN || errno == EWOULDBLOCK) { 
		// No data available in non-blocking mode. 
		// Since the socket is configured as non-blocking we let the program continue normally.
		return SYSOK; 
	}
}

error unix_tcp_write(byte* buffer, isize* write_len) {
    ssize_t ret = write(global_client_sock_fd, buffer, *write_len);

    if (ret > 0) { 
        *write_len = ret;
        return SYSOK;
    }

    *write_len = 0; // Reset the write length on error.
    perror("Error writing to socket");
    return ERSYS;
}

error setup_tcp_server_config(int port)
{
	struct sockaddr_in server_addr;

	close(global_server_sock_fd); // In case it was previosly created.

	global_server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (global_server_sock_fd < 0) {
		perror("Socket creation failed");
		return ERSYS;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(global_server_sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Socket bind failed");
		close(global_server_sock_fd);
		return ERSYS;
	}

	if (listen(global_server_sock_fd, 1) < 0) {
		perror("Listen failed");
		close(global_server_sock_fd);
		return ERSYS;
	}
	printf("Socket listening.\n");
	return SYSOK;
}

error acceptconn() {
	int client_fd;
	socklen_t client_len;
	struct sockaddr_in client_addr;

	client_len = sizeof(client_addr);
	client_fd = accept(global_server_sock_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) {
		perror("Accept failed");
		return ERSYS;
	}
	printf("Client connected to cfg server.\n");
	
	global_client_sock_fd = client_fd;
	return SYSOK;
}

error closeconn() {
	printf("Closing connection with client.\n");
	if (close(global_client_sock_fd) !=0){
		return ERSYS;
	}
}

error sys_setup(int port)
{
	if (set_socket_non_blocking(global_client_sock_fd) != 0) {
		printf("Failed to set config config socket to non-blocking mode\n");
		return ERSYS;
	}

	if (setup_tcp_server_config(port) < 0) {
		printf("Error setup config server.\n");
		return ERSYS;
	}

	sleep_nano = &sleep_nano_linux;
	commandline.read = unix_tcp_read;
	commandline.write = unix_tcp_write;
	
	return SYSOK;
}