#pragma once
#include "vlc.h"
#include <string>


/*考虑CSize有的环境若是没有项目没法跑，改为自定义类增加可移植性*/
class VlcSize {
public:
	int nWidth;
	int nHeight;
	VlcSize(int width = 0, int height = 0) {
		nWidth = width;
		nHeight = height;
	}
	VlcSize(const VlcSize& size) {
		nWidth = size.nWidth;
		nHeight = size.nHeight;
	}
	VlcSize& operator=(const VlcSize& size) {
		if (this != &size) {
			nWidth = size.nWidth;
			nHeight = size.nHeight;
		}
	}
};

class HVlc{
public:
	HVlc();
	~HVlc();

	int SetMedia(const std::string& strUrl);		//设置媒体 注：包含中文请传入utf-8编码格式
#ifdef WIN32
	int SetHwnd(HWND hWnd);							//设置播放的窗口
#endif
	int Play();										//播放
	int Pause();									//暂停
	int Stop();										//停止
	float GetPosition();							//获取进度
	void SetPostion(float pos);						//设置进度
	int GetVolume();								//获取音量
	int SetVolume(int volume);						//设置音量
	VlcSize GetMeidaInfo();							//获取媒体宽高
	float GetLength();								//获取长度
	std::string UnicodeToUtf8(const std::wstring& strIn);	//编码转换

protected:
	libvlc_instance_t* m_instance;					//实例化对象
	libvlc_media_t* m_media;						//实例化媒体
	libvlc_media_player_t* m_player;				//实例化播放器
	std::string m_Url;								//保存媒体地址

#ifdef WIN32
	HWND m_hWnd;
#endif
};

