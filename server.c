#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#include "rtm.h"

#define MAX_CLIENTS 32
#define OP_LEN 128
#define MAX_MASK 32

int connection_socket;
int loop = 1;
routing_table_t *routing_table;

/*An array of File descriptors which the server process
 * is maintaining in order to talk with the connected clients.
 * Master skt FD is also a member of this array*/
int monitored_fd_set[MAX_CLIENTS];

/*Remove all the FDs, if any, from the the array*/
static void intitiaze_monitor_fd_set(){
    int i = 0;
    for(; i < MAX_CLIENTS; i++)
        monitored_fd_set[i] = -1;
}


/*Add a new FD to the monitored_fd_set array*/
static void add_to_monitored_fd_set(int skt_fd){
    int i = 0;
    for(; i < MAX_CLIENTS; i++){
        if(monitored_fd_set[i] != -1)
            continue;
        monitored_fd_set[i] = skt_fd;
        break;
    }
}


/*Remove the FD from monitored_fd_set array*/
static void remove_from_monitored_fd_set(int skt_fd){
    int i = 0;
    for(; i < MAX_CLIENTS; i++){
        if(monitored_fd_set[i] != skt_fd)
            continue;
        monitored_fd_set[i] = -1;
        break;
    }
}


/* Clone all the FDs in monitored_fd_set array into
 * fd_set Data structure*/
static void refresh_fd_set(fd_set *fd_set_ptr){
    FD_ZERO(fd_set_ptr);
    int i = 0;
    for(; i < MAX_CLIENTS; i++){
        if(monitored_fd_set[i] != -1){
            FD_SET(monitored_fd_set[i], fd_set_ptr);
        }
    }
}


int digits_only(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return 0;
    }
    
    return 1;
}


static int isValidIP(const char *addr) {
    struct sockaddr_in sa;
    return addr && inet_pton(AF_INET, addr, &(sa.sin_addr)) != 0;
}


/* Mask is valid if it begins with a /, the rest of the string following the / contains only digits, and the numerical value of the rest of the string is between 0 and 32, inclusive. */
static int isValidMask(const char *mask) {
    return digits_only(mask) && atoi(mask) <= 32;
}

/*Get the numerical max value among all FDs which server
 * is monitoring*/
static int get_max_fd(){
    int i = 0;
    int max = -1;
    
    for(; i < MAX_CLIENTS; i++){
        if(monitored_fd_set[i] > max)
            max = monitored_fd_set[i];
    }
    
    return max;
}


/* Parses a string command, in the format <Opcode, Dest, Mask, GW, OIF> with each field separated by a space, from the routing table manager to create a sync message for clients, instructing them on how to update their copies of the routing table. */
static int create_sync_message(char *operation, sync_msg_t *sync_msg) {
    char *token = strtok(operation, " ");
    if (token) {
        switch (token[0]) {
            case 'C':
                sync_msg->op_code = CREATE;
                break;
            case 'U':
                sync_msg->op_code = UPDATE;
                break;
            case 'D':
                sync_msg->op_code = DELETE;
                break;
            case 'S':
                sync_msg->op_code = DUMMY; // to prevent process_sync_msg from printing that we inserted a dummy node
                display(routing_table);
                return 0;
            default:
                fprintf(stderr, "Invalid operation: unknown op code\n");
                return -1;
        }
    }
    else {
        fprintf(stderr, "Invalid operation: missing op code\n");
        return -1;
    }
    
    token = strtok(NULL, " ");
    if (isValidIP(token)) {
        memcpy(sync_msg->contents.dest, token, strlen(token));
    }
    else {
        fprintf(stderr, "Invalid operation: invalid or missing destination IP\n");
        return -1;
    }
    
    token = strtok(NULL, " ");
    if (isValidMask(token)) {
        sync_msg->contents.mask = atoi(token);
    }
    else {
        fprintf(stderr, "Invalid operation: invalid or missing subnet mask value\n");
        return -1;
    }
    
    /* Only CREATE and UPDATE require a gw and oif*/
    if (sync_msg->op_code == CREATE || sync_msg->op_code == UPDATE) {
        token = strtok(NULL, " ");
        if (isValidIP(token)) {
            memcpy(sync_msg->contents.gw, token, strlen(token));
        }
        else {
            fprintf(stderr, "Invalid operation: invalid or missing gateway IP\n");
            return -1;
        }
        
        token = strtok(NULL, " ");
        if (token) {
            memcpy(sync_msg->contents.oif, token, strlen(token));
        }
        else {
            fprintf(stderr, "Invalid operation: missing OIF\n");
            return -1;
        }
    }
    
    return 0;
}

/* Break out of main infinite loop and inform clients of shutdown to exit cleanly. */
void signal_handler(int signal_num)
{
    if(signal_num == SIGINT)
    {
        int i, ready_to_update = 0, loop = 0;
        sync_msg_t sync_msg;
        sync_msg.op_code = DUMMY;
        for(i = 2; i < MAX_CLIENTS; i++){
            int comm_socket_fd = monitored_fd_set[i];
            if (comm_socket_fd != -1) {
                write(comm_socket_fd, &sync_msg, sizeof(sync_msg_t));
                write(comm_socket_fd, &ready_to_update, sizeof(int));
                write(comm_socket_fd, &loop, sizeof(int));
            }
        }
        
        /* Clean up resources */
        deinit_routing_table(routing_table);
        free(routing_table);
        close(connection_socket);
        remove_from_monitored_fd_set(connection_socket);
        unlink(SOCKET_NAME);
        exit(0);
    }
}


int main() {
    struct sockaddr_un name;
    int ret;
    int data_socket;
    int ready_to_update;  // indicates to client if current table state is stable
    fd_set readfds;
    
    intitiaze_monitor_fd_set();
    add_to_monitored_fd_set(0);
    
    /*In case the program exited inadvertently on the last run,
     *remove the socket.
     **/
    
    unlink(SOCKET_NAME);
    
    /* master socket for accepting connections from client */
    connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connection_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    /*initialize*/
    memset(&name, 0, sizeof(struct sockaddr_un));
    
    /*Specify the socket Cridentials*/
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);
    
    /* Bind socket to socket name.*/
    ret = bind(connection_socket, (const struct sockaddr *) &name,
               sizeof(struct sockaddr_un));
    if (ret == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /* Prepare for accepting connections.  */
    ret = listen(connection_socket, 20);
    if (ret == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    add_to_monitored_fd_set(connection_socket);
    
    signal(SIGINT, signal_handler);  //register signal handlers
    
    /* The server continuously checks for new client connections, monitors existing connections (i.e. incoming messages and inactive connections, modifies the routing table if need be, and broadcasts any changes to all client processes. */
    routing_table = calloc(1, sizeof(routing_table_t));
    init_routing_table(routing_table);
    while (1) {
        char operation[OP_LEN];
        sync_msg_t sync_msg;
        memset(&sync_msg, 0, sizeof(sync_msg_t));
        
        ready_to_update = 0;

        refresh_fd_set(&readfds); /*Copy the entire monitored FDs to readfds*/
        
        printf("Please select from the following options:\n");
        printf("1.CREATE <Destination IP> <Mask (0-32)> <Gateway IP> <OIF>\n");
        printf("2.UPDATE <Destination IP> <Mask (0-32)> <New Gateway IP> <New OIF>\n");
        printf("3.DELETE <Destination IP> <Mask (0-32)>\n");
        printf("4.SHOW\n");
        
        // display(routing_table);
        
        // printf("Select...\n");
        
        select(get_max_fd() + 1, &readfds, NULL, NULL, NULL);  /* Wait for incoming connections. */

        /* New client connection: send entire table state to newly connected client. */
        if(FD_ISSET(connection_socket, &readfds)){
            printf("Got a client\n");
            data_socket = accept(connection_socket, NULL, NULL);
            if (data_socket == -1) {
                perror("accept");
                exit(1);
            }
            
            add_to_monitored_fd_set(data_socket);
            
            routing_table_entry_t *entry = routing_table->head;
            while (entry->contents->mask != DUMMY_MASK) {
                printf("Entry...\n");
    
                sync_msg.op_code = CREATE;
                sprintf(operation, "C %s %u %s %s", entry->contents->dest, entry->contents->mask, entry->contents->gw, entry->contents->oif);
                create_sync_message(operation, &sync_msg);
                write(data_socket, &sync_msg, sizeof(sync_msg_t));
                write(data_socket, &ready_to_update, sizeof(int));
                write(data_socket, &loop, sizeof(int));
                
                entry = entry->next;
            }
            ready_to_update = 1;
            
            sync_msg.op_code = DUMMY;
            write(data_socket, &sync_msg, sizeof(sync_msg_t));
            write(data_socket, &ready_to_update, sizeof(int));
            write(data_socket, &loop, sizeof(int));
            
            printf("All current entries synchronized to new client.\n");
        }
        else if(FD_ISSET(0, &readfds)){ // update from routing table manager via stdin
            printf("Manager has some changes to make\n");
            ret = read(0, operation, OP_LEN - 1);
            operation[strcspn(operation, "\r\n")] = 0; // flush new line
            if (ret == -1) {
                perror("read");
                return 1;
            }
            operation[ret] = 0;
            
            //printf("Operation : %s\n", operation);
            
            if (!create_sync_message(operation, &sync_msg)) {
                process_sync_mesg(routing_table, &sync_msg); // update server's table

                /* Notify existing clients of changes */
                int i, comm_socket_fd;
                ready_to_update = 1;
                for (i = 2; i < MAX_CLIENTS; i++) {
                    comm_socket_fd = monitored_fd_set[i];
                    if (comm_socket_fd != -1) {
                        write(comm_socket_fd, &sync_msg, sizeof(sync_msg));
                        write(comm_socket_fd, &ready_to_update, sizeof(int));
                        write(comm_socket_fd, &loop, sizeof(int));
                    }
                }
                printf("Entry synchronized to all connected clients.\n");
            }
        }
        else { /* Check active status of clients */
            printf("Client activity detected\n");
            int i;
            for(i = 2; i < MAX_CLIENTS; i++){
                if(FD_ISSET(monitored_fd_set[i], &readfds)){
                    int done;
                    int comm_socket_fd = monitored_fd_set[i];
                    
                    ret = read(comm_socket_fd, &done, sizeof(int));
                    if (done == 1) {
                        close(comm_socket_fd);
                        remove_from_monitored_fd_set(comm_socket_fd);
                    }
                    else if (ret == -1) {
                        perror("read");
                        exit(1);
                    }
                    else {
                        printf("%i\n", done);
                    }
                }
            }
        }
    }
    exit(0);
}
