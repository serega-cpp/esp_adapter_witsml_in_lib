#include "TcpClient.h"
#include <string.h>

TcpClient::TcpClient():
	m_server_sock(INVALID_SOCKET)
{
}

TcpClient::~TcpClient()
{
	Disconnect();
}

bool TcpClient::Connect(const char *hostname, unsigned short int portnum)
{
	m_server_sock = tcp_client(hostname, portnum);
	return m_server_sock != INVALID_SOCKET;
}

bool TcpClient::Disconnect()
{
	if (m_server_sock != INVALID_SOCKET) {
		closesocket(m_server_sock);
		m_server_sock = INVALID_SOCKET;
	}

	return true;
}

bool TcpClient::Send(const char * data, size_t size)
{
	if (m_server_sock == INVALID_SOCKET)
		return false;

	int sent_bytes = send(m_server_sock, data, (int)size, 0);

	return sent_bytes == (int)size;
}

bool TcpClient::IsConnected()
{
	return m_server_sock != INVALID_SOCKET;
}

bool TcpClient::set_address(const char *hostname, unsigned short int portnum, sockaddr_in *adr)
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

SOCKET TcpClient::tcp_client(const char *hostname, unsigned short int portnum)
{
	SOCKET s;
	struct sockaddr_in peer;

	if (set_address(hostname, portnum, &peer) &&
		(s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET)
	{
		if (connect(s, (sockaddr *) &peer, sizeof(peer)) != CONNECT_ERROR)
			return s;

		closesocket(s);
	}

	return INVALID_SOCKET;
}
