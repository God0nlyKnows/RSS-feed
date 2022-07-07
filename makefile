TARGET=main3
CC=gcc
CFLAGS= 
OBJS=main.o list.o -lpthread -lcurl
SRCS=main.c list.c

${TARGET}: ${OBJS}
	${CC} ${CFLAGS} -o ${TARGET} ${OBJS}

main.o: main.c list.h
list.o: list.c list.h