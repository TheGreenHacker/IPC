#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtm.h"

/* Look up a entry in the table by destination IP and mask */
static routing_table_entry_t *find(const routing_table_t *table, const char *dest, unsigned short mask) {
    routing_table_entry_t *entry = table->head;
    while (entry->contents->mask != DUMMY_MASK) {
        if (!strcmp(entry->contents->dest, dest) && entry->contents->mask == mask) {
            break;
        }
        entry = entry->next;
    }
    return entry;
}

/* Adds a new routing table entry to the front of the routing table, if does not exist. */
static void insert(routing_table_t *table, const char *dest, unsigned short mask, const char *gw, const char *oif) {
    routing_table_entry_t *entry = calloc(1, sizeof(routing_table_entry_t));
    entry->contents = calloc(1, sizeof(contents_t));
    
    memcpy(entry->contents->dest, dest, strlen(dest));
    entry->contents->mask = mask;
    memcpy(entry->contents->gw, gw, strlen(gw));
    memcpy(entry->contents->oif, oif, strlen(oif));
    
    entry->next = table->head;
    table->head->prev = entry;
    table->head = entry;
}


/* Updates the entry whose destination IP is dest and subnet mask is mask. */
static void update(routing_table_entry_t *entry, const char *gw, const char *oif) {
    memset(entry->contents->gw, 0, IP_ADDR_LEN);
    memset(entry->contents->oif, 0, OIF_LEN);
    memcpy(entry->contents->gw, gw, strlen(gw));
    memcpy(entry->contents->oif, oif, strlen(oif));
}


/* Deletes the entry with destination IP and subnet mask mask */
static void del(routing_table_t *table, routing_table_entry_t *entry) {
    if (entry->prev) {
        //printf("Unlinking from previous neighbor\n");
        entry->prev->next = entry->next;
    }
    if (entry->next) {
        //printf("Unlinking from next neighbor\n");
        entry->next->prev = entry->prev;
    }
    if (table->head == entry) {
        table->head = entry->next;
    }
    free(entry->contents);
    free(entry);
}


void init_routing_table(routing_table_t *table) {
    if (table) {
        table->head = calloc(1, sizeof(routing_table_entry_t));
        table->head->contents = calloc(1, sizeof(contents_t));
        table->head->contents->mask = DUMMY_MASK;
    }
}


/* Display each row (entry) of a routing table. */
void display(const routing_table_t *table) {
    routing_table_entry_t *entry = table->head;
    while (entry->contents->mask != DUMMY_MASK) {
        printf("Destination IP: %s  Mask: %u Gateway IP: %s  OIF: %s\n", entry->contents->dest, entry->contents->mask, entry->contents->gw, entry->contents->oif);
        entry = entry->next;
    }
}


/* Update one's copy of the routing table given a sync message. */
void process_sync_mesg(routing_table_t *table, const sync_msg_t *sync_msg) {
    routing_table_entry_t *entry = find(table, sync_msg->contents.dest, sync_msg->contents.mask);
    switch (sync_msg->op_code) {
        case CREATE:
            if (entry->contents->mask == DUMMY_MASK) {
                insert(table, sync_msg->contents.dest, sync_msg->contents.mask, sync_msg->contents.gw, sync_msg->contents.oif);
            }
            break;
        case UPDATE:
            if (entry->contents->mask != DUMMY_MASK) {
                update(entry, sync_msg->contents.gw, sync_msg->contents.oif);
            }
            break;
        case DELETE:
            if (entry->contents->mask != DUMMY_MASK) {
                del(table, entry);
            }
            break;
        default:
            break;
    }
}



void deinit_routing_table(routing_table_t *table) {
    while (table->head){
        del(table, table->head);
    }
}
