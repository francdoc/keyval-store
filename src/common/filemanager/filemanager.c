#include <fcntl.h>

#include "common/common.h"
#include "common/syserrors.h"

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