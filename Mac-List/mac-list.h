#ifndef MAC_LIST_H
#define MAC_LIST_H

#define MAC_ADDR_LEN 18

typedef struct dll_ dll_t;
typedef struct dll_node_ dll_node_t;

/* Data structure definition for MAC address list entry */
typedef struct mac_list_entry_ {
    char mac[MAC_ADDR_LEN];
} mac_list_entry_t;

/* Public API's for MAC list functionality */
void display_mac_list(const dll_t *mac_list);
dll_node_t *find_mac(const dll_t *mac_list, const char *mac);

#endif
