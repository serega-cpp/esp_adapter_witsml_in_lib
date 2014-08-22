#ifndef __TCPCLIENT_H__
#define __TCPCLIENT_H__

#include "tcp_sock.h"

class TcpClient
{
public:
	TcpClient();
	~TcpClient();

	bool Connect(const char *hostname, unsigned short int portnum);
	bool Disconnect();

	bool Send(const char * data, size_t size);

	bool IsConnected();

private:
	static bool set_address(const char *hostname, unsigned short int portnum, sockaddr_in *adr);
	static SOCKET tcp_client(const char *hostname, unsigned short int portnum);

	SOCKET m_server_sock;
};

#endif // __TCPCLIENT_H__
