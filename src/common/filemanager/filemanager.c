#include <fcntl.h>

#include "common/common.h"
#include "common/syscodes.h"
#include "common/filemanager/filemanager.h"

filemanager_t new_filemanager(char* cmd_set, char* cmd_get, char* cmd_del, char* cmd_separator, isize read_buffsize) {
	filemanager_t flm;
	zero(&flm);
	flm.cmd_set = cmd_set;
	flm.cmd_get = cmd_get;
	flm.cmd_del = cmd_del;
	flm.cmd_separator = cmd_separator;
	flm.read_buffsize = read_buffsize;
	return flm;
}

error filemanager_process_cmd(filemanager_t flm, byte* cmd, isize len_cmd, char* read_val) {
	char* key = NULL;
	char* value = NULL;

	char* token = strtok((char*)cmd, flm.cmd_separator);

	if (token && strcmp(token, flm.cmd_set) == 0) {
		key = strtok(NULL, flm.cmd_separator);
		value = strtok(NULL, flm.cmd_separator);
		printf("Key: %s\n", key);
		printf("Value: %s\n", value);
		
		if (key && value) {
			int fd = open(key, O_CREAT | O_WRONLY | O_TRUNC, 0644);
			if (fd < 0) {
				perror("Failed to create file");
				return ERSYS;
			}

			ssize_t bytes_written = write(fd, value, strlen(value));
			if (bytes_written < 0) {
				perror("Failed to write to file");
				close(fd);
				return ERSYS;
			}

			close(fd);
			printf("File '%s' created successfully.\n", key);
			return SYSOK;
		}
	} 

	if (token && strcmp(token, flm.cmd_del) == 0) {
		key = strtok(NULL, flm.cmd_separator);
		printf("Key: %s\n", key);

		if (access(key, F_OK) == 0) {
			if (remove(key) == 0) {
				printf("File '%s' deleted successfully.\n", key); 
				return SYSOK;
			} else {
				perror("Failed to delete file");
				return ERSYS;
			}	
		} else {
			printf("File '%s' does not exist.\n", key);
			return SYSOK;
		}
	}

	if (token && strcmp(token, flm.cmd_get) == 0) {
		key = strtok(NULL, flm.cmd_separator);
		printf("Key: %s\n", key);

		int fd = open(key, O_RDONLY);
		if (fd < 0) {
			perror("File not found or error opening file");
			return ERFNOTFOUND;
		}

		// Read the content of the file
		ssize_t bytes_read = read(fd, read_val, flm.read_buffsize - 1); // Leave 1 byte of space for null-terminator at the end of the buffer.
		if (bytes_read < 0) {
			perror("Error reading file");
			close(fd);
			return ERSYS;
		}
		if (bytes_read > 0) {
			read_val[bytes_read] = '\0'; // Null-terminate the buffer to prevent garbage output.
			printf("Value: %s\n", read_val); 
		}

		close(fd); 
		return RETURNVAL;
	}

	return ERSYS;	
}