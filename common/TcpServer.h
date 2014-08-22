#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "tcp_sock.h"
#include <string>

class TcpConnectedClient
{
public:
	TcpConnectedClient(SOCKET s);
	TcpConnectedClient(const TcpConnectedClient &client);
	~TcpConnectedClient();

	bool Send(const char * data, size_t size);
	int Recv(char * data, size_t size);

	bool IsConnected();

	void Reset(SOCKET s = INVALID_SOCKET);
	void Close();

	std::string GetAddress();
	unsigned short int GetPort();

private:
	SOCKET m_sock;
	struct sockaddr_in m_addr;

	friend class TcpServer;
};

class TcpServer
{
public:
	TcpServer();
	~TcpServer();

	bool Create(const char *hostname, unsigned short int nListenPort);
	bool WaitForClient();

	void StopServer();
	bool CloseConnection();

	TcpConnectedClient *GetConnectedClient(bool take_ownership = false);

private:
	static bool set_address(const char *hostname, unsigned short int portnum, sockaddr_in *adr);
	static SOCKET tcp_server(const char *hostname, unsigned short int portnum);

	SOCKET m_listen_sock;
	TcpConnectedClient m_client;
};

#endif // __TCPSERVER_H__
