
// D3DMFCDlg.h: 头文件
//

#pragma once
#include "D3DApp.h"


// CD3DMFCDlg 对话框
class CD3DMFCDlg : public CDialogEx
{
// 构造
public:
	CD3DMFCDlg(CWnd* pParent = nullptr);	// 标准构造函数
	~CD3DMFCDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_D3DMFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	virtual BOOL PreTranslateMessage(MSG* pMsg);

// 实现
protected:
	HICON m_hIcon;
	D3DApp* app;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CBrush m_brush;
	afx_msg void OnBnClickedMfccolorbutton1();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
//	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
};