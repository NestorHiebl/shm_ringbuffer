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
#include <semaphore.h>
#include <pthread.h>

#ifndef RINGBUFFER_EXTERN
#define RINGBUFFER_EXTERN



extern int get_shared_memory_size(int ringbuffer_size);

extern void sender_attempt_graceful_exit();
extern void receiver_attempt_graceful_exit();

extern void enter_sender_loop(sem_t *semaphore, char *ringbuffer, int ringbuffer_size);
extern void enter_receiver_loop(sem_t *semaphore, char *ringbuffer, int ringbuffer_size);

#endif