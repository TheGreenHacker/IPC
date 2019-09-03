#include <stdio.h>
#include <stdlib.h>
#include "dll.h"

/* Initialize an empty list in which the head and the tail are the same, i.e. they both point
 to a dummy node. */
dll_t *init_dll() {
    dll_t *dll = calloc(1, sizeof(dll_t));
    dll_node_t *node = calloc(1, sizeof(dll_t));
    node->next = node;
    node->prev = node;
    dll->head = node; // empty list consists of only
    dll->tail = node;
    return dll;
}

/* Add new node with data to back of list. */
void append(dll_t *dll, void *data) {
    dll_node_t *head = dll->head; 
    dll_node_t *node = calloc(1, sizeof(dll_node_t));
    node->data = data;
    node->next = dll->head;
    node->prev = dll->tail;
    dll->head->prev = node;
    dll->tail->next = node;
    dll->tail = node;
    if (dll->head != head) {
        printf("Fuck something's wrong...\n");
    }
}

/* Delete a node from the list. */
void del(dll_t *dll, dll_node_t *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    if (node == dll->tail) { // need to update tail node if we're deleting it
        dll->tail = node->prev;
    }
    free(node);
}

/* Delete all nodes from the list. */
void deinit_dll(dll_t *dll) {
    dll_node_t *node = dll->head->next;
    while (node != dll->head) {
        dll_node_t *next = node->next;
        del(dll, node);
        node = next;
    }
    free(dll);
}
