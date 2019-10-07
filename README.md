# IPC
The purpose of this project is to explore the application of various IPC techniques in C to implementing Linux system applications. The server process maintains and updates two network data structures: a L3 IPv4 Routing Table and L2 MAC list. Changes to the routing table and MAC List are synchronized to the client via UNIX domain sockets. The clients then use the MAC addresses as keys to look up the corresponding IP address stored in a shared memory region created by the server. Note that while the client, for demonstration purposes simply only prints the data, the concept of data synchronization using IPC mechanisms can easily be extended to real-life network administration tasks as the Routing Table and ARP Table (MAC list + shared memory IP) contains necessary info for other application functionality, such as firewalls and best-path routing algorithm computations. 

## Routing Entry Fields
An entry in the routing table contains:
* dest -- IPv4 address of destination network
* mask -- integer value in [0, 32] that represents the subnet mask
* gw -- IPv4 address of gateway
* oif -- name of outgoing interface

## MAC Entry Fields
An entry in the MAC list contains:
* mac -- 6 byte L2 MAC address

## Routing Table Operations
* CREATE dest mask gw oif -- create a new record in the routing table if not found
* UPDATE dest mask <new>gw <new>oif -- if a record with the given dest and mask is found, update it's
gw and oif with the new values
* DELETE dest mask -- delete the entry with dest and mask from the routing table
  
## Mac List Operations
* CREATE MAC -- create a new record in the MAC list if not found (server is responsible for storing corresponding IP address in shared memory region)
* DELETE MAC -- delete the entry with mac and deallocate shared memory region corresponding to mac

## Synchronization Protocol
* Whenever a new client connects with the server, the client shall receive the entire states of the routing table and mac list
* Whenever the server validly creates, updates, or deletes an entry, the client's copies of the table shall
reflect all such changes
* When a client shuts down via CTRL-C, the server continues running smoothly
* When the server shuts down via CTRL-C, all clients shut down cleanly
* All routing table entries are uniquely identified by the dest and mask fields and mac list entries identified by mac field
* The server can flush routing table and mac list at any time, emptying the contents of both data structures, and informs clients to do the same by sending them a USRSIG1 signal.

## Resources
* Udemy's "Linux Inter Process Communication (IPC) from Scratch" course by Abhishek Sagar

## Other Notes
When client shuts down on Mac OS or Windows, server SEG FAULTS. So please, only run on Linux systems. Also, -lrt is not recognized by clang.
