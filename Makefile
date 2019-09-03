CC = gcc
CFLAGS = -Wall -Wextra
DEPSRC = DLL/dll.c Mac-List/mac-list.c Routing-Table/routing-table.c Sync/sync.c

.PHONY: dll mac-list routing-table sync

default: dll mac-list routing-table sync
	$(CC) $(CFLAGS) client.c $(DEPSRC) -o client
	$(CC) $(CFLAGS) server.c $(DEPSRC) -o server

client: dll mac-list routing-table sync
	$(CC) $(CFLAGS) client.c $(DEPSRC) -o client

server: dll mac-list routing-table sync
	$(CC) $(CFLAGS) server.c $(DEPSRC) -o server

dll:
	$(CC) $(CFLAGS) -c DLL/dll.c -o DLL/dll.o
	# $(CC) $(CFLAGS) DLL/main.c DLL/dll.c -o DLL/main

mac-list: dll
	$(CC) $(CFLAGS) -c Mac-List/mac-list.c -o Mac-List/mac-list.o

routing-table: dll
	$(CC) $(CFLAGS) -c Routing-Table/routing-table.c -o  Routing-Table/routing-table.o

sync: dll mac-list routing-table
	$(CC) $(CFLAGS) -c Sync/sync.c -o Sync/sync.o

clean:
	rm -f client server DLL/dll.o Mac-List/mac-list.o Routing-Table/routing-table.o
