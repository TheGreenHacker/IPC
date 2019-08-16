#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#include "rtm.h"

int data_socket;
int loop = 1;
int disconnect = 1;

/* Break out of main infinite loop and inform server of intent to disconnect. */
void signal_handler(int signal_num)
{
    if(signal_num == SIGINT)
    {
        loop = 0;
        write(data_socket, &disconnect, sizeof(int));
        close(data_socket);
        exit(0);
    }
}

int main() {
    struct sockaddr_un addr;
    routing_table_t *routing_table;
    int ret;
    int ready_to_update;  // indicates if current state of table is stable
    
    data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data_socket == -1) {
        perror("socket");
        exit(1);
    }
    
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);
    
    ret = connect (data_socket, (const struct sockaddr *) &addr,
                   sizeof(struct sockaddr_un));
    if (ret == -1) {
        fprintf(stderr, "The server is down.\n");
        exit(1);
    }
    
    routing_table = calloc(1, sizeof(routing_table_t));
    init_routing_table(routing_table);
    
    signal(SIGINT, signal_handler);  //register signal handler
    
    /* Continously wait for updates from the routing manager server regarding table contents, stability of
     updates to the table, and server status. */
    while (loop) {
        sync_msg_t sync_msg;
        
        printf("Waiting for sync mesg\n");
        ret = read(data_socket, &sync_msg, sizeof(sync_msg_t));
        if (ret == -1) {
            perror("read");
            break;
        }
        
        printf("Is the table stable?\n");
        ret = read(data_socket, &ready_to_update, sizeof(int));
        if (ret == -1) {
            perror("read");
            break;
        }
        
        printf("Server you still there?\n");
        ret = read(data_socket, &loop, sizeof(int));
        if (ret == -1) {
            perror("read");
            break;
        }
        
        process_sync_mesg(routing_table, &sync_msg);
        if (ready_to_update) {
            display(routing_table);
        }
    }
    
    exit(0);
}
