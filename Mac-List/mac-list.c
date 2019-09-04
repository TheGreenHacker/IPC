#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mac-list.h"
#include "../DLL/dll.h"
#include "../Routing-Table/routing-table.h"

extern int get_IP(const char *mac, char *ip);

/* Display each row (entry) of a routing table. */
void display_mac_list(const dll_t *mac_list) {
    printf("Printing MAC list\n");
    
    dll_node_t *node = mac_list->head->next;
    while (node != mac_list->head) {
        mac_list_entry_t entry = *((mac_list_entry_t *) node->data);
        printf("MAC: %s ", entry.mac);
        
        char ip[IP_ADDR_LEN];
        if (get_IP(entry.mac, ip) != -1) {
            printf("IP: %s", ip);
        }
        printf("\n");
        node = node->next;
    }
}

/* Look up entry in MAC list by MAC address. */
dll_node_t *find_mac(const dll_t *mac_list, const char *mac) {
    dll_node_t *node = mac_list->head->next;
    while (node != mac_list->head) {
        mac_list_entry_t entry = *((mac_list_entry_t *) node->data);
        if (!strcmp(entry.mac, mac)) {
            break;
        }
        node = node->next;
    }
    return node;
}
