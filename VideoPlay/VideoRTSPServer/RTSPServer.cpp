#include "RTSPServer.h"
#include <rpc.h>
#pragma comment(lib, "rpcrt4.lib")

SocketIniter RTSPServer::init;

int RTSPServer::Init(const std::string& strIP, short port) {
	m_addr.Update(strIP, port);
	m_socket.Bind(m_addr);
	m_socket.Listen();
	return 0;
}

int RTSPServer::Invoke() {
	m_threadMain.Start();
	m_pool.Invoke();
	return 0;
}

void RTSPServer::Stop() {
	m_socket.Close();
	m_threadMain.Stop();
	m_pool.Stop();
}

int RTSPServer::threadWorker() {
	HAddress cliAddr;
	HSocket client = m_socket.Accept(cliAddr);
	//有连接之后给线程分发任务
	if (INVALID_SOCKET != client) {
		RTSPSession session(client);
		m_lstSession.PushBack(session);
		m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&RTSPServer::ThreadSession));
	}

	return 0;
}

int RTSPServer::ThreadSession() {
	RTSPSession session;
	if (m_lstSession.PopFront(session)) {
		int ret = session.PickRequestAndReply(RTSPServer::PlayCallBack, this);
		return ret;
	}
	return -1;
}

void RTSPServer::PlayCallBack(RTSPServer* thiz, RTSPSession& session) {
	thiz->UdpWorker(session.GetClientUDPAddress());
}

void RTSPServer::UdpWorker(const HAddress& Client) {
	HBuffer frame = m_h264.ReadOneFrame();
	RTPFrame rtp;
	while (frame.size() > 0) {
		m_helper.SendMediaFrame(rtp, frame, Client);
		frame = m_h264.ReadOneFrame();
	}
}

RTSPSession::RTSPSession() {
	m_port = -1;
	UUID uuid;	//生成唯一的ssession id
	UuidCreate(&uuid);
	m_id.resize(8);
	snprintf((char*)m_id.c_str(), m_id.size(), "%u%u", uuid.Data1, uuid.Data2);
}

RTSPSession::RTSPSession(const HSocket& client)
	: m_client(client) 
{
	UUID uuid;	//生成唯一的ssession id
	UuidCreate(&uuid);
	m_id.resize(8);
	snprintf((char*)m_id.c_str(), m_id.size(), "%u%u", uuid.Data1, uuid.Data2);
	m_port = -1;
}

RTSPSession::RTSPSession(const RTSPSession& session) {
	m_id = session.m_id;
	m_client = session.m_client;
	m_port = session.m_port;
}

RTSPSession& RTSPSession::operator=(const RTSPSession& session) {
	if (this != &session) {
		m_id = session.m_id;
		m_client = session.m_client;
		m_port = session.m_port;
	}
	return *this;
}

int RTSPSession::PickRequestAndReply(RTSPPLAYCB cb, RTSPServer* thiz) {
	int ret = -1;
	do {
		HBuffer buffer = Pick();
		if (buffer.size() <= 0)return-1;
		RTSPRequest req = AnalyseRequest(buffer);

		//method范围0-4,还可以做其他校验方式
		if (req.method() < 0) {
			TRACE("buffer[%s]\r\n", (char*)buffer);
			return-2;
		}
		RTSPReply rep = Reply(req);
		ret = m_client.Send(rep.toBuffer());
		if (req.method() == 2) {
			m_port = (short)atoi(req.port());
		}
		if (req.method() == 3) {
			cb(thiz, *this);
		}
	} while (ret >= 0);
	if (ret < 0)return ret;	//send error
	return 0;
}

HAddress RTSPSession::GetClientUDPAddress() const {
	HAddress addr;
	int len = addr.Size();
	getsockname(m_client, addr, &len);
	addr.Fresh();
	addr = m_port;
	return addr;
}

HBuffer RTSPSession::PickOneLine(HBuffer& buffer) {
	if (buffer.size() <= 0)return HBuffer();

	HBuffer result, temp;
	int i = 0;
	//读到换行符表示这一句数据完整了
	for (; i < (int)buffer.size(); i++) {
		result += buffer.at(i);
		if (buffer.at(i) == '\n')break;
	}
	//右移数据
	temp = i + 1 + (char*)buffer;
	buffer = temp;

	return result;
}

