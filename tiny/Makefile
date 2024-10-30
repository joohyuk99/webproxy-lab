CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: tiny adder

csapp.o:
	$(CC) $(CFLAGS) -c csapp.c

adder: csapp.o
	$(MAKE) -C cgi-bin adder

tiny: adder csapp.o
	$(CC) $(CFLAGS) -o tiny tiny.c csapp.o
#	./tiny 8000

run: tiny
	./tiny 8000

clean:
	rm -f *~ *.o tiny ./cgi-bin/adder
