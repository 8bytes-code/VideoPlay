#pragma once
#include "vlc.h"
#include <string>


/*����CSize�еĻ�������û����Ŀû���ܣ���Ϊ�Զ��������ӿ���ֲ��*/
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

	int SetMedia(const std::string& strUrl);		//����ý�� ע�����������봫��utf-8�����ʽ
#ifdef WIN32
	int SetHwnd(HWND hWnd);							//���ò��ŵĴ���
#endif
	int Play();										//����
	int Pause();									//��ͣ
	int Stop();										//ֹͣ
	float GetPosition();							//��ȡ����
	void SetPostion(float pos);						//���ý���
	int GetVolume();								//��ȡ����
	int SetVolume(int volume);						//��������
	VlcSize GetMeidaInfo();							//��ȡý����
	float GetLength();								//��ȡ����
	std::string UnicodeToUtf8(const std::wstring& strIn);	//����ת��

protected:
	libvlc_instance_t* m_instance;					//ʵ��������
	libvlc_media_t* m_media;						//ʵ����ý��
	libvlc_media_player_t* m_player;				//ʵ����������
	std::string m_Url;								//����ý���ַ

#ifdef WIN32
	HWND m_hWnd;
#endif
};

