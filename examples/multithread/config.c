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

#include "common/filemanager/filemanager.h"

#define MAX_CLIENTS 5 

#define shellBufferSize 512 // bytes
#define filemanagerBufferSize shellBufferSize // bytes

readwriter_t commandline;
sleeper sleep_nano;

int global_server_sock_fd = 0;

clientdata_t clients[MAX_CLIENTS];

byte bufferRead[shellBufferSize];
byte bufferWrite[shellBufferSize];

#define microseconds (1000)
#define milliseconds (1000 * microseconds)
#define seconds (1000 * milliseconds)

void sleep_nano_linux(int64_t nanoseconds)
{
	double sleep_seconds = (double)(nanoseconds) / 1e9;
	sleep(sleep_seconds);
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
	// TODO: adapt shell to take fd
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
	filemanager_t flm;
    flm = new_filemanager("SET", "GET", "DEL", " ", filemanagerBufferSize);
    
	error err;
	isize totalRead;

    clientdata_t *client = (clientdata_t *)arg;
    int fd = client->fd;

	shell_t s = shell_new(commandline, sizeof(bufferRead), fd);
	printf("[CONFIG]: Shell ready.\n");

	bool client_connected = true;

    while (true) {	
		totalRead = shellBufferSize; // Buffer must be set for each read cycle.
		
		err = shell_read(&s, bufferRead, &totalRead);
		if (err != 0) {
			printf("[CONFIG]: Error reading shell. Closing program.\n");
			close(fd);
			exit(EXIT_FAILURE);
		}

		if (totalRead > 0) {
			printf("[CONFIG]: Received cmd: %s\n", bufferRead);

			char read_val[filemanagerBufferSize] = {0};
			err = filemanager_process_cmd(flm, bufferRead, totalRead, read_val);
							
			if (err == SYSOK) { // If the processed command was valid and successfully handled, close the connection with the client.
				snprintf((char*) bufferWrite, sizeof(bufferWrite), "OK\n");
				isize write_len = strlen((char*)bufferWrite);
				
				err = shell_write(&s, bufferWrite, &write_len);
				if (err != SYSOK) {
					printf("[CONFIG]: Debug I: shell_write returned %d\n", err);
					printf("[CONFIG]: Error writing shell. Closing program.\n");
					close(fd);
					exit(EXIT_FAILURE);
				}
			}
			else if (err == ERRFILE) {
				snprintf((char*)bufferWrite, sizeof(bufferWrite), "NOTFOUND\n");
				isize write_len = strlen((char*)bufferWrite);
				
				err = shell_write(&s, bufferWrite, &write_len);
				if (err != SYSOK) {
					printf("[CONFIG]: Debug II: shell_write returned %d\n", err);
					printf("[CONFIG]: Error writing shell. Closing program.\n");
					close(fd);
					exit(EXIT_FAILURE);
				}
			}
			else if (err == RETURNVAL) {
				size_t fixed_overhead = strlen("OK\n") + strlen("\n") + 1; // To avoid warning related to truncated writing.
				size_t max_read_val_length = sizeof(bufferWrite) - fixed_overhead;
				snprintf((char*)bufferWrite, sizeof(bufferWrite), "OK\n%.*s\n", (int)max_read_val_length, read_val);
				isize write_len = strlen((char*)bufferWrite);
				
				err = shell_write(&s, bufferWrite, &write_len);
				if (err != SYSOK) {
					printf("[CONFIG]: Debug III: shell_write returned %d\n", err);
					printf("[CONFIG]: Error writing shell. Closing program.\n");
					close(fd);
					exit(EXIT_FAILURE);
				}
			}
			else if (err == ERRSYS) {
				printf("[CONFIG]: Command not found, ending program.\n");
				close(fd);
				exit(EXIT_FAILURE);                
			}

			// If we got here then we are good to close the connection to the client.
			close(fd);
			client_connected = false;
		}
		sleep_nano(500 * microseconds);
		if (client_connected == false) {
				break;
		}
	}
    client->free = true;
    return NULL;
}

error sys_update() {
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