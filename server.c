#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
//#include <pthread.h>

#include "RoutingTable/RoutingTable.h"
#include "sync.h"

#define MAX_CLIENTS 32
#define BUFFER_SIZE 32

char dest[16];
char mask[4];
char gw[16];
char oif[32];
char dest_mask[20];

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
    return inet_pton(AF_INET, addr, &(sa.sin_addr)) != 0;
}


/* Mask is valid if it begins with a /, the rest of the string following the / contains only digits, and the numerical value of the rest of the string is between 0 and 32, inclusive. */
static int isValidMask(const char *mask) {
    int int_mask = atoi(mask);
    return mask[0] == '/' && int_mask >= 0 && int_mask <= 32 && digits_only(&mask[1]);
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


/* Prompt routing table manager to fill in the fields for a sync msg body, validating the inputs along the way. */
static void prepare_entry(char op) {
    if (op == 'c' || op == 'u' || op == 'd') {
        do {
            printf("Destination network: \n");
            fgets(dest, sizeof(dest), stdin);
            dest[strcspn(dest, "\r\n")] = 0; // to flush newline from previous stdin
            /*
            printf("The destination you entered is %s\n", dest);
            if (!isValidIP(dest)) {
                printf("You entered an invalid destination IP\n");
            }
            else {
                printf("fucking input streams...\n");
            }
            
            if (isValidIP("122.1.1.1")) {
                printf("WTFFFFF\n");
            }
            else {
                printf("ohhhh\n");
            }
             */
            memcpy(dest_mask, dest, strlen(dest));
        } while (!isValidIP(dest));
        
        do {
            printf("Mask: \n");
            fgets(mask, sizeof(mask), stdin);
            mask[strcspn(mask, "\r\n")] = 0; // to flush newline from previous stdin
            memcpy(dest_mask + strlen(dest), mask, strlen(mask));
        } while (!isValidMask(mask));
        
        if (op != 'd') { // Gateway IP and OIF inquiries only necessary for INSERT AND UPDATE
            do {
                printf("Gateway IP: \n");
                fgets(gw, sizeof(gw), stdin);
                gw[strcspn(gw, "\r\n")] = 0; // to flush newline from previous stdin
            } while (!isValidIP(gw));
            
            printf("OIF: \n");
            fgets(oif, sizeof(oif), stdin);
            oif[strcspn(oif, "\r\n")] = 0; // to flush newline from previous stdin
        }
    }
}


int main() {
    struct sockaddr_un name;
    int ret;
    int connection_socket;
    int data_socket;
    int flag = 1;  // if received, client is done else server has sent all current updates
    fd_set readfds;
    
    /*
    printf("%i \n", isValidIP("122.1.1.1"));
    exit(0);
    */
    intitiaze_monitor_fd_set();
    
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
    
    /* The server continuously checks for new client connections, monitors existing connections (i.e. incoming messages and inactive connections, modifies the routing table if need be, and broadcasts any changes to all client processes. */
    routing_table = calloc(1, sizeof(routing_table_t));
    for (;;) {
        char c, ch;
        sync_msg_t sync_msg;
        
        /* Prompt routing table manager to make changes */
        printf("Select from the following options\n");
        printf("1.CREATE (c)\n");
        printf("2.UPDATE (u)\n");
        printf("3.DELETE (d)\n");
        
        c = getchar();
        scanf("%c",&ch);  // flush newline
        prepare_entry(c);
        
        /* Update server's copy of routing table */
        switch (c) {
            case 'c':
                sync_msg.op_code = CREATE;
                insert(routing_table, dest_mask, gw, oif);
                break;
            case 'u':
                sync_msg.op_code = UPDATE;
                update(routing_table, dest_mask, gw, oif);
                break;
            case 'd':
                sync_msg.op_code = DELETE;
                del(routing_table, dest_mask);
                break;
            default:
                printf("Invalid operation. No changes made.\n");
                break;
        }
        
        printf("Display updated table? (y/n)\n");
        
        c = getchar();
        if (c == 'y') {
            display(routing_table);
        }
        scanf("%c",&ch);  // flush newline
        
        printf("Preparing sync message\n");
        
        /* Prepare sync message */
        memcpy(sync_msg.msg_body.dest, dest, strlen(dest));
        memcpy(sync_msg.msg_body.mask, mask, strlen(mask));
        memcpy(sync_msg.msg_body.gw, gw, strlen(gw));
        memcpy(sync_msg.msg_body.oif, oif, strlen(oif));
        
        printf("Notifying clients\n");
        
        /* Notify existing clients of changes */
        int i, comm_socket_fd;
        for (i = 0; i < MAX_CLIENTS; i++) {
            comm_socket_fd = monitored_fd_set[i];
            if (comm_socket_fd != -1) {
                write(comm_socket_fd, &sync_msg, sizeof(sync_msg));
                write(comm_socket_fd, &flag, sizeof(int));
            }
        }
        
        refresh_fd_set(&readfds); /*Copy the entire monitored FDs to readfds*/
        
        printf("Select...\n");
        
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
            
            routing_table_entry_t *head = routing_table->head;
            while (head) {
                printf("Entry...\n");
                memset(&sync_msg, 0, sizeof(sync_msg_t));
                
                sync_msg.op_code = CREATE;
                
                char *slash_pos = strchr(head->dest_mask, '/');
                
                memcpy(sync_msg.msg_body.dest, head->dest_mask, slash_pos - head->dest_mask);
                memcpy(sync_msg.msg_body.mask, slash_pos, strlen(slash_pos));
                memcpy(sync_msg.msg_body.gw, head->gw, strlen(head->gw));
                memcpy(sync_msg.msg_body.oif, head->oif, strlen(head->oif));
                
                write(data_socket, &sync_msg, sizeof(sync_msg_t));
                
                head = head->next;
            }
            printf("Client: %i\n", data_socket);
            printf("Flag is %i\n", flag);
            write(data_socket, &flag, sizeof(int));
        }
        else { /* There might be a client who wants to disconnect */
            for(i = 0; i < MAX_CLIENTS; i++){
                if(FD_ISSET(monitored_fd_set[i], &readfds)){
                    comm_socket_fd = monitored_fd_set[i];
                    
                    ret = read(comm_socket_fd, &flag, sizeof(int));
                    if (flag == 0) {
                        close(comm_socket_fd);
                        remove_from_monitored_fd_set(comm_socket_fd);
                    }
                    else if (ret == -1) {
                        perror("read");
                        exit(1);
                    }
                }
            }
        }
    }
    
    /* Clean up resources */
    close(connection_socket);
    remove_from_monitored_fd_set(connection_socket);
    unlink(SOCKET_NAME);
    exit(0);
}
