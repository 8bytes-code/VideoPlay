
// VideoClientDlg.h: 头文件
//

#pragma once
class VideoClientController;

// CVideoClientDlg 对话框
class CVideoClientDlg : public CDialogEx
{
// 构造
public:
	CVideoClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIDEOCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	bool m_status;	//按钮状态

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnPlay();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnThumbposchangingSliderPos(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnThumbposchangingSliderVolume(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

public:
	VideoClientController* m_controller;

protected:
	//dlg控件变量
	CEdit m_video;
	CSliderCtrl m_pos;
	CSliderCtrl m_volume;
	CEdit m_url;
	CButton m_playbtn;

	float m_length;
};
