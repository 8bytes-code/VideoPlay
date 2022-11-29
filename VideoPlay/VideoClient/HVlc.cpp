#include "pch.h"
#include "HVlc.h"

HVlc::HVlc(){
	//tips:��ʼ��ʵ�����Ķ���
	m_instance = libvlc_new(0, NULL);
	m_media = NULL;
	m_player = NULL;
	m_hWnd = NULL;
}

HVlc::~HVlc(){
	//��Ϊ��ȫ���ͷ�
	if (m_player != NULL) {
		libvlc_media_player_t* temp = m_player;
		m_player = NULL;
		libvlc_media_player_release(temp);
	}
	if (m_media != NULL) {
		libvlc_media_t* temp = m_media;
		m_media = NULL;
		libvlc_media_release(temp);
	}
	if (m_instance != NULL) {
		libvlc_instance_t* temp = m_instance;
		m_instance = NULL;
		libvlc_release(temp);
	}
}

int HVlc::SetMedia(const std::string& strUrl){
	if (m_instance == NULL || m_hWnd == NULL)return -1;

	//������ñ�����¼��ַ����У��
	if (strUrl == m_Url)return 0;
	m_Url = strUrl;

	//һ��ֻ�ܲ���һ��������ǰ��ǰ���������ȸɵ�
	if (m_media != NULL) {
		libvlc_media_release(m_media);	
		m_media = NULL;
	}
	m_media = libvlc_media_new_location(m_instance, strUrl.c_str());
	if (!m_media) return -2;

	//ͬ��������ҲҪע����Դ��ʱ���٣�������������ڴ�й¶
	if (m_player != NULL) {
		libvlc_media_player_release(m_player);
		m_player = NULL;
	}
	m_player = libvlc_media_player_new_from_media(m_media);
	if (!m_player) return -3;

	//��������
	CRect rect;
	GetWindowRect(m_hWnd, rect);
	std::string strRatio = "";
	strRatio.resize(32);
	sprintf((char*)strRatio.c_str(), "%f:%d", rect.Width()*0.8, rect.Height());
	libvlc_video_set_aspect_ratio(m_player, strRatio.c_str());
	//���ô���Ҫע��ʱ��
	libvlc_media_player_set_hwnd(m_player, m_hWnd);

	return 0;
}

#ifdef WIN32
int HVlc::SetHwnd(HWND hWnd) {
	m_hWnd = hWnd;	
	return 0;
}
#endif

int HVlc::Play() {
	if (!m_player || !m_instance || !m_media) return -1;
	return libvlc_media_player_play(m_player);
}

int HVlc::Pause() {
	if (!m_player || !m_instance || !m_media) return -1;
	libvlc_media_player_pause(m_player);
	return 0;
}

int HVlc::Stop() {
	if (!m_player || !m_instance || !m_media) return -1;
	libvlc_media_player_stop(m_player);
	return 0;
}

float HVlc::GetPosition() {
	if (!m_player || !m_instance || !m_media) return -1.0f;
	return libvlc_media_player_get_position(m_player);
}

void HVlc::SetPostion(float pos) {
	if (!m_player || !m_instance || !m_media) return;
	libvlc_media_player_set_position(m_player, pos);
}

int HVlc::GetVolume() {
	if (!m_player || !m_instance || !m_media) return -1;
	return libvlc_audio_get_volume(m_player);
}

int HVlc::SetVolume(int volume) {
	if (!m_player || !m_instance || !m_media) return -1;
	return libvlc_audio_set_volume(m_player, volume);
}

VlcSize HVlc::GetMeidaInfo() {
	if (!m_player || !m_instance || !m_media) return VlcSize(-1,-1);
	return VlcSize(
		libvlc_video_get_width(m_player), 
		libvlc_video_get_height(m_player)
	);
}

float HVlc::GetLength() {
	if (!m_player || !m_instance || !m_media) return -1.0f;
	libvlc_time_t tm = libvlc_media_player_get_length(m_player);
	float ret = tm / 1000.0f;	//tick����msΪ��λ
	return ret;
}

std::string HVlc::UnicodeToUtf8(const std::wstring& strIn) {
	std::string strOut;
	//������������Ȼ�ȡ��ת��֮��ĳ���
	int length = ::WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), strIn.size(), NULL, 0, NULL, NULL);
	strOut.resize(length + 1);
	//�ٰ�ת�������ݴ��ݸ�strout
	::WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), strIn.size(), (LPSTR)strOut.c_str(), length, NULL, NULL);

	return strOut;
}
