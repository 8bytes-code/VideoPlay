#pragma once
#include "Socket.h"
#include "CHeQueue.h"
#include "HeThread.h"
#include "RTPHelper.h"
#include "MediaFile.h"
#include <string>
#include <map>

/*请求类*/
class RTSPRequest {
public:
	RTSPRequest();
	RTSPRequest(const RTSPRequest& protocol);
	RTSPRequest& operator=(const RTSPRequest& protocol);
	~RTSPRequest() { m_method = -1; }

	/*set*/
	void SetMethod(const HBuffer& method);
	void SetUrl(const HBuffer& url);
	void SetSequence(const HBuffer& seq);
	void SetClientPort(int ports[]);
	void SetSession(const HBuffer& session);
	/*get*/
	int method() const { return m_method; }
	const HBuffer& url() const { return m_url; }
	const HBuffer& session() const { return m_session; }
	const HBuffer& sequence() const { return m_seq; }
	const HBuffer& port(int index = 0) const { return index ? m_client_port[1] : m_client_port[0]; }

private:
	int m_method;				//-1:初始化 0:options 1:describe 2:setup 3:play 4:tearown
	HBuffer m_url;				//保存地址
	HBuffer m_session;			//保存会话id
	HBuffer m_seq;				//保存请求序列号
	HBuffer m_client_port[2];	//client port
};

/*回复或者应答类*/
class RTSPReply {
public:
	RTSPReply();
	RTSPReply(const RTSPReply& protocol);
	RTSPReply& operator=(const RTSPReply& protocol);
	~RTSPReply() { m_method = -1; }
	HBuffer toBuffer();

	/*set*/
	void SetMethod(int method);
	void SetOptions(const HBuffer& options);
	void SetSequence(const HBuffer& seq);
	void SetSdp(const HBuffer& sdp);
	void SetClientPort(const HBuffer& port0, const HBuffer& port1);
	void SetServerPort(const HBuffer& port0, const HBuffer& port1);
	void SetSession(const HBuffer& session);

private:
	int m_method;				//-1:初始化 0:options 1:describe 2:setup 3:play 4:tearown
	int m_client_port[2];		//client port
	int m_server_port[2];		//server port
	HBuffer m_sdp;				//保存会话描述信息
	HBuffer m_seq;				//保存请求序列号
	HBuffer m_options;			//保存支持命令列表
	HBuffer m_session;			//保存会话id
};

class RTSPSession;
class RTSPServer;
typedef void (*RTSPPLAYCB)(RTSPServer* thiz, RTSPSession& session);

/*会话id类*/
class RTSPSession {
public:
	RTSPSession();
	RTSPSession(const HSocket& client);
	RTSPSession(const RTSPSession& session);
	RTSPSession& operator=(const RTSPSession& session);
	~RTSPSession() {}
	int PickRequestAndReply(RTSPPLAYCB cb, RTSPServer* thiz);	//处理request请求并应答
	HAddress GetClientUDPAddress() const;

private:
	HBuffer PickOneLine(HBuffer& buffer);					//选择一行处理
	HBuffer Pick();											//选择
	RTSPRequest AnalyseRequest(const HBuffer& buffer);		//分析
	RTSPReply Reply(const RTSPRequest& request);			//回复

private:
	HBuffer m_id;		//session id
	HSocket m_client;	//对应套接字
	short m_port;
};


class RTSPServer : public ThreadFuncBase {
public:
	RTSPServer() :m_socket(true), m_status(0), m_pool(10) {
		m_threadMain.UpdateWorker(ThreadWorker(this, (FUNCTYPE)&RTSPServer::threadWorker));
		m_h264.Open("./test.h264");
	}
	int Init(const std::string& strIP = "0.0.0.0", short port = 554);	//初始化
	int Invoke();														//启动
	void Stop();														//停止
	~RTSPServer() {
		Stop();
	}

protected:
	int threadWorker();										//线程任务，return0继续，负数终止，其余警告
	int ThreadSession();									//处理session线程
	static void PlayCallBack(RTSPServer* thiz, RTSPSession& session);	//回调
	void UdpWorker(const HAddress& Client);					//udp干活

private:
	HSocket m_socket;
	HAddress m_addr;
	int m_status;											//状态：0未初始化 1完成初始化 2正在运行 3关闭
	HeThread m_threadMain;									//主线程
	HeThreadPool m_pool;									//session线程池
	static SocketIniter init;								//网络初始化单例
	CHeQueue<RTSPSession> m_lstSession;						//线程安全的队列
	RTPHelper m_helper;										//RTP
	MediaFile m_h264;										//流媒体
};

