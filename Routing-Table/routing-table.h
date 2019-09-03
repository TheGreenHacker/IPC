#ifndef ROUTINGTABLE_H
#define ROUTINGTABLE_H

#define IP_ADDR_LEN 16
#define OIF_LEN 32

typedef struct dll_ dll_t;
typedef struct dll_node_ dll_node_t;

/* Data structure definition for routing table entry */
typedef struct routing_table_entry_ {
    char dest[IP_ADDR_LEN];
    unsigned short mask;
    char gw[IP_ADDR_LEN];
    char oif[OIF_LEN];
} routing_table_entry_t;

/* Public API's for routing table functionality */
void display_routing_table(const dll_t *routing_table);
dll_node_t *find_routing_table_entry(const dll_t *routing_table, const char *dest, const unsigned short mask);
void update(dll_node_t *node, const char *gw, const char *oif);

#endif
