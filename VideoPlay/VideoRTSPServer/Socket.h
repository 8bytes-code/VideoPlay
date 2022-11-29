#pragma once
#include <WinSock2.h>
#include <memory>
#include "base.h"

#pragma comment(lib, "ws2_32.lib")


class Socket {
public:
	//nType true.TCP false.UDP
	Socket(bool bType = true) {
		m_sock = INVALID_SOCKET;
		if (bType) {
			m_sock = socket(PF_INET, SOCK_STREAM, 0);
		}
		else {
			m_sock = socket(PF_INET, SOCK_DGRAM, 0);
		}
	}

	Socket(SOCKET s) {
		m_sock = s;
	}

	void Close() {
		if (m_sock != INVALID_SOCKET) {
			SOCKET temp = m_sock;
			m_sock = INVALID_SOCKET;
			closesocket(temp);
		}
	}

	~Socket() {
		Close();
	}

	operator SOCKET() {
		return m_sock;
	}

private:
	SOCKET m_sock;
};

/*自封装地址类*/
class HAddress {
public:
	HAddress() {
		m_port = -1;
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_family = AF_INET;
	}
	HAddress(const std::string& ip, short port) {
		m_ip = ip;
		m_port = port;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	HAddress(const HAddress& addr) {
		m_ip = addr.m_ip;
		m_port = addr.m_port;
		memcpy(&m_addr, &addr.m_addr, sizeof(sockaddr_in));
	}
	HAddress& operator=(const HAddress& addr) {
		if (this != &addr) {
			m_ip = addr.m_ip;
			m_port = addr.m_port;
			memcpy(&m_addr, &addr.m_addr, sizeof(sockaddr_in));
		}
		return *this;
	}
	HAddress& operator=(short port) {
		m_port = (unsigned short)port;
		m_addr.sin_port = htons(port);
		return *this;
	}

	~HAddress() {}

	void Update(const std::string& ip, short port) {
		m_ip = ip;
		m_port = port;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	operator const sockaddr* () const {
		return (sockaddr*)&m_addr;
	}
	operator sockaddr* () {
		return (sockaddr*)&m_addr;
	}
	operator sockaddr_in* () {
		return &m_addr;
	}

	int Size() const {
		return sizeof(sockaddr_in);
	}

	const std::string Ip() const {
		return m_ip;
	}

	unsigned short Port() const {
		return m_port;
	}

	//刷新ip
	void Fresh() {
		m_ip = inet_ntoa(m_addr.sin_addr);
	}

private:
	std::string m_ip;
	unsigned short m_port;
	sockaddr_in m_addr;
};

/*解决Socket复制导致无法完全析构*/
class HSocket {
public:
	HSocket(bool isTcp = true) : m_socket(new Socket(isTcp)), m_isTcp(isTcp) {}
	HSocket(const HSocket& sock) : m_socket(sock.m_socket), m_isTcp(sock.m_isTcp) {}	//复制构造，利用列表初始化
	HSocket(SOCKET sock, bool isTcp) :m_socket(new Socket(sock)), m_isTcp(isTcp) {}

	HSocket& operator=(const HSocket& sock) {
		if (this != &sock) {
			m_socket = sock.m_socket;
		}
		return *this;
	}

	~HSocket() {
		m_socket.reset();
	}

	operator SOCKET() const {
		return *m_socket;
	}

	/*封装tcp基本步骤-start-*/

	int Bind(const HAddress& addr) {
		if (m_socket == nullptr) {
			m_socket.reset(new Socket(m_isTcp));
		}
		return bind(*m_socket, addr, addr.Size());
	}

	int Listen(int backlog = 5) {
		return listen(*m_socket, backlog);
	}

	HSocket Accept(HAddress& addr) {
		int len = addr.Size();
		if (m_socket == nullptr)return HSocket(INVALID_SOCKET, true);
		SOCKET server = *m_socket;
		if (server == INVALID_SOCKET)return HSocket(INVALID_SOCKET, true);
		SOCKET s = accept(server, addr, &len);
		return HSocket(s, m_isTcp);
	}

	int Connect(const HAddress& addr) {
		return connect(*m_socket, addr, addr.Size());
	}

	int Recv(HBuffer& buffer) {
		return recv(*m_socket, buffer, buffer.size(), 0);
	}

	int Send(const HBuffer& buffer) {
		printf("send:%s\r\n", (char*)buffer);

		int index = 0;
		char* pData = buffer;
		while ((int)buffer.size() > index) {
			int ret = send(*m_socket, pData + index, buffer.size() - index, 0);
			if (ret < 0)return ret;
			if (ret == 0)break;
			index += ret;
		}

		return index;	//返回发送大小
	}

	/*封装tcp基本步骤-end-*/

	void Close() {
		m_socket.reset();
	}

private:
	std::shared_ptr<Socket> m_socket;
	bool m_isTcp;
};


/*初始化网络环境*/
class SocketIniter {
public:
	SocketIniter() {
		WSAData wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);
		// TODO:返回值这个可以做处理也可以不做，比如错误就重试，再不行就g
	}

	~SocketIniter() {
		WSACleanup();
	}
};
