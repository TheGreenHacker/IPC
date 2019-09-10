#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "DLL/dll.h"
#include "Mac-List/mac-list.h"
#include "Routing-Table/routing-table.h"
#include "Sync/sync.h"

int data_socket;
int loop = 1; // indicates if server is still up and running
int disconnect = 1; // indicates to server that client wants to disconnect

/* Client's copies of network data structures */
dll_t *routing_table;
dll_t *mac_list;

/* Break out of main infinite loop and inform server of intent to disconnect. */
void signal_handler(int signal_num) {
    if (signal_num == SIGINT) {
        loop = 0;
        write(data_socket, &disconnect, sizeof(int));
        close(data_socket);
        deinit_dll(routing_table);
        deinit_dll(mac_list);
        exit(0);
    }
    else if (signal_num == SIGUSR1) {
        deinit_dll(routing_table);
        deinit_dll(mac_list);

        routing_table = init_dll();
        mac_list = init_dll();
    }
}

/* Optionally, display contents of a data stucture (routing table or MAC list). */
void display_ds(int synchronized) {
    char c, flush;
    switch (synchronized) {
        case RT:
            printf("Routing table is up to date. Would you like to see it?(y/n)\n");
            c = getchar();
            scanf("%c", &flush);
            if (c == 'y') {
                display_routing_table(routing_table);
            }
            break;
        case ML:
            printf("MAC list is up to date. Would you like to see it?(y/n)\n");
            c = getchar();
            scanf("%c", &flush);
            if (c == 'y') {
                display_mac_list(mac_list);
            }
            break;
        default:
            break;
    }
}

int main() {
    struct sockaddr_un addr;
    
    routing_table = init_dll();
    mac_list = init_dll();
    
    data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data_socket == -1) {
        perror("socket");
        exit(1);
    }
    
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);
    
    if (connect(data_socket, (const struct sockaddr *) &addr,
                sizeof(struct sockaddr_un)) == -1) {
        fprintf(stderr, "The server is down.\n");
        exit(1);
    }
    
    pid_t pid = getpid();
    write(data_socket, &pid, sizeof(pid_t)); // send server client's process id
    
    signal(SIGINT, signal_handler);  //register signal handler
    signal(SIGUSR1, signal_handler);    
    /* Continously wait for updates to routing table and MAC list from the server regarding table contents and server state. */
    while (loop) {
        int synchronized;
        char ip[IP_ADDR_LEN];
        sync_msg_t *sync_msg = calloc(1, sizeof(sync_msg_t));
        memset(ip, 0, IP_ADDR_LEN);
        
        if (read(data_socket, sync_msg, sizeof(sync_msg_t)) == -1) {
            perror("read");
            break;
        }
        
        if (read(data_socket, &synchronized, sizeof(int)) == -1) {
            perror("read");
            break;
        }
        
        if (read(data_socket, &loop, sizeof(int)) == -1) {
            perror("read");
            break;
        }
        
        if (sync_msg->l_code == L3) {
            process_sync_mesg(routing_table, sync_msg);
        }
        else {
            process_sync_mesg(mac_list, sync_msg);
        }
        
        display_ds(synchronized);
    }
    
    exit(0);
}
