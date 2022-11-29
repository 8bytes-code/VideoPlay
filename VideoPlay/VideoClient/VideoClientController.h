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

	int Init(CWnd*& pWnd);						//��ʼ��
	int Invoke();								//����
	float VideoCtrl(HVlcCommand cmd);			//���ƹ��ܣ�����-1.0f��ʾ����
	int SetMedia(const std::string& strUrl);	//����ý��·����Դ��ע������������ʹ��utf-8����
	void SetPosition(float pos);				//���ý���
	int SetWnd(HWND hWnd);						//���ô���
	int SetVolume(int volume);					//��������
	VlcSize GetMediaInfo();						//��ȡý���С���ݣ���ߣ�
	std::string UnicodeToUtf8(const std::wstring& strIn);	//����ת����HVLC��������ʵ�֣��˴�ֻ�ǵ���

protected:
	HVlc m_hvlc;			//��װ���vlc����
	CVideoClientDlg m_dlg;	//��Ŀ�Դ��Ӵ��ڶ���
};

