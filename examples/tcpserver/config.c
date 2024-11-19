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

readwriter_t commandline;
sleeper sleep_nano;

int global_server_sock_fd = -1;
int global_client_sock_fd = -1;

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
    return 0;
}

// Read port for Unix socket.
error unix_tcp_read(byte* buffer, isize* read_len)
{
    ssize_t ret = read(global_client_sock_fd, buffer, *read_len);

	if (ret > 0) { // "ret" equals to the number of bytes that were read.
		*read_len = ret;
		return 0;
	}

	if (ret < 0) { 
		// An error occurred during the read operation.
		// Reset *read_len is to 0 to prevent main event loop from printing useless data.
		// This ensures no spamming occurs in the event loop if the client does not send any data.
		*read_len = 0;
	}

	if (ret == 0) { // Indicates that the client has closed the socket.
		printf("Connection lost with client, closing program.\n");
		exit(EXIT_FAILURE);
	}

	if (errno == EAGAIN || errno == EWOULDBLOCK) {
		return 0; // No data available in non-blocking mode. Since the socket is configured as non-blocking we let the program continue.
}
}

error setup_tcp_server_config(int port)
{
	struct sockaddr_in server_addr;
	int client_fd;
	socklen_t client_len;
	struct sockaddr_in client_addr;

	close(global_server_sock_fd); // In case it was previosly created.

	global_server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (global_server_sock_fd < 0) {
		perror("Socket creation failed");
		return ERSYS;
	}

	int opt = 1;

	// TODO: remove reusador.
	if (setsockopt(global_server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("Setsockopt failed");
		close(global_server_sock_fd);
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

	client_len = sizeof(client_addr);

	client_fd = accept(global_server_sock_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) {
		perror("Accept failed");
		return ERSYS;
	}
	printf("Client connected to cfg server.\n");

	global_client_sock_fd = client_fd;

	return 0;
}

error closeconn() {
	printf("Closing connection with client.\n");
	if (close(global_client_sock_fd) !=0){
		return ERSYS;
	}
}

error sys_setup(int port)
{
	if (setup_tcp_server_config(port) < 0) {
		printf("Error setup config server.\n");
		return ERSYS;
	}

	if (set_socket_non_blocking(global_client_sock_fd) != 0) {
		printf("Failed to set config config socket to non-blocking mode\n");
		return ERSYS;
	}

	sleep_nano = &sleep_nano_linux;
	commandline.read = unix_tcp_read;
	
	return 0;
}

error process_cmd(byte* cmd, isize len_cmd) {
    char* cmd_set = "SET";
	char* cmd_get = "GET";
    char* cmd_del = "DEL";

    char* key = NULL;
    char* value = NULL;

    char* token = strtok((char*)cmd, " "); // Gets cmd (first word in message from client).

    if (token && strcmp(token, cmd_set) == 0) { // Checks first if token is not NULL.
		key = strtok(NULL, " ");
        value = strtok(NULL, " ");
		printf("Key: %s\n", key);
		printf("Value: %s\n", value);
        
        if (key && value) { // Ensure both key and value are not NULL.
            int fd = open(key, O_CREAT | O_WRONLY | O_TRUNC, 0644); // Create a file with overwrite, read and write access.
            if (fd < 0) {
                perror("Failed to create file");
                return ERSYS;
            }

            ssize_t bytes_written = write(fd, value, strlen(value)); // Write the value to the file.
            if (bytes_written < 0) {
                perror("Failed to write to file");
                close(fd);
                return ERSYS;
            }

            close(fd); // Close the file descriptor after writing.
            printf("File '%s' created successfully.\n", key);
			return 0;
		}
	} 
	
	if (token && strcmp(token, cmd_del) == 0) {
		key = strtok(NULL, " ");
		printf("Key: %s\n", key);

        if (access(key, F_OK) == 0) { // F_OK checks for file existence.
            if (remove(key) == 0) {
                printf("File '%s' deleted successfully.\n", key); 
				return 0;
			} else {
				perror("Failed to delete file");
				return ERSYS;
			}	
		} else {
			// TODO: this requires more handling.
			printf("File '%s' does not exist.\n", key);
		}
	}

	if (token && strcmp(token, cmd_get) == 0) {
		key = strtok(NULL, " ");
		printf("Key: %s\n", key);
		// Open the file for reading
		int fd = open(key, O_RDONLY);
		if (fd < 0) {
			perror("File not found or error opening file");
			// TODO: this requires more handling.
			printf("NOTFOUND\n");
			return ERSYS;
		}

		// Read the content of the file
		char buffer[512];
		ssize_t bytes_read = read(fd, buffer, sizeof(buffer)-1); // Leave 1 byte of space for null-terminator at the end of the buffer.
		if (bytes_read < 0) {
			perror("Error reading file");
			close(fd);
			return ERSYS;
		}
		if (bytes_read > 0) {
			// Null-terminate the buffer to prevent garbage output.
			buffer[bytes_read] = '\0';
			printf("Value: %s\n", buffer);
		}

		// Close the file.
		close(fd);
		return 0;
	}
	
	return ERSYS;	
}