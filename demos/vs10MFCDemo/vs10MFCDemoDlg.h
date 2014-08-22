
// VS10MFCDemoDlg.h : 头文件
//

#pragma once
#include "afxwin.h"

// CVS10MFCDemoDlg 对话框
class CVS10MFCDemoDlg : public CDialogEx
{
// 构造
public:
	CVS10MFCDemoDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_VS10MFCDEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCrash();
	void set_DropDownSize(CComboBox& box, UINT LinesToDisplay);
	CComboBox m_cboThread;
	CComboBox m_cboExcType;
};
