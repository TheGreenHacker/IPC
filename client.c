#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "RoutingTable/RoutingTable.h"
#include "sync.h"

int main() {
    struct sockaddr_un addr;
    routing_table_t *routing_table;
    int ret;
    int data_socket;
    int flag;  // indicates to server if client is still connected or not or if server is done sending all current updates
    char conn;  // does client still want to remain connected to server?
    char flush; // flush newline from each stdin operation
    char dest_mask[20];
    sync_msg_t sync_msg;
    
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
    
    /* Continously wait for updates from the routing manager server and update copy of routing table accordingly. */
    do {
        printf("About to read from server\n");
        
        /* Updates to routing table */
        ret = read(data_socket, &sync_msg, sizeof(sync_msg_t));
        printf("%i \n", ret);
        if (ret == -1) {
            perror("read");
            break;
        }
        
        /* Put together the key of the encoded routing table entry */
        memset(dest_mask, 0, sizeof(dest_mask));
        size_t dest_len = strlen(sync_msg.msg_body.dest);
        strncat(dest_mask, sync_msg.msg_body.dest, dest_len);
        strncat(dest_mask + dest_len, sync_msg.msg_body.mask, strlen(sync_msg.msg_body.mask));
        
        switch (sync_msg.op_code) {
            case CREATE:
                insert(routing_table, dest_mask, sync_msg.msg_body.gw, sync_msg.msg_body.oif);
                break;
            case UPDATE:
                update(routing_table, dest_mask, sync_msg.msg_body.gw, sync_msg.msg_body.oif);
                break;
            case DELETE:
                del(routing_table, dest_mask);
                break;
            default:
                printf("%s\n", sync_msg.msg_body.dest);
                printf("%s\n", sync_msg.msg_body.mask);
                perror("Unknown OPCODE");
                break;
        }
        
        /* Have we received all current updates from the server? (i.e. entire table state when just connected to server) */
        ret = read(data_socket, &flag, sizeof(int));
        if (ret == -1) {
            perror("read");
            break;
        }
        else if (flag == 1) { // don't dislpay table in inconsistent state
            printf("Display the updated routing table?\n");
            conn = getchar();
            scanf("%c", &flush);  // flush newline
            if (conn == 'y') {
                display(routing_table);
            }
        }
        else {
            printf("Received %i bytes for flag\n", ret);
            printf("Flag is %i \n", flag);
        }
        
        printf("Proceed to receiving updates from server?\n");
        conn = getchar();
        scanf("%c", &flush);  // flush newline
        if (conn != 'y') {
            printf("Bye\n");
            flag = 0;
            write(data_socket, &flag, sizeof(int));
            break;
        }
    } while (1);
    
    close(data_socket);
    exit(0);
}
