#ifndef SYNC_H
#define SYNC_H

#define SOCKET_NAME "RTMSocket"

/* Data structure definitions for synchronization protocol */
typedef enum {
    CREATE,
    UPDATE,
    DELETE
} OPCODE;

typedef struct _msg_body {
    char dest[16];
    char mask[4];
    char gw[16];
    char oif[32];
} msg_body_t;

typedef struct _sync_msg {
    OPCODE op_code;
    msg_body_t msg_body;
} sync_msg_t;

#endif
