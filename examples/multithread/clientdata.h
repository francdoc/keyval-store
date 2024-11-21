#include <pthread.h>
#include <stdbool.h>

typedef struct {
	int fd;
	pthread_t thread;
	bool free;
} clientdata_t;

void cd_init(clientdata_t* clients, int len);
int cd_getFreeIndex(clientdata_t* clients, int len);