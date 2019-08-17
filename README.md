# Routing Table Manager
The server process maintains and updates a L3 routing table, which is synchronized across all clients
at any point in time using UNIX domain sockets.

## Routing Entry Fields
An entry in the routing table contains:
* dest -- IPv4 address of destination network
* mask -- integer value in [0, 32] that represents the subnet mask
* gw -- IPv4 address of gateway
* oif -- name of outgoing interface

## Routing Table Operations
The routing table manager can perform the following operations on the routing table:
* CREATE dest mask gw oif -- create a new record in the routing table if not found
* UPDATE dest mask <new>gw <new>oif -- if a record with the given dest and mask is found, update it's
gw and oif with the new values
* DELETE dest mask -- delete the entry with dest and mask from the routing table

## Synchronization Protocol
* Whenever a new client connects with the server, the client shall receive the entire current table state
* Whenever the server validly creates, updates, or deletes an entry, the client's copies of the table shall
reflect all such changes
* When a client shuts down via CTRL-C, the server continues running smoothly
* When the server shuts down via CTRL-C, all clients shut down cleanly
* All entries are uniquely identified by the dest and mask fields

## Other Notes
When client shuts down on Mac OS or Windows, server SEG FAULTS. So please, only run on Linux systems. 
