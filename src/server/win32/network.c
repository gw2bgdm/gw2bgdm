#include <WinSock2.h>
#include <WS2tcpip.h>
#include "server/debug.h"
#include "server/network.h"

static SOCKET g_sock;
static bool g_is_wsa;

bool network_create(u32 port)
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return false;
	}

	g_is_wsa = true;
	g_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (g_sock == SOCKET_ERROR)
	{
		return false;
	}

	SOCKADDR_IN si = { 0 };
	si.sin_family = AF_INET;
	si.sin_addr.S_un.S_addr = INADDR_ANY;
	si.sin_port = htons((u16)port);

	if (bind(g_sock, (SOCKADDR*)&si, sizeof(si)) == SOCKET_ERROR)
	{
		return false;
	}

	return true;
}

void network_destroy(void)
{
	if (g_is_wsa)
	{
		if (g_sock != SOCKET_ERROR)
		{
			closesocket(g_sock);
		}

		WSACleanup();
	}
}

i32 network_recv(void* dst, u32 dst_bytes, u32* addr, u32* port)
{
	SOCKADDR_IN si = { 0 };
	i32 sil = sizeof(si);
	i32 len = recvfrom(g_sock, dst, (i32)dst_bytes, 0, (SOCKADDR*)&si, &sil);

	*addr = si.sin_addr.S_un.S_addr;
	*port = si.sin_port;

	return len;
}

void network_send(void const * src, u32 src_bytes, u32 addr, u32 port)
{
	SOCKADDR_IN si = { 0 };
	si.sin_family = AF_INET;
	si.sin_addr.S_un.S_addr = addr;
	si.sin_port = (u16)port;

	int res = sendto(g_sock, src, (i32)src_bytes, 0, (SOCKADDR*)&si, sizeof(si));
	if (res == SOCKET_ERROR)
	{
		i8 str_addr[22] = { 0 };
		network_addr_to_str(addr, str_addr);
		LOG_ERR("sendto(%s:%d) failed with error %x", str_addr, port, WSAGetLastError());
	}
	else
	{
		i8 str_addr[22] = { 0 };
		network_addr_to_str(addr, str_addr);
		LOG(7, "sendto(%s:%d) succesfully sent %d bytes", str_addr, port, res);
	}
}

// Convert ip to string
i8 const* network_addr_to_str(u32 addr, i8* str_addr)
{
	if (!str_addr)
		return NULL;

	memset(str_addr, 0, INET_ADDRSTRLEN);

	SOCKADDR_IN si = { 0 };
	si.sin_family = AF_INET;
	si.sin_addr.S_un.S_addr = addr;
	return inet_ntop(AF_INET, &(si.sin_addr), str_addr, INET_ADDRSTRLEN);

	//wsprintfA(str_addr,
	//	"%d.%d.%d.%d",
	//	(addr & 0xFF),
	//	(addr >> 8) & 0xFF,
	//	(addr >> 16) & 0xFF,
	//	(addr >> 24) & 0xFF
	//);
}
