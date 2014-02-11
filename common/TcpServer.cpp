#include "TcpServer.h"
#include <string.h>
#include <sstream>

TcpConnectedClient::TcpConnectedClient(SOCKET s):
	m_sock(s)
{
	memset(&m_addr, 0, sizeof(m_addr));
}

TcpConnectedClient::TcpConnectedClient(const TcpConnectedClient &client):
	m_sock(client.m_sock),
	m_addr(client.m_addr)
{
}

TcpConnectedClient::~TcpConnectedClient()
{
	Close();
}

bool TcpConnectedClient::Send(const char * data, size_t size)
{
	if (m_sock == INVALID_SOCKET)
		return false;

	int sent_bytes = send(m_sock, data, (int)size, 0);

	return sent_bytes == (int)size;
}

int TcpConnectedClient::Recv(char * data, size_t size)
{
	if (m_sock == INVALID_SOCKET)
		return false;

	return recv(m_sock, data, (int) size, 0);
}

bool TcpConnectedClient::IsConnected()
{
	return m_sock != INVALID_SOCKET;
}

void TcpConnectedClient::Reset(SOCKET s)
{
	m_sock = s;
}

void TcpConnectedClient::Close()
{
	if (m_sock != INVALID_SOCKET) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
}

std::string TcpConnectedClient::GetAddress()
{
	return std::string(inet_ntoa(m_addr.sin_addr));
}

unsigned short int TcpConnectedClient::GetPort()
{
	return m_addr.sin_port;
}

///////////////////////////////////////////////////////////////////////////////

TcpServer::TcpServer():
	m_listen_sock(INVALID_SOCKET),
	m_client(INVALID_SOCKET)
{
}

TcpServer::~TcpServer()
{
	if (m_listen_sock != INVALID_SOCKET)
		closesocket(m_listen_sock);
}

bool TcpServer::Create(const char *hostname, unsigned short int nListenPort)
{
	m_listen_sock = tcp_server(hostname, nListenPort);
	return m_listen_sock != INVALID_SOCKET;
}

bool TcpServer::WaitForClient()
{
	if (m_listen_sock != INVALID_SOCKET) {
		socklen_t addr_size = sizeof(m_client.m_addr);
		SOCKET client_sock = accept(m_listen_sock, reinterpret_cast<struct sockaddr *>(&m_client.m_addr), &addr_size);

		m_client.Close();
		m_client.Reset(client_sock);
	}
		
	return m_client.IsConnected();
}

void TcpServer::StopServer()
{
	if (m_listen_sock != INVALID_SOCKET) {
		closesocket(m_listen_sock);
		m_listen_sock = INVALID_SOCKET;
	}
}

bool TcpServer::CloseConnection()
{
	m_client.Close();

	return !m_client.IsConnected();
}

TcpConnectedClient *TcpServer::GetConnectedClient(bool take_ownership)
{
	if (!take_ownership) {
		return &m_client;
	}
	else {
		TcpConnectedClient *client = new TcpConnectedClient(m_client);
		m_client.Reset();
		return client;
	}
}

bool TcpServer::set_address(const char *hostname, unsigned short int portnum, sockaddr_in *adr)
{
	memset(adr, 0, sizeof(*adr));
	adr->sin_family = AF_INET;

	if (hostname != NULL) {
		adr->sin_addr.s_addr = inet_addr(hostname);
		if (adr->sin_addr.s_addr == INADDR_NONE) {
			hostent *hp = gethostbyname(hostname);
			if (hp == NULL)
				return false;
			memcpy(&adr->sin_addr, hp->h_addr_list[0], sizeof(adr->sin_addr));
		}
	}
	else {
		adr->sin_addr.s_addr = htonl(INADDR_ANY);
	}

	adr->sin_port = htons(portnum);

	return true;
}

SOCKET TcpServer::tcp_server(const char *hostname, unsigned short int portnum)
{
	SOCKET s;
	struct sockaddr_in local;

	if (set_address(hostname, portnum, &local) &&
		(s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET) {

			const int reuse_addr = 1;
			setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse_addr), sizeof(reuse_addr));

			if (bind(s, reinterpret_cast<struct sockaddr *>(&local), sizeof(local)) == 0 &&
				listen(s, SOMAXCONN) == 0) {
					return s; // ok
			}

			closesocket(s);
	}

	return INVALID_SOCKET;
}

///////////////////////////////////////////////////////////////////////////////
