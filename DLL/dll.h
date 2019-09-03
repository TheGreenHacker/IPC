#ifndef DLL_H
#define DLL_H

/* Data structure definitions for generic circular doubly linked list */
typedef struct dll_node_ {
    void *data;
    struct dll_node_ *next;
    struct dll_node_ *prev;
} dll_node_t;

typedef struct dll_{
    dll_node_t *head;
    dll_node_t *tail;
} dll_t;

/* DLL API's */
dll_t *init_dll();
void append(dll_t *dll, void *data);
void del(dll_t *dll, dll_node_t *node);
void deinit_dll(dll_t *dll);

#endif
