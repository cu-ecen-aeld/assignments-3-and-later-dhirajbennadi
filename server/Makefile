INCLUDE_DIRS = 
LIB_DIRS = 
#CC=aarch64-none-linux-gnu-gcc
#CC=gcc

# ifeq ($(TARGET),CROSS_COMPILE)
#         CC = aarch64-none-linux-gnu-gcc
# else
#         CC = gcc
# endif

CC=$(CROSS_COMPILE)gcc


HFILES= 
CFILES= aesdsocket.c

SRCS= ${HFILES} ${CFILES}

all:	aesdsocket

clean:
	-rm -rf aesdsocket aesdsocket.o

aesdsocket: aesdsocket.o
	$(CC) -o $@ $< -g

.c.o:
	$(CC) -c $<