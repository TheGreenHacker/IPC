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

#include "DLL/dll.h"
#include "Mac-List/mac-list.h"
#include "Routing-Table/routing-table.h"
#include "Sync/sync.h"

#define MAX_CLIENTS 32
#define OP_LEN 128
#define MAX_MASK 32

/* Indicates to client if any data structure has been synchronized.
 WAIT: none
 RT: routing table
 ML: MAC list
 */
int synchronized;
int connection_socket;
int loop = 1;
dll_t *routing_table;
dll_t *mac_list;

/*An array of File descriptors which the server process
 * is maintaining in order to talk with the connected clients.
 * Master skt FD is also a member of this array*/
int monitored_fd_set[MAX_CLIENTS];

/*Remove all the FDs, if any, from the the array*/
void intitiaze_monitor_fd_set(){
    int i = 0;
    for(; i < MAX_CLIENTS; i++)
        monitored_fd_set[i] = -1;
}


/*Add a new FD to the monitored_fd_set array*/
void add_to_monitored_fd_set(int skt_fd){
    int i = 0;
    for(; i < MAX_CLIENTS; i++){
        if(monitored_fd_set[i] != -1)
            continue;
        monitored_fd_set[i] = skt_fd;
        break;
    }
}


/*Remove the FD from monitored_fd_set array*/
void remove_from_monitored_fd_set(int skt_fd){
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
void refresh_fd_set(fd_set *fd_set_ptr){
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


int isValidIP(const char *addr) {
    struct sockaddr_in sa;
    return addr && inet_pton(AF_INET, addr, &(sa.sin_addr)) != 0;
}


/* Mask is valid if it begins with a /, the rest of the string following the / contains only digits, and the numerical value of the rest of the string is between 0 and 32, inclusive. */
int isValidMask(const char *mask) {
    return digits_only(mask) && atoi(mask) <= 32;
}

int isValidMAC(const char *addr) {
    int i;
    for(i = 0; i < MAC_ADDR_LEN - 1; i++) {
        if(i % 3 != 2 && !isxdigit(addr[i]))
            return 0;
        if(i % 3 == 2 && addr[i] != ':')
            return 0;
    }
    return addr[i] == '\0';
}

/*Get the numerical max value among all FDs which server
 * is monitoring*/
int get_max_fd(){
    int i = 0;
    int max = -1;
    
    for(; i < MAX_CLIENTS; i++){
        if(monitored_fd_set[i] > max)
            max = monitored_fd_set[i];
    }
    
    return max;
}


/* Parses a string command, in the format <Opcode, Dest, Mask, GW, OIF> with each field separated by a space, from the routing table manager to create a sync message for clients, instructing them on how to update their copies of the routing table. */
int create_sync_message(char *operation, sync_msg_t *sync_msg) {
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
                sync_msg->op_code = NONE;
                display_routing_table(routing_table);
                display_mac_list(mac_list);
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
        sync_msg->l_code = L3;
        memcpy(sync_msg->msg_body.routing_table_entry.dest, token, strlen(token));
    }
    else if (isValidMAC(token)) {
        sync_msg->l_code = L2;
        memcpy(sync_msg->msg_body.mac_list_entry.mac, token, strlen(token));
        return 0;
    }
    else {
        fprintf(stderr, "Invalid operation: invalid or missing destination IP/MAC address\n");
        return -1;
    }
    
    token = strtok(NULL, " ");
    if (isValidMask(token)) {
        sync_msg->msg_body.routing_table_entry.mask = atoi(token);
    }
    else {
        fprintf(stderr, "Invalid operation: invalid or missing subnet mask value\n");
        return -1;
    }
    
    /* Only CREATE and UPDATE require a gw and oif*/
    if (sync_msg->op_code == CREATE || sync_msg->op_code == UPDATE) {
        token = strtok(NULL, " ");
        if (isValidIP(token)) {
            memcpy(sync_msg->msg_body.routing_table_entry.gw, token, strlen(token));
        }
        else {
            fprintf(stderr, "Invalid operation: invalid or missing gateway IP\n");
            return -1;
        }
        
        token = strtok(NULL, " ");
        if (token) {
            memcpy(sync_msg->msg_body.routing_table_entry.oif, token, strlen(token));
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
        int i, synchronized = WAIT, loop = 0;
        sync_msg_t sync_msg;
        sync_msg.op_code = NONE;
        for(i = 2; i < MAX_CLIENTS; i++){
            int comm_socket_fd = monitored_fd_set[i];
            if (comm_socket_fd != -1) {
                write(comm_socket_fd, &sync_msg, sizeof(sync_msg_t));
                write(comm_socket_fd, &synchronized, sizeof(int));
                write(comm_socket_fd, &loop, sizeof(int));
            }
        }
        
        /* Clean up resources */
        deinit_dll(routing_table);
        deinit_dll(mac_list);
        close(connection_socket);
        remove_from_monitored_fd_set(connection_socket);
        unlink(SOCKET_NAME);
        exit(0);
    }
}

/* Send newly client all necessary CREATE commands to replicate the server's copies of the current routing table or MAC list. */
void update_new_client(int data_socket, LCODE l_code, char *op, sync_msg_t *sync_msg) {
    dll_node_t *head = LCODE l_code == L3 ? routing_table->head : mac_list->head;
    if (head->data) {
        printf("fuck me\n");
    }
    dll_node_t *curr = head->next;
    if (curr == head) {
        printf("WTFFFF\n");
    }
    //display_routing_table(routing_table);
    //printf("why the fuck is second client not getting any updates?\n");    
    while (curr != head) {
        printf("Second client, do you read me\n");
        routing_table_entry_t rt_entry = *((routing_table_entry_t *) curr->data);
        mac_list_entry_t ml_entry = *((mac_list_entry_t *) curr->data);
        
        sync_msg->op_code = CREATE;
        if (l_code == L3) {
            sprintf(op, "C %s %u %s %s", rt_entry.dest, rt_entry.mask, rt_entry.gw, rt_entry.oif);
        }
        else {
            sprintf(op, "C %s", ml_entry.mac);
        }
        
        create_sync_message(op, sync_msg);
        
        write(data_socket, sync_msg, sizeof(sync_msg_t));
        write(data_socket, &synchronized, sizeof(int));
        write(data_socket, &loop, sizeof(int));
        
        curr = curr->next;
    }
    
    /* Send dummy sync message to inform client that all current updates have been sent. */
    sync_msg->op_code = NONE;
    write(data_socket, sync_msg, sizeof(sync_msg_t));
    synchronized = l_code == L3 ? RT : ML;
    write(data_socket, &synchronized, sizeof(int));
    write(data_socket, &loop, sizeof(int));
}


int main() {
    struct sockaddr_un name;
    int ret;
    int data_socket;
    fd_set readfds;
    
    routing_table = init_dll();
    mac_list = init_dll();
    
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
    while (1) {
        char op[OP_LEN];
        sync_msg_t *sync_msg = calloc(1, sizeof(sync_msg_t));
        synchronized = 0;

        refresh_fd_set(&readfds); /*Copy the entire monitored FDs to readfds*/
        
        printf("Please select from the following options:\n");
        printf("1.CREATE <Destination IP> <Mask (0-32)> <Gateway IP> <OIF>\n");
        printf("2.UPDATE <Destination IP> <Mask (0-32)> <New Gateway IP> <New OIF>\n");
        printf("3.DELETE <Destination IP> <Mask (0-32)>\n");
        printf("4.SHOW\n");
        
        select(get_max_fd() + 1, &readfds, NULL, NULL, NULL);  /* Wait for incoming connections. */

        /* New connection: send entire routing table and mac list states to newly connected client. */
        if(FD_ISSET(connection_socket, &readfds)){
            //printf("Got a client\n");
            data_socket = accept(connection_socket, NULL, NULL);
            printf("%i\n", data_socket);
            if (data_socket == -1) {
                perror("accept");
                exit(1);
            }
            
            add_to_monitored_fd_set(data_socket);
            
            update_new_client(data_socket, L3, op, sync_msg);
            update_new_client(data_socket, L2, op, sync_msg);
        }
        else if(FD_ISSET(0, &readfds)){ // update from network admin via stdin
            //printf("Admin has some changes to make\n");
            ret = read(0, op, OP_LEN - 1);
            op[strcspn(op, "\r\n")] = 0; // flush new line
            if (ret == -1) {
                perror("read");
                return 1;
            }
            op[ret] = 0;
            
            //printf("Operation : %s\n", operation);
            
            if (!create_sync_message(op, sync_msg)) {
                // update server's tables
                if (sync_msg->l_code == L3) {
                    process_sync_mesg(routing_table, sync_msg);
                    synchronized = RT;
                }
                else {
                    process_sync_mesg(mac_list, sync_msg);
                    synchronized = ML;
                }

                /* Notify existing clients of changes */
                int i, comm_socket_fd;
                for (i = 2; i < MAX_CLIENTS; i++) {
                    comm_socket_fd = monitored_fd_set[i];
                    if (comm_socket_fd != -1) {
                        write(comm_socket_fd, sync_msg, sizeof(sync_msg_t));
                        write(comm_socket_fd, &synchronized, sizeof(int));
                        write(comm_socket_fd, &loop, sizeof(int));
                    }
                }
                //printf("Entry synchronized to all connected clients.\n");
            }
        }
        else { /* Check active status of clients */
            //printf("Client activity detected\n");
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
