#ifndef ROUTINGTABLE_H
#define ROUTINGTABLE_H

#define IP_ADDR_LEN 16
#define OIF_LEN 32
#define SOCKET_NAME "RTMSocket"

/* Data structures to model routing table and operations */

typedef enum {
    CREATE,
    UPDATE,
    DELETE,
    SHUTDOWN
} OPCODE;

typedef struct _contents {
    char dest[IP_ADDR_LEN];
    unsigned short mask;
    char gw[IP_ADDR_LEN];
    char oif[OIF_LEN];
} contents_t;

typedef struct _sync_msg {
    OPCODE op_code;
    contents_t contents;
} sync_msg_t;

typedef struct routing_table_entry_ {
    contents_t *contents;
    struct routing_table_entry_ *next;
    struct routing_table_entry_ *prev;
} routing_table_entry_t;

typedef struct routing_table_{
    routing_table_entry_t *head;
} routing_table_t;

void insert(routing_table_t *table, const char *dest, unsigned short mask, const char *gw,  const char *oif);
void update(routing_table_t *table, const char *dest, unsigned short mask, const char *gw, const char *oif);
void del(routing_table_t *table, const char *dest, unsigned short mask);
void display(const routing_table_t *table);

void process_sync_mesg(routing_table_t *table, const sync_msg_t *sync_msg);

#endif
