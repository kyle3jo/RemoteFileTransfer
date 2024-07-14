# makefile for server.c, protocol.h, protocol.c
all: server client clean

server: server.o protocol.o
	gcc server.o protocol.o -o server

client: client.o protocol.o
	gcc client.o protocol.o -o client	

server.o: server.c protocol.h
	gcc -c server.c

client.o: client.c protocol.h
	gcc -c client.c	

protocol.o: protocol.c protocol.h
	gcc -c protocol.c

clean:
	rm *o