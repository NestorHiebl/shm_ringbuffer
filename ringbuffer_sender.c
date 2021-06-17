#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include "ringbuffer_extern.h"
#include <semaphore.h>
#include <pthread.h>


int main(int argc, char *argv[]) {

    int ringbuffer_size = -1;
    int option = 0;
    while ((option = getopt(argc, argv, "s:")) != -1) {
        switch (option) {
            case 's':
                ringbuffer_size = (int) strtol(optarg, NULL, 10);
                break;
        
            default:
                fprintf(stderr, "Invalid size supplied to sender\n");
                exit(1);
                break;
        }
    }

    if (ringbuffer_size < 3) {
        exit(1);
    }

    key_t shared_memory_key = 6969;
    
    int shared_memory_flags = IPC_CREAT | 0666;

    int shared_memory_size = get_shared_memory_size(ringbuffer_size);

    // Get the shared memory
    int shared_memory_identifier = 0;
    if ((shared_memory_identifier = shmget(shared_memory_key, shared_memory_size, shared_memory_flags)) == -1) {
        fprintf(stderr, "Sender failed to open shared memory\n");
        perror(NULL);
        exit(1);
    }

    // Attach the shared memory to a specific address
    char *shared_memory_address = NULL;
    shared_memory_address = shmat(shared_memory_identifier, shared_memory_address, 0);
    if (shared_memory_address == (char*) -1) {
        fprintf(stderr, "Sender failed to attach shared memory\n");
        perror(NULL);
        exit(1);
    }

    // Place a semaphore at the start of shared memory
    sem_t *semaphore = (sem_t*) shared_memory_address;

    // Initialize the semaphore
    if (sem_init(semaphore, 1, 0) != 0) {
        fprintf(stderr, "Sender could not initiate semaphore\n");

        // Kill the shared memory before exiting

        if (shmctl(shared_memory_identifier, IPC_RMID, NULL /* The buffer argument is ignored when removing a segment */) == -1) {
            perror("Sender could not close shared memory\n");
        }

        if (shmdt(shared_memory_address) == -1) {
            perror("Sender could not detach shared memory\n");
        }

        exit(1);
    }
    
    // Calculate the starting address of the ringbuffer
    char *ringbuffer_begin = (char*) shared_memory_address + sizeof(sem_t);

    // Main loop
    char stdin_buffer = 0;
    int sem_container = 0;
    int ringbuffer_index = 0;
    while (1) {
        // Check if the ringbuffer is full
        sem_getvalue(semaphore, &sem_container);
        // printf("Semaphore value: %d\n", sem_container);
        if (sem_container > (ringbuffer_size - 1)) {
            // Skip to the next iteration
            usleep(100);
            continue;
        }
        
        stdin_buffer = fgetc(stdin);

        ringbuffer_begin[ringbuffer_index] = stdin_buffer;

        ringbuffer_index++;

        if (ringbuffer_index == ringbuffer_size) {
            ringbuffer_index = 0;
        }

        sem_post(semaphore);
        
        if (stdin_buffer == EOF) {
            break;
        }
    }
    
    // The deinitializations should probably be moved to the receiver, so that no signalling
    // on its side is needed

    // Wait for the receiver to signal that it's done
    while (ringbuffer_begin[0] != 0) {
        usleep(100);
    }
    
    // Start exit procedure
    if (sem_destroy(semaphore) != 0) {
        fprintf(stderr, "Could not destroy semaphore\n");
    }
    
    if (shmctl(shared_memory_identifier, IPC_RMID, NULL /* The buffer argument is ignored when removing a segment */) == -1) {
        perror("Sender could not close shared memory\n");
    }

    if (shmdt(shared_memory_address) == -1) {
        perror("Sender could not detach shared memory\n");
    }
    
    return 0;
}