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

char *get_ringbuffer_address(void *shared_memory_address) {
    return (char*) shared_memory_address + sizeof(sem_t);
}

void receiver_attempt_graceful_exit(sem_t *semaphore, int shared_memory_identifier, char *shared_memory_address) {
    if (sem_destroy(semaphore) != 0) {
        fprintf(stderr, "Could not destroy semaphore\n");
    }

    if (shmctl(shared_memory_identifier, IPC_RMID, NULL /* The buffer argument is ignored when removing a segment */) == -1) {
        perror("Sender could not close shared memory\n");
    }

    if (shmdt(shared_memory_address) == -1) {
        perror("Receiver could not detach shared memory\n");
    }
}

void enter_sender_loop(sem_t *semaphore, char *ringbuffer, int ringbuffer_size) {
    int stdin_buffer = 0;
    int sem_container = 0;
    int ringbuffer_index = 0;
    while (1) {
        // Check if the ringbuffer is full
        sem_getvalue(semaphore, &sem_container);
        if (sem_container > (ringbuffer_size - 1)) {
            // Skip to the next iteration
            usleep(100);
            continue;
        }
        
        stdin_buffer = fgetc(stdin);

        ringbuffer[ringbuffer_index] = stdin_buffer;

        ringbuffer_index++;

        if (ringbuffer_index == ringbuffer_size) {
            ringbuffer_index = 0;
        }

        sem_post(semaphore);
        
        if (stdin_buffer == EOF) {
            break;
        }
    }
}

void enter_receiver_loop(sem_t *semaphore, char *ringbuffer, int ringbuffer_size) {
    int ringbuffer_index = 0;
    while (1) {
        sem_wait(semaphore);

        int printchar = ringbuffer[ringbuffer_index];

        if (printchar == EOF) {
            break;
        }

        putchar(printchar);

        ringbuffer_index++;

        if (ringbuffer_index == ringbuffer_size) {
            ringbuffer_index = 0;
        }
    }
}