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

void print_help() {
    printf( "Usage:\n"
            "\tringbuffer_sender -s [ringbuffer size];\n"
            "\tringbuffer_receiver -s [ringbuffer size]\n\n"
            "The ringbuffer size argument should be greater than 3. Using different values "
            "for the sender and receiver results in undefined behaviour. The receiver is to "
            "be started second, after the sender. Using the '&' operator to run both processes "
            "in the same line is posible but may lead to a race condition.\n");
}

void get_options(int argc, char *argv[], int *ringbuffer_size) {
        int option = 0;
    while ((option = getopt(argc, argv, "s:h")) != -1) {
        switch (option) {
            case 's':
                *ringbuffer_size = (int) strtol(optarg, NULL, 10);
                break;
            case 'h':
                print_help();
                exit(0);
            default:
                fprintf(stderr, "Invalid argument supplied. Use the -h flag to get help\n");
                exit(1);
        }
    }
}

/**
 *      @brief Calculate the required shared memory size based on the ringbuffer size, assuming that
 *      the ringbuffer will be utilized as an array of char (with width 1) along with a single sem_t
 *      
 *      @param ringbuffer_size The size of the ringbuffer
 *      @return The shared memory size
 */
int get_shared_memory_size(int ringbuffer_size) {
    return (ringbuffer_size * sizeof(char)) + sizeof(sem_t);
}

/**
 *      @brief Get the ringbuffer address assuming that the first sizeof(sem_t) slots in shared memory
 *      are occupied by a semaphore
 *      
 *      @param shared_memory_address The starting address of the shared memory 
 *      @return Char pointer to the starting address of the ringbuffer
 */
char *get_ringbuffer_address(void *shared_memory_address) {
    return (char*) shared_memory_address + sizeof(sem_t);
}


/**
 *      @brief Attempts to destroy the semaphore and mark the shared memory object for deletition
 *      
 *      @param semaphore The semaphore
 *      @param shared_memory_identifier The shared memory identifier
 *      @param shared_memory_address The shared memory address
 */
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