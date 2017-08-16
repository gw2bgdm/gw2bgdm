#pragma once
#include "core/types.h"

// Returns the last network error WSAGetLastError
int network_err();

// Creates the networking interface.
bool network_create(i8 const* addr, u16 port);

// Destroys the networking interface.
void network_destroy(void);

// Receives a packet from the network. Retunrns the length of the packet receieved.
u32 network_recv(void* dst, u32 dst_bytes);

// Sends a packet.
void network_send(void const * src, u32 src_bytes);