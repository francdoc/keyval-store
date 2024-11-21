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

#include <arpa/inet.h>

#include "common/common.h"
#include "common/shell/shell.h"

#include "config.h"

#include "common/syscodes.h"

#include "clientdata.h"

#define MAX_CLIENTS 5 

readwriter_t commandline;
sleeper sleep_nano;

int global_server_sock_fd = 0;
int global_client_sock_fd = 0;

clientdata_t clients[MAX_CLIENTS];

void sleep_nano_linux(int64_t nanoseconds)
{
	double sleep_seconds = (double)(nanoseconds) / 1e9;
	sleep(sleep_seconds);
}

// Reader port.
error unix_tcp_read(byte* buffer, isize* read_len)
{
    ssize_t ret = read(global_client_sock_fd, buffer, *read_len);

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
error unix_tcp_write(byte* buffer, isize* write_len) {
    ssize_t ret = write(global_client_sock_fd, buffer, *write_len);

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

	if (listen(global_server_sock_fd, MAX_CLIENTS) < 0) {
		perror("[CONFIG]: Listen failed");
		close(global_server_sock_fd);
		return ERRSYS;
	}

	cd_init(clients, MAX_CLIENTS);

	printf("[CONFIG]: Server listening.\n");
	return SYSOK;
}

void *handle_client(void *arg) {
    clientdata_t *client = (clientdata_t *)arg;
    int fd = client->fd;

    char buffer[128];
    int n;

    // Blocking read
    if ((n = read(fd, buffer, sizeof(buffer) - 1)) == -1) {
        perror("Error reading from socket");
        close(fd);
        client->free = true;
        return NULL;
    }
    buffer[n] = '\0';
    printf("Received %d bytes: %s\n", n, buffer);

    if (write(fd, "Hola.\n", 5) == -1) {
        perror("Error writing to socket");
        close(fd);
        client->free = true; 
        return NULL;
    }

    close(fd);
    client->free = true;
    return NULL;
}

error acceptconn() {
	struct sockaddr_in clientaddr;
	socklen_t addr_len = sizeof(clientaddr);

	int client_fd = accept(global_server_sock_fd, (struct sockaddr *)&clientaddr, &addr_len);	
	if (client_fd < 0) {
		perror("[CONFIG]: Accept failed.\n");
		return ERRSYS;
	}
	printf("[CONFIG]: Client connected to server.\n");
	printf("[CONFIG]: Client connection from: %s\n", inet_ntoa(clientaddr.sin_addr));

	int index = cd_getFreeIndex(clients, MAX_CLIENTS);
	if (index == -1) {
		perror("[CONFIG]: getFreeIndex function failed.\n");
		return ERRSYS;
	}

	clients[index].fd = client_fd;
	clients[index].free = false;

	if (pthread_create(&clients[index].thread, NULL, handle_client, &clients[index]) != 0) {
		perror("Error creating thread");
		clients[index].free = true;
		close(client_fd);
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