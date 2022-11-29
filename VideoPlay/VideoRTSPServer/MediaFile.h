#pragma once
#include "Socket.h"

class MediaFile {
public:
	MediaFile();
	~MediaFile();

	int Open(const HBuffer& path, int nType = 96);	//打开文件
	HBuffer ReadOneFrame();							//读一帧,buffer.size()=0，说明没有帧
	void Close();									//关闭
	void Reset();									//重置，ReadOneFrame又可以继续

protected:
	//因格式不同可以通过ReadOneFrame调用不同方法，拓展
	
	long FindH264Head(int& headsize);		//读取h264头部，返回-1表示查找失败
	HBuffer ReadH264Frame();				//读取h264的帧

private:
	long m_size;
	FILE* m_file;
	HBuffer m_filepath;
	//96:h264
	int m_type;				
};

