#ifndef SYNC_H
#define SYNC_H

#include "../Routing-Table/routing-table.h"
#include "../Mac-List/mac-list.h"

/* Synchronization protocol constants and structure definitions */
#define SOCKET_NAME "NetworkAdminSocket"

#define WAIT 0
#define RT 1
#define ML 2

typedef struct dll_ dll_t;
/*
typedef struct routing_table_entry_ routing_table_entry_t;
typedef struct mac_list_entry_ mac_list_entry_t;
 */

typedef enum {
    CREATE,
    UPDATE,
    DELETE,
    NONE
} OPCODE;

/* Specifies whether we're dealing with L3 (IP routing table) or L2 (MAC address list) */
typedef enum {
    L3,
    L2
} LCODE;

typedef struct _sync_msg {
    OPCODE op_code;
    LCODE l_code;
    union {
        routing_table_entry_t routing_table_entry;
        mac_list_entry_t mac_list_entry;
    } msg_body;
} sync_msg_t;

void process_sync_mesg(dll_t *dll, sync_msg_t *sync_msg);

#endif
