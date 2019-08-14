#ifndef ROUTINGTABLE_H
#define ROUTINGTABLE_H

typedef struct routing_table_entry_ {
    char dest_mask[20]; // key consisting of destination ip and mask
    char gw[16];  // gateway IP
    char oif[32];  // outgoing interface
    struct routing_table_entry_ *next;
    struct routing_table_entry_ *prev;
} routing_table_entry_t;

typedef struct routing_table_{
    routing_table_entry_t *head;
} routing_table_t;

void insert(routing_table_t *table, const char *dest_mask, const char *gw,  const char *oif);
void update(routing_table_t *table, const char *dest_mask, const char *gw, const char *oif);
void del(routing_table_t *table, const char *dest_mask);
void display(const routing_table_t *table);

#endif
