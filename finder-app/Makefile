INCLUDE_DIRS = 
LIB_DIRS = 
#CC=aarch64-none-linux-gnu-gcc
CC=gcc


HFILES= 
CFILES= writer.c

SRCS= ${HFILES} ${CFILES}

all:	writer

clean:
	-rm -rf writer writer.o

writer: writer.o
	$(CC) -o $@ $<

.c.o:
	$(CC) -c $<