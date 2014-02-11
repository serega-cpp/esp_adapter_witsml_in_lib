#ifndef __TCP_SOCK_HDR_H__
#define __TCP_SOCK_HDR_H__

#ifdef _WIN32

#include <windows.h>
#include <winsock.h>

#pragma comment(lib, "Ws2_32.lib")

class WinsockInitializer
{
public:
	WinsockInitializer();
	~WinsockInitializer();
};

static WinsockInitializer winsock_initizer;

typedef int socklen_t;

const int CONNECT_ERROR = SOCKET_ERROR;

#else // !_WIN32

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

typedef int SOCKET;

const int INVALID_SOCKET = -1;
const int CONNECT_ERROR = -1;

#define closesocket close
#define Sleep(t) sleep((t)<1000?1:(t)/1000)

#endif // _WIN32

#endif // __TCP_SOCK_HDR_H__
