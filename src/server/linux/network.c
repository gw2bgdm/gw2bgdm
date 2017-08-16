#include "server/network.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static int g_sock = -1;

bool network_create(u32 port)
{
    g_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sock < 0)
    {
        return 0;
    }
    
    int val = 1;
    setsockopt(g_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    
    struct sockaddr_in si = { 0 };
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = htonl(INADDR_ANY);
    si.sin_port = htons((u16)port);
    
    if (bind(g_sock, (struct sockaddr*)&si, sizeof(si)) < 0)
    {
        return 0;
    }

    return true;
}

void network_destroy(void)
{
    if (g_sock >= 0)
    {
        close(g_sock);
    }
}

i32 network_recv(void* dst, u32 dst_bytes, u32* addr, u32* port)
{
    struct sockaddr_in si = { 0 };
    int sil = sizeof(si);
	int len = recvfrom(g_sock, dst, dst_bytes, 0, (struct sockaddr*)&si, &sil);
	
	*addr = si.sin_addr.s_addr;
	*port = si.sin_port;
	
    return len;
}

void network_send(void const* src, u32 src_bytes, u32 addr, u32 port)
{
    struct sockaddr_in si = { 0 };
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = addr;
    si.sin_port = (u16)port;
    
    sendto(g_sock, src, src_bytes, 0, (struct sockaddr*)&si, sizeof(si));
}