HBuffer RTSPSession::Pick() {
	HBuffer result;
	HBuffer buf(1);
	int ret = 1;
	while (ret > 0) {
		buf.Zero();						//内存值置零，不会改变大小
		ret = m_client.Recv(buf);		//充分考虑数据量大时分流接收
		if (ret > 0) {
			result += buf;
			//若是result-4等于"\r\n\r\n"就代表没有数据或者完整
			if (result.size() >= 4) {
				UINT val = *(UINT*)(result.size() - 4 + (char*)result);
				if (val == *(UINT*)"\r\n\r\n") {
					break;
				}
			}
		}
	}
	return result;
}

RTSPRequest RTSPSession::AnalyseRequest(const HBuffer& buffer) {
	TRACE("%s\r\n", (char*)buffer);

	RTSPRequest request;
	HBuffer data = buffer;
	HBuffer line = PickOneLine(data);
	HBuffer method(32), url(1024), version(16), seq(64);	//固定的格式
	//buffer首先能读到这仨数据
	if (sscanf(line, "%s %s %s\r\n", (char*)method, (char*)url, (char*)version) < 3) {
		TRACE("error at :[%s]\r\n", (char*)line);
		return request;
	}

	line = PickOneLine(data);
	if (sscanf(line, "CSeq: %s\r\n", (char*)seq) < 1) {
		TRACE("error at :[%s]\r\n", (char*)line);
		return request;
	}
	request.SetMethod(method);
	request.SetUrl(url);
	request.SetSequence(seq);
	//判断要用什么方法
	if ((strcmp(method, "OPTIONS") == 0) || (strcmp(method, "DESCRIBE") == 0)) {
		return request;
	}
	else if (strcmp(method, "SETUP") == 0) {
		do {
			line = PickOneLine(data);
			if (strstr((const char*)line, "client_port=") == NULL)continue;
			break;
		} while (line.size() > 0);
		
		int port[2] = { 0 };
		if (sscanf(line, "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n", port, port + 1) == 2) {
			request.SetClientPort(port);
			return request;
		}
	}
	else if ((strcmp(method, "PLAY") == 0) || (strcmp(method, "TEARDOWN") == 0)) {
		line = PickOneLine(data);
		HBuffer session(64);
		if (sscanf(line, "Session: %s\r\n", (char*)session) == 1) {
			request.SetSession(session);
			return request;
		}
	}

	return request;
}

RTSPReply RTSPSession::Reply(const RTSPRequest& request) {
	RTSPReply reply;
	reply.SetSequence(request.sequence());
	if (request.session().size() > 0) {
		reply.SetSession(request.session());
	}
	else {
		//有的时候session确实是空的，但是总得表示一下用一个有的内容填充
		reply.SetSession(m_id);
	}

	reply.SetMethod(request.method());
	switch (request.method()) {
		case 0:		//OPTIONS
			reply.SetOptions("Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n");
			break;
		case 1:		//DESCRIBE
		{
			HBuffer sdp;
			sdp << "v=0\r\n";
			sdp << "o=- " << (char*)m_id << " 1 IN IP4 127.0.0.1\r\n";
			sdp << "t=0 0\r\n" << "a=control:*\r\n" << "m=video 0 RTP/AVP 96\r\n";
			sdp << "a=framerate:24\r\n";
			sdp << "a=rtpmap:96 H264/90000\r\n" << "a=control:track0\r\n";
			reply.SetSdp(sdp);
		}
		break;
		case 2:		//SETUP
			reply.SetClientPort(request.port(), request.port(1));
			reply.SetServerPort("55000", "55001");	//前者RTP，后者RTCP端口
			reply.SetSession(m_id);
			break;
		case 3:		//PLAY
		case 4:		//TEARDOWN
			break;
		default:break;
	}
	return reply;
}

RTSPRequest::RTSPRequest() {
	m_method = -1;
}

RTSPRequest::RTSPRequest(const RTSPRequest& protocol) {
	m_method = protocol.m_method;
	m_url = protocol.m_url;
	m_session = protocol.m_session;
	m_seq = protocol.m_seq;
	m_client_port[0] = protocol.m_client_port[0];
	m_client_port[1] = protocol.m_client_port[1];
}

