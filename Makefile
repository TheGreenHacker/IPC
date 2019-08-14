CC = gcc
CFLAGS = -Wall -Wextra

.PHONY: RoutingTable

default: RoutingTable
	$(CC) $(CFLAGS) client.c RoutingTable/RoutingTable.c -o client
	$(CC) $(CFLAGS) server.c RoutingTable/RoutingTable.c -o server

client: RoutingTable
	$(CC) $(CFLAGS) client.c RoutingTable/RoutingTable.c -o client

server: RoutingTable
	$(CC) $(CFLAGS) server.c RoutingTable/RoutingTable.c -o server

RoutingTable:
	$(CC) $(CFLAGS) -c RoutingTable/RoutingTable.c -o RoutingTable/RoutingTable.o

clean:
	rm -f client server RoutingTable/RoutingTable.o
