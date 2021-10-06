CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -pthread

all: aesdsocket

aesdsocket: aesdsocket.o
	 $(CC) $(CFLAGS) -o aesdsocket aesdsocket.o
aesdsocket.o: aesdsocket.c
	 $(CC) $(CFLAGS) -c aesdsocket.c queue.h

clean: 
	 rm -f  *.o  \
	        aesdsocket
