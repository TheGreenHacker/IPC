#include <stdio.h>
#include <stdlib.h>

#include "../DLL/dll.h"
#include "sync.h"
/*
#include "../Mac-List/mac-list.h"
#include "../Routing-Table/routing-table.h"
 */

void process_sync_mesg(dll_t *dll, sync_msg_t *sync_msg) {
    dll_node_t *node;
    if (sync_msg->l_code == L3) {
        node = find_routing_table_entry(dll, sync_msg->msg_body.routing_table_entry.dest, sync_msg->msg_body.routing_table_entry.mask);
        switch (sync_msg->op_code) {
            case CREATE:
                if (node == dll->head) {
                    append(dll, &sync_msg->msg_body.routing_table_entry);
                    node = find_routing_table_entry(dll, sync_msg->msg_body.routing_table_entry.dest, sync_msg->msg_body.routing_table_entry.mask);
                    if (node != dll->head) {
                        routing_table_entry_t entry = *((routing_table_entry_t *) node->data);
                        printf("Added: Destination IP: %s mask: %u Gateway IP: %s OIF: %s\n", entry.dest, entry.mask, entry.gw, entry.oif);
                    }
                }
                break;
            case UPDATE:
                if (node != dll->head) {
                    update(node, sync_msg->msg_body.routing_table_entry.gw, sync_msg->msg_body.routing_table_entry.oif);
                    node = find_routing_table_entry(dll, sync_msg->msg_body.routing_table_entry.dest, sync_msg->msg_body.routing_table_entry.mask);
                    if (node != dll->head) {
                        routing_table_entry_t entry = *((routing_table_entry_t *) node->data);
                        printf("Updated: Destination IP: %s mask: %u Gateway IP: %s OIF: %s\n", entry.dest, entry.mask, entry.gw, entry.oif);
                    }
                }
                break;
            case DELETE:
                if (node != dll->head) {
                    del(dll, node);
                    node = find_routing_table_entry(dll, sync_msg->msg_body.routing_table_entry.dest, sync_msg->msg_body.routing_table_entry.mask);
                    if (node == dll->head) {
                        printf("Deleted: Destination IP: %s mask: %u\n", sync_msg->msg_body.routing_table_entry.dest, sync_msg->msg_body.routing_table_entry.mask);
                    }
                }
                break;
            default:
                break;
        }
    }
    else {
        node = find_mac(dll, sync_msg->msg_body.mac_list_entry.mac);
        switch (sync_msg->op_code) {
            case CREATE:
                if (node == dll->head) {
                    append(dll, &sync_msg->msg_body.mac_list_entry);
                    node = find_mac(dll, sync_msg->msg_body.mac_list_entry.mac);
                    if (node != dll->head) {
                        mac_list_entry_t entry = *((mac_list_entry_t *) node->data);
                        printf("Added: MAC: %s\n", entry.mac);
                    }
                }
                break;
            case DELETE:
                if (node != dll->head) {
                    del(dll, node);
                    node = find_mac(dll, sync_msg->msg_body.mac_list_entry.mac);
                    if (node == dll->head) {
                         printf("Deleted: MAC: %s\n", sync_msg->msg_body.mac_list_entry.mac);
                    }
                }
                break;
            default:
                break;
        }
    }
}