RTSPRequest& RTSPRequest::operator=(const RTSPRequest& protocol) {
	if (this != &protocol) {
		m_method = protocol.m_method;
		m_url = protocol.m_url;
		m_session = protocol.m_session;
		m_seq = protocol.m_seq;
		m_client_port[0] = protocol.m_client_port[0];
		m_client_port[1] = protocol.m_client_port[1];
	}
	return *this;
}

void RTSPRequest::SetMethod(const HBuffer& method) {
	if (strcmp(method, "OPTIONS") == 0)m_method = 0;
	else if (strcmp(method, "DESCRIBE") == 0)m_method = 1;
	else if (strcmp(method, "SETUP") == 0)m_method = 2;
	else if (strcmp(method, "PLAY") == 0)m_method = 3;
	else if (strcmp(method, "TEARDOWN") == 0)m_method = 4;
}

void RTSPRequest::SetUrl(const HBuffer& url) {
	m_url = (char*)url;
}

void RTSPRequest::SetSequence(const HBuffer& seq) {
	m_seq = (char*)seq;
}

void RTSPRequest::SetClientPort(int ports[]) {
	m_client_port[0] << ports[0];
	m_client_port[1] << ports[1];
}

void RTSPRequest::SetSession(const HBuffer& session) {
	m_session = (char*)session;
}

RTSPReply::RTSPReply() {
	m_method = -1;
}

RTSPReply::RTSPReply(const RTSPReply& protocol) {
	m_method = protocol.m_method;
	m_client_port[0] = protocol.m_client_port[0];
	m_client_port[1] = protocol.m_client_port[1];
	m_server_port[0] = protocol.m_server_port[0];
	m_server_port[1] = protocol.m_server_port[1];
	m_sdp = protocol.m_sdp;
	m_seq = protocol.m_seq;
	m_options = protocol.m_options;
	m_session = protocol.m_session;
}

RTSPReply& RTSPReply::operator=(const RTSPReply& protocol) {
	if (this != &protocol) {
		m_method = protocol.m_method;
		m_client_port[0] = protocol.m_client_port[0];
		m_client_port[1] = protocol.m_client_port[1];
		m_server_port[0] = protocol.m_server_port[0];
		m_server_port[1] = protocol.m_server_port[1];
		m_sdp = protocol.m_sdp;
		m_seq = protocol.m_seq;
		m_options = protocol.m_options;
		m_session = protocol.m_session;
	}
	return *this;
}

HBuffer RTSPReply::toBuffer() {
	HBuffer result;
	result << "RTSP/1.0 200 OK\r\n" << "CSeq: " << m_seq << "\r\n";
	switch (m_method) {
		case 0:		//OPTIONS
			result << "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n\r\n";
			break;
		case 1:		//DESCRIBE
			result << "Content-Base: 127.0.0.1\r\n";
			result << "Content-type: application/sdp\r\n";
			result << "Content-length: " << m_sdp.size() << "\r\n\r\n";
			result << (char*)m_sdp;
			break;
		case 2:		//SETUP
			result << "Transport: RTP/AVP;unicast;client_port=" << m_client_port[0] << "-" << m_client_port[1];
			result << ";server_port=" << m_server_port[0] << "-" << m_server_port[1] << "\r\n";
			result << "Session: " << (char*)m_session << "\r\n\r\n";
			break;
		case 3:		//PLAY
			result << "Range: npt=0.000-\r\n";
			result << "Session: " << (char*)m_session << "\r\n\r\n";
			break;
		case 4:		//TEARDOWN
			result << "Session: " << (char*)m_session << "\r\n\r\n";
			break;
		default:break;
	}
	return result;
}

void RTSPReply::SetMethod(int method) {
	m_method = method;
}

void RTSPReply::SetOptions(const HBuffer& options) {
	m_options = options;
}

void RTSPReply::SetSequence(const HBuffer& seq) {
	m_seq = seq;
}

void RTSPReply::SetSdp(const HBuffer& sdp) {
	m_sdp = sdp;
}

void RTSPReply::SetClientPort(const HBuffer& port0, const HBuffer& port1) {
	port0 >> m_client_port[0];
	port1 >> m_client_port[1];
}

void RTSPReply::SetServerPort(const HBuffer& port0, const HBuffer& port1) {
	port0 >> m_server_port[0];
	port1 >> m_server_port[1];
}

void RTSPReply::SetSession(const HBuffer& session) {
	m_session = session;
}
