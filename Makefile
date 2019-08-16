CC = gcc
CFLAGS = -Wall -Wextra

default: rtm
	$(CC) $(CFLAGS) client.c rtm.c -o client
	$(CC) $(CFLAGS) server.c rtm.c -o server

client: rtm
	$(CC) $(CFLAGS) client.c rtm.c -o client

server: rtm
	$(CC) $(CFLAGS) server.c rtm.c -o server

rtm:
	$(CC) $(CFLAGS) -c rtm.c -o rtm.o

clean:
	rm -f client server RoutingTable/RoutingTable.o
