#include <stdio.h>
#include <stdlib.h>

#include "dll.h"

dll_node_t *find(dll_t *dll, int n) {
    dll_node_t *node = dll->head->next;
    while (node != dll->head) {
        if (*((int *) node->data) == n) {
            return node;
        }
        node = node->next;
    }
    return node;
}

void print(dll_t *dll) {
    printf("Printing...\n");
    dll_node_t *node = dll->head->next;
    while (node != dll->head) {
        printf("%i\n", *((int *) node->data));
        node = node->next;
    }
}

void reverse_print(dll_t *dll) {
    printf("Reverse printing...\n");
    dll_node_t *node = dll->tail;
    while (node != dll->head) {
        printf("%i\n", *((int *) node->data));
        node = node->prev;
    }
}

int main() {
    dll_t *dll = init_dll();
    int i;
    for (i = 0; i < 5; i++) {
        int *n = malloc(sizeof(int));
        *n = i;
        append(dll, n);
    }
    
    //print(dll);
    reverse_print(dll);
    
    for (i = 0; i < 5; i++) {
        if (find(dll, i) != dll->head) {
            printf("Correct\n");
        }
    }

    del(dll, find(dll, 3));
    del(dll, find(dll, 0));
    del(dll, find(dll, 4));
    
    print(dll);
    reverse_print(dll);
    
    deinit_dll(dll);
    
    print(dll);
    reverse_print(dll);
    exit(0);
}
