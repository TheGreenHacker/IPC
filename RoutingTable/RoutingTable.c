#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RoutingTable.h"


/* Look up a entry in the table by the key consisting of the destination and subnet */
static routing_table_entry_t *find(const routing_table_t *table, const char *dest_mask) {
    routing_table_entry_t *entry = table->head;
    while (entry && strcmp(entry->dest_mask, dest_mask)) {
        entry = entry->next;
    }
    return entry;
}

/* Adds a new routing table entry to the front of the routing table, if does not exist. */
void insert(routing_table_t *table, const char *dest_mask, const char *gw, const char *oif) {
    if (!find(table, dest_mask)) {
        routing_table_entry_t *entry = calloc(1, sizeof(routing_table_entry_t));
        
        memcpy(entry->dest_mask, dest_mask, strlen(dest_mask));
        memcpy(entry->gw, gw, strlen(gw));
        memcpy(entry->oif, oif, strlen(oif));
        
        if (table->head) {
            entry->next = table->head;
            table->head->prev = entry;
        }
        table->head = entry;
    }
}


/* Updates the entry whose key is dest_mask with new values for gw and oif fields. */
void update(routing_table_t *table, const char *dest_mask, const char *gw, const char *oif) {
    routing_table_entry_t *entry = find(table, dest_mask);
    if (entry) {
        memset(entry->gw, 0, sizeof(entry->gw));
        memset(entry->oif, 0, sizeof(entry->oif));
        memcpy(entry->gw, gw, strlen(gw));
        memcpy(entry->oif, oif, strlen(oif));
    }
}


/* Deletes the entry with key dest_mask. */
void del(routing_table_t *table, const char *dest_mask) {
    routing_table_entry_t *entry = find(table, dest_mask);
    if (entry) {
        if (entry->prev) {
            entry->prev->next = entry->next;
        }
        if (entry->next) {
            entry->next->prev = entry->prev;
        }
        free(entry);
    }
}


/* Display each row (entry) of a routing table. */
void display(const routing_table_t *table) {
    routing_table_entry_t *entry = table->head;
    while (entry) {
        printf("Destination subnet(key): %s  Gateway IP: %s  OIF: %s\n", entry->dest_mask, entry->gw, entry->oif);
        entry = entry->next;
    }
}
