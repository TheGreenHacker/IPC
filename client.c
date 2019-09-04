#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#include "DLL/dll.h"
#include "Mac-List/mac-list.h"
#include "Routing-Table/routing-table.h"
#include "Sync/sync.h"

int data_socket;
int loop = 1;
int disconnect = 1;
dll_t *routing_table;
dll_t *mac_list;

/* Get the IP address corresponding to the MAC address from the shared region that the server created.
 Returns number of bytes on succcess else -1 on failure. */
int get_IP(const char *mac, char *ip) {
    int shm_fd = shm_open(mac, O_CREAT | O_RDONLY , 0660);
    if (shm_fd == -1) {
        printf("Could not open shared memory for MAC %s - IP %s pair\n", mac, ip);
        return -1;
    }
    
    void *shm_reg = mmap(NULL, IP_ADDR_LEN, PROT_READ, MAP_SHARED, shm_fd, 0);
    if(shm_reg == MAP_FAILED){
        printf("Mapping failed\n");
        return -1;
    }
    
    memcpy(ip, shm_reg, IP_ADDR_LEN);
    if (munmap(shm_reg, IP_ADDR_LEN) == -1) {
        printf("Unmapping failed\n");
        return -1;
    }
    
    close(shm_fd);
    return strlen(ip);
}

/* Break out of main infinite loop and inform server of intent to disconnect. */
void signal_handler(int signal_num)
{
    if(signal_num == SIGINT)
    {
        loop = 0;
        write(data_socket, &disconnect, sizeof(int));
        close(data_socket);
        deinit_dll(routing_table);
        deinit_dll(mac_list);
        exit(0);
    }
}

/* Optionally, display contents of a data structure (routing table or MAC list). */
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
    int ret;
    
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
    
    ret = connect (data_socket, (const struct sockaddr *) &addr,
                   sizeof(struct sockaddr_un));
    if (ret == -1) {
        fprintf(stderr, "The server is down.\n");
        exit(1);
    }
    
    signal(SIGINT, signal_handler);  //register signal handler
    
    /* Continously wait for updates from the routing manager server regarding table contents, stability of
     updates to the table, and server status. */
    while (loop) {
        int synchronized;
        char ip[IP_ADDR_LEN];
        sync_msg_t *sync_msg = calloc(1, sizeof(sync_msg_t));
        memset(ip, 0, IP_ADDR_LEN);
        
        //printf("Waiting for sync mesg\n");
        ret = read(data_socket, sync_msg, sizeof(sync_msg_t));
        if (ret == -1) {
            perror("read");
            break;
        }
        
        //printf("Is the table stable?\n");
        ret = read(data_socket, &synchronized, sizeof(int));
        if (ret == -1) {
            perror("read");
            break;
        }
        
        //printf("Server you still there?\n");
        ret = read(data_socket, &loop, sizeof(int));
        if (ret == -1) {
            perror("read");
            break;
        }
        
        if (sync_msg->l_code == L3) {
            process_sync_mesg(routing_table, sync_msg);
        }
        else {
            if (get_IP(sync_msg->msg_body.mac_list_entry.mac, ip)) {
                printf("IP address for MAC address %s is: %s\n", sync_msg->msg_body.mac_list_entry.mac, ip);
            }
            process_sync_mesg(mac_list, sync_msg);
        }
        
        display_ds(synchronized);
    }
    
    exit(0);
}
