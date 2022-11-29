#include "RTPHelper.h"
#include <Windows.h>

#define RTP_MAX_SIZE 1300

int RTPHelper::SendMediaFrame(RTPFrame& rtpframe, HBuffer& frame, const HAddress& client) {
	size_t frame_size = frame.size();
	int sepsize = GetFrameSepSize(frame);
	frame_size -= sepsize;	//减掉帧头就是后面的数据长度
	BYTE* pFrame = sepsize + (BYTE*)frame;
	if (frame_size > RTP_MAX_SIZE) {
		//需要分片
		BYTE nalu = pFrame[0] & 0x1F;
		size_t count = frame_size / RTP_MAX_SIZE;
		size_t restsize = frame_size % RTP_MAX_SIZE;
		for (size_t i = 0; i < count; i++) {
			rtpframe.m_pyload.resize(RTP_MAX_SIZE + 2);
			((BYTE*)rtpframe.m_pyload)[0] = 0x60 | 28;		//0110 0000 | 0001 1100
			((BYTE*)rtpframe.m_pyload)[1] = nalu;			//0000 0000 middle
			if (i == 0)
				((BYTE*)rtpframe.m_pyload)[1] |= 0x80;		//1000 0000 start
			else if ((restsize == 0) && (i == count - 1))
				((BYTE*)rtpframe.m_pyload)[1] |= 0x40;		//0100 0000 end

			memcpy(2 + (BYTE*)rtpframe.m_pyload, pFrame + RTP_MAX_SIZE * i + 1, RTP_MAX_SIZE);
			//发送数据
			SendFrame(rtpframe, client);
			//序号是累加得到，时间戳靠计算，从0开始，每帧追加(时钟频率90000)/(每秒帧数24)
			rtpframe.m_head.serial++;
		}
		if (restsize > 0) {
			//处理尾部
			rtpframe.m_pyload.resize(restsize + 2);
			((BYTE*)rtpframe.m_pyload)[0] = 0x60 | 28;		//0110 0000 | 0001 1100
			((BYTE*)rtpframe.m_pyload)[1] = nalu;			//0000 0000 middle
			((BYTE*)rtpframe.m_pyload)[1] = 0x40 | ((BYTE*)rtpframe.m_pyload)[1];	//0100 0000 end
			memcpy(2 + (BYTE*)rtpframe.m_pyload, pFrame + RTP_MAX_SIZE * count + 1, restsize);
			//发送数据
			SendFrame(rtpframe, client);
			rtpframe.m_head.serial++;
		}
	}
	else {
		//小包可以直接发
		rtpframe.m_pyload.resize(frame.size() - sepsize);
		memcpy(rtpframe.m_pyload, pFrame, frame.size() - sepsize);
		//发送数据
		SendFrame(rtpframe, client);
		rtpframe.m_head.serial++;

	}
	//时间戳计算，通过累加(int)(时钟频率/每秒帧数)
	rtpframe.m_head.timestamp += 90000 / 24;
	//发送后，加入休眠，等待发送完成，控制发送速度
	Sleep(1000 / 30);
	return 0;
}

int RTPHelper::GetFrameSepSize(HBuffer& frame) {
	BYTE buf[] = { 0,0,0,1 };
	if (memcmp(frame, buf, 4) == 0) return 4;
	return 3;
}

int RTPHelper::SendFrame(const HBuffer& frame, const HAddress& client) {
	//fwrite(frame, 1, frame.size(), m_file);
	//fwrite("00000000", 1, 8, m_file);
	//fflush(m_file);	//刷新缓存，写入硬盘
	int ret = sendto(m_udp, frame, frame.size(), 0, client, client.Size());
	//printf("ret:%d, size:%d, ip:%s, port:%d\r\n", ret, frame.size(), client.Ip().c_str(), client.Port());
	return 0;
}

RTPHeader::RTPHeader() {
	csrccount = 0;
	extension = 0;
	padding = 0;
	version = 2;
	pytype = 96;
	mark = 0;
	serial = 0;
	timestamp = 0;
	ssrc = 0x98765432;
	memset(csrc, 0, sizeof(csrc));
}

RTPHeader::RTPHeader(const RTPHeader& header) {
	memset(csrc, 0, sizeof(csrc));
	int size = 14 + 4 * csrccount;
	memcpy(this, &header, size);
}

RTPHeader& RTPHeader::operator=(const RTPHeader& header) {
	if (this != &header) {
		int size = 14 + 4 * csrccount;
		memcpy(this, &header, size);
	}
	return *this;
}

RTPHeader::operator HBuffer() {
	RTPHeader header = *this;
	header.serial = htons(header.serial);
	header.timestamp = htonl(header.timestamp);
	header.ssrc = htonl(header.ssrc);
	int size = 12 + 4 * csrccount;
	HBuffer result(size);
	memcpy(result, &header, size);
	return result;
}

RTPFrame::operator HBuffer() {
	HBuffer result;
	result += (HBuffer)m_head;
	result += m_pyload;
	return result;
}
