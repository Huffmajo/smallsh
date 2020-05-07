
# -g		adds debugging features
# -Wall		turns on compiler warnings
# -std=c99	compile newer version of C	<--compile in c89 for now
CFLAGS=-g -Wall

CC=gcc

TARGET = smallsh

all: ${TARGET}

${TARGET}: ${TARGET}.c 
	${CC} ${CFLAGS} -o ${TARGET} ${TARGET}.c

clean:
	${RM} ${TARGET}
