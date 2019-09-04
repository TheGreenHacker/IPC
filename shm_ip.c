#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "Routing-Table/routing-table.h"

/* Stores IP address in newly created shared memory region corresponding to its key, which is a MAC address. Returns the size of the created shm on success otherwise -1 on failure. */
int store_IP(const char *mac, const char *ip) {
    size_t size = strlen(ip); // account for terminating null byte
    int shm_fd = shm_open(mac, O_CREAT | O_RDWR | O_TRUNC, 0660);
    if (shm_fd == -1) {
        printf("Could not create shared memory for MAC %s - IP %s pair\n", mac, ip);
        return -1;
    }
    
    if (ftruncate(shm_fd, size) == -1) {
        printf("Error on ftruncate to allocate size for IP %s\n", ip);
        return -1;
    }
    
    void *shm_reg =  mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_reg == MAP_FAILED){
        printf("Mapping failed\n");
        return -1;
    }
    
    memset(shm_reg, 0, size);
    memcpy(shm_reg, ip, size);
    
    if (munmap(shm_reg, size) == -1) {
        printf("Unmapping failed\n");
        return -1;
    }
    
    close(shm_fd);
    return size;
}

/* Get the IP address corresponding to the MAC address from the shared region that the server created. Returns number of bytes on succcess else -1 on failure. */
int get_IP(const char *mac, char *ip) {
    int shm_fd = shm_open(mac, O_CREAT | O_RDONLY , 0660);
    if (shm_fd == -1) {
        printf("Could not open shared memory for MAC %s - IP %s pair\n", mac, ip);
        return -1;
    }
    
    void *shm_reg = mmap(NULL, IP_ADDR_LEN, PROT_READ, MAP_SHARED, shm_fd, 0);
    if(shm_reg == MAP_FAILED){
        printf("Mapping failed\n");
        return -1;
    }
    
    memcpy(ip, shm_reg, IP_ADDR_LEN);
    if (munmap(shm_reg, IP_ADDR_LEN) == -1) {
        printf("Unmapping failed\n");
        return -1;
    }
    
    close(shm_fd);
    return strlen(ip);
}


