#include <WS2tcpip.h>
#include "meter/network.h"
#include "meter/logging.h"

static SOCKET g_sock;
static SOCKADDR_IN g_si;
static bool g_is_wsa;
static bool g_is_init;

int network_err()
{
	return WSAGetLastError();
}

bool network_create(i8 const* addr, u16 port)
{
	if (!addr || port == 0)
		return false;

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

	DWORD nb = 1;
	if (ioctlsocket(g_sock, FIONBIO, &nb) != 0)
	{
		return false;
	}

	ADDRINFO hints = { 0 };
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	ADDRINFO *res = 0;
	if (getaddrinfo(addr, 0, &hints, &res) != 0)
	{
		return false;
	}

	g_si.sin_family = AF_INET;
	g_si.sin_addr.S_un = ((SOCKADDR_IN*)(res->ai_addr))->sin_addr.S_un;
	g_si.sin_port = htons(port);

	freeaddrinfo(res);

	g_is_init = true;
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

		g_is_wsa = false;
	}

	g_is_init = false;
}

u32 network_recv(void* dst, u32 dst_bytes)
{
	if (g_is_init == false)
	{
		return 0;
	}

	i32 sil = sizeof(g_si);
	i32 len = recvfrom(g_sock, (i8*)dst, dst_bytes, 0, (SOCKADDR*)&g_si, &sil);
	if (len < 0) {
		int err = network_err();
		if (err == WSAEWOULDBLOCK)
			len = 0;
		
		if (err != WSAEWOULDBLOCK)
		{
			LOG_ERR("[client] recvfrom() failed with error %d", err);
			WSASetLastError(err);
		}
	}
	return len;
}

void network_send(void const * src, u32 src_bytes)
{
	if (g_is_init == false)
	{
		return;
	}
	
	int res = sendto(g_sock, (i8*)src, src_bytes, 0, (SOCKADDR*)&g_si, sizeof(g_si));
	if (res == SOCKET_ERROR)
	{
		LOG_ERR("[client] sendto() failed with error %d", network_err());
	}
}
