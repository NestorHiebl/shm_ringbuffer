CC = gcc
LINKER = gcc

OBJECTS = ringbuffer_sender.o ringbuffer_receiver.o ringbuffer_extern.o

WARNINGS = -Wall -Wextra -pedantic
DEBUG = -g

CCFLAGS = $(WARNINGS) $(DEBUG)

all: $(OBJECTS)
	$(LINKER) -pthread -o ringbuffer_sender ringbuffer_sender.o ringbuffer_extern.o -lrt
	$(LINKER) -pthread -o ringbuffer_receiver  ringbuffer_receiver.o ringbuffer_extern.o -lrt

ringbuffer_sender.o: ringbuffer_sender.c ringbuffer_extern.c
	$(CC) -c $(CCFLAGS) ringbuffer_sender.c -o ringbuffer_sender.o -lrt

ringbuffer_receiver.o: ringbuffer_receiver.c ringbuffer_extern.c
	$(CC) -c $(CCFLAGS) ringbuffer_receiver.c -o ringbuffer_receiver.o -lrt

ringbuffer_extern.o: ringbuffer_extern.c ringbuffer_extern.h
	$(CC) -c $(CCFLAGS) ringbuffer_extern.c -o ringbuffer_extern.o -lrt

clean:
	rm -f *.o a.out ringbuffer_sender ringbuffer_receiver tempCodeRunner*

test:
	echo
	./ringbuffer_sender -m 50 < text & ./ringbuffer_receiver -m 50
	