CC = gcc
CFLAGS = -Wall -Wextra
DEPSRC = DLL/dll.c Mac-List/mac-list.c Routing-Table/routing-table.c Sync/sync.c

.PHONY: dll mac-list routing-table sync shm_ip

default: dll mac-list routing-table sync shm_ip
	$(CC) $(CFLAGS) client.c shm_ip.c $(DEPSRC) -o client -lrt
	$(CC) $(CFLAGS) server.c shm_ip.c $(DEPSRC) -o server -lrt

client: dll mac-list routing-table sync shm_ip
	$(CC) $(CFLAGS) client.c shm_ip.c $(DEPSRC) -o client -lrt

server: dll mac-list routing-table sync shm_ip
	$(CC) $(CFLAGS) server.c shm_ip.c $(DEPSRC) -o server -lrt

dll:
	$(CC) $(CFLAGS) -c DLL/dll.c -o DLL/dll.o
	# $(CC) $(CFLAGS) DLL/main.c DLL/dll.c -o DLL/main

mac-list: dll shm_ip
	$(CC) $(CFLAGS) -c Mac-List/mac-list.c -o Mac-List/mac-list.o

routing-table: dll
	$(CC) $(CFLAGS) -c Routing-Table/routing-table.c -o  Routing-Table/routing-table.o

sync: dll mac-list routing-table shm_ip
	$(CC) $(CFLAGS) -c Sync/sync.c -o Sync/sync.o

shm_ip:
	$(CC) $(CFLAGS) -c shm_ip.c -o shm_ip.o

clean:
	rm -f client server DLL/dll.o Mac-List/mac-list.o Routing-Table/routing-table.o shm_ip.o
