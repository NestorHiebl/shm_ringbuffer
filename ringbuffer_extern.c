#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include "ringbuffer_extern.h"
#include <pthread.h>
#include <semaphore.h>

int get_shared_memory_size(int ringbuffer_size) {
    return (ringbuffer_size * sizeof(char)) + sizeof(sem_t);
}