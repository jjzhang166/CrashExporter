
// VS10MFCDemoDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"

// CVS10MFCDemoDlg �Ի���
class CVS10MFCDemoDlg : public CDialogEx
{
// ����
public:
	CVS10MFCDemoDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_VS10MFCDEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
