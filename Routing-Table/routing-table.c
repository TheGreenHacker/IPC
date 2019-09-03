#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "routing-table.h"
#include "../DLL/dll.h"

/* Display each row (entry) of a routing table. */
void display_routing_table(const dll_t *routing_table) {
    printf("Printing routing table\n");
    
    dll_node_t *node = routing_table->head->next;
    while (node != routing_table->head) {
        routing_table_entry_t entry = *((routing_table_entry_t *) node->data);
        printf("Destination IP: %s  Mask: %u Gateway IP: %s  OIF: %s\n", entry.dest, entry.mask, entry.gw, entry.oif);
        node = node->next;
    }
}

/* Look up entry in routing table by destination IP and subnet mask. */
dll_node_t *find_routing_table_entry(const dll_t *routing_table, const char *dest, const unsigned short mask) {
    dll_node_t *node = routing_table->head->next;
    while (node != routing_table->head) {
        routing_table_entry_t entry = *((routing_table_entry_t *) node->data);
        if (!strcmp(entry.dest, dest) && entry.mask == mask) {
            break;
        }
        node = node->next;
    }
    return node;
}

/* Updates the entry whose destination IP is dest and subnet mask is mask. */
void update(dll_node_t *node, const char *gw, const char *oif) {
    routing_table_entry_t *entry = node->data;
    memset(entry->gw, 0, IP_ADDR_LEN);
    memset(entry->oif, 0, OIF_LEN);
    memcpy(entry->gw, gw, strlen(gw));
    memcpy(entry->oif, oif, strlen(oif));
}
