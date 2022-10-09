# CC=$(CROSS_COMPILE)gcc
# CFLAGS=-Wall

# all: aesdsocket
# aesdsocket: aesdsocket.o
# aesdsocket.o: aesdsocket.c queue.h

# clean:
# 	rm -f aesdsocket aesdsocket.o
# run: aesdsocket
# 	./aesdsocket


INCLUDE_DIRS = 
LIB_DIRS = -lrt -pthread
CC=$(CROSS_COMPILE)gcc


HFILES= queue.h
CFILES= aesdsocket.c

SRCS= ${HFILES} ${CFILES}

all:	aesdsocket

clean:
	-rm -rf aesdsocket aesdsocket.o

aesdsocket: aesdsocket.o
	$(CC) -o $@ $< -g $(LIB_DIRS)

.c.o: ${SRCS}
	$(CC) -c $^