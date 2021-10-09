CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror

all: aesdsocket

aesdsocket: aesdsocket.o
	 $(CC) $(CFLAGS) -o aesdsocket aesdsocket.o
aesdsocket.o: aesdsocket.c
	 $(CC) $(CFLAGS) -c aesdsocket.c

clean: 
	 rm -f  *.o  \
	        aesdsocket
