CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: echoclient echoserver

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

echoclient.o: echoclient.c csapp.h
echoserver.o: echoserver.c csapp.h

echoclient: csapp.o echoclient.o
	$(CC) $(CFLAGS) -o echoclient csapp.o echoclient.o

echoserver: csapp.o echoserver.o
	$(CC) $(CFLAGS) -o echoserver csapp.o echoserver.o

clean:
	rm -f *~ *.o