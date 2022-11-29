#pragma once
#include "base.h"
#include "Socket.h"

class RTPHeader {
public:
	/*
	* 位域设计
	* 注意：Datasheet默认左边是高位，右边是低位
	* 位域中先声明的是低位，后声明的是高位，以字节为单位
	*/

	unsigned short csrccount : 4;	//cc计数
	unsigned short extension : 1;	//X 拓展
	unsigned short padding : 1;		//P 填充
	unsigned short version : 2;		//V 版本
	unsigned short pytype : 7;		//PT有效荷载
	unsigned short mark : 1;		//M 标记
	unsigned short serial;			//序列号
	unsigned timestamp;				//时戳
	unsigned ssrc;					//同步信源
	unsigned csrc[15];				//特约信源

public:
	RTPHeader();
	RTPHeader(const RTPHeader& header);
	RTPHeader& operator=(const RTPHeader& header);
	operator HBuffer();
};

class RTPFrame {
public:
	RTPHeader m_head;		//RTP头设计
	HBuffer m_pyload;		//RTP荷载
public:
	operator HBuffer();
};

class RTPHelper {
public:
	RTPHelper() :timestamp(0), m_udp(false) {
		m_udp.Bind(HAddress("0.0.0.0", (short)55000));
		//m_file = fopen("./out.bin", "wb+");
	}
	~RTPHelper() {
		//fclose(m_file);
	}

	int SendMediaFrame(RTPFrame& rtpframe, HBuffer& frame, const HAddress& client);

private:
	int GetFrameSepSize(HBuffer& frame);	//获取帧头大小
	int SendFrame(const HBuffer& frame, const HAddress& client);

private:
	DWORD timestamp;
	HSocket m_udp;
	FILE* m_file;
};

