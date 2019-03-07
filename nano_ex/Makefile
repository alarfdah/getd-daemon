all: client server

client: client.c
	gcc -o $@ $< -l nanomsg

server: server.c
	gcc -o $@ $< -l nanomsg
