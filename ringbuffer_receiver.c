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
                fprintf(stderr, "Invalid size supplied to receiver\n");
                exit(1);
                break;
        }
    }
    
    if (ringbuffer_size < 3) {
        fprintf(stderr, "Invalid size supplied to sender\n");
        exit(1);
    }

    key_t shared_memory_key = 6969;
    
    int shared_memory_flags = 0660;

    int shared_memory_size = get_shared_memory_size(ringbuffer_size);

    // Get the (already initialized) shared memory (race condition here)
    sleep(1); // AWFUL solution
    int shared_memory_identifier = 0;
    if ((shared_memory_identifier = shmget(shared_memory_key, shared_memory_size, shared_memory_flags)) == -1) {
        perror("Receiver failed to open shared memory\n");
        exit(1);
    }

    // Attach the shared memory to a concrete address
    char *shared_memory_address = NULL;
    shared_memory_address = shmat(shared_memory_identifier, shared_memory_address, 0);
    if (shared_memory_address == (char*) -1) {
        perror("Receiver failed to attach shared memory\n");
        exit(1);
    }
    
    // Get the semaphore (initialized by the sender)
    sem_t *semaphore = (sem_t*) shared_memory_address;

    // Calculate the ringbuffer's starting address    
    char *ringbuffer_begin = get_ringbuffer_address(shared_memory_address);

    // Main loop
    enter_receiver_loop(semaphore, ringbuffer_begin, ringbuffer_size);
    
    // Start exit procedure
    receiver_attempt_graceful_exit(semaphore, shared_memory_identifier, shared_memory_address);

    return 0;
}