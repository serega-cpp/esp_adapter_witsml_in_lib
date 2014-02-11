#include "tcp_sock.h"

#ifdef _WIN32

WinsockInitializer::WinsockInitializer()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 2), &wsaData);
}

WinsockInitializer::~WinsockInitializer()
{
	WSACleanup();
}

#else // _WIN32

#endif // _WIN32
