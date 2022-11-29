#include "pch.h"
#include "VideoClientController.h"

VideoClientController::VideoClientController() {
	m_dlg.m_controller = this;
}

VideoClientController::~VideoClientController() {
}

int VideoClientController::Init(CWnd*& pWnd) {
	pWnd = &m_dlg;
	return 0;
}

int VideoClientController::Invoke() {
	INT_PTR nResponse = m_dlg.DoModal();
	if (nResponse == IDOK) {
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȷ�������رնԻ���Ĵ���
	}
	else if (nResponse == IDCANCEL) {
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȡ�������رնԻ���Ĵ���
	}
	else if (nResponse == -1) {
		TRACE(traceAppMsg, 0, "����: �Ի��򴴽�ʧ�ܣ�Ӧ�ó���������ֹ��\n");
		TRACE(traceAppMsg, 0, "����: ������ڶԻ�����ʹ�� MFC �ؼ������޷� #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS��\n");
	}
	return nResponse;
}

int VideoClientController::SetMedia(const std::string& strUrl) {
	return m_hvlc.SetMedia(strUrl);
}

float VideoClientController::VideoCtrl(HVlcCommand cmd) {
	switch (cmd) {
		case HVLC_PLAY:
			return float(m_hvlc.Play());
			break;
		case HVLC_PAUSE:
			return float(m_hvlc.Pause());
			break;
		case HVLC_STOP:
			return float(m_hvlc.Stop());
			break;
		case HVLC_GET_VOLUME:
			return float(m_hvlc.GetVolume());
			break;
		case HVLC_GET_POSITION:
			return float(m_hvlc.GetPosition());
			break;
		case HVLC_GET_LENGTH:
			return m_hvlc.GetLength();
			break;
		default:break;
	}
	return -1.0f;
}

void VideoClientController::SetPosition(float pos) {
	m_hvlc.SetPostion(pos);
}

int VideoClientController::SetWnd(HWND hWnd) {
	return m_hvlc.SetHwnd(hWnd);
}

int VideoClientController::SetVolume(int volume) {
	return m_hvlc.SetVolume(volume);
}

VlcSize VideoClientController::GetMediaInfo() {
	return m_hvlc.GetMeidaInfo();
}

std::string VideoClientController::UnicodeToUtf8(const std::wstring& strIn) {
	return m_hvlc.UnicodeToUtf8(strIn);
}
