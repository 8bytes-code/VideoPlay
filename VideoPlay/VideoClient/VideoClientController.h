#pragma once
#include "HVlc.h"
#include "VideoClientDlg.h"

enum HVlcCommand {
	HVLC_PLAY,
	HVLC_PAUSE,
	HVLC_STOP,
	HVLC_GET_VOLUME,
	HVLC_GET_POSITION,
	HVLC_GET_LENGTH
};

class VideoClientController {
public:
	VideoClientController();
	~VideoClientController();

	int Init(CWnd*& pWnd);						//初始化
	int Invoke();								//启动
	float VideoCtrl(HVlcCommand cmd);			//控制功能，返回-1.0f表示错误
	int SetMedia(const std::string& strUrl);	//设置媒体路径来源，注：包含中文请使用utf-8编码
	void SetPosition(float pos);				//设置进度
	int SetWnd(HWND hWnd);						//设置窗口
	int SetVolume(int volume);					//设置音量
	VlcSize GetMediaInfo();						//获取媒体大小数据（宽高）
	std::string UnicodeToUtf8(const std::wstring& strIn);	//编码转换，HVLC是真正的实现，此处只是调用

protected:
	HVlc m_hvlc;			//封装后的vlc对象
	CVideoClientDlg m_dlg;	//项目自带从窗口对象
};

