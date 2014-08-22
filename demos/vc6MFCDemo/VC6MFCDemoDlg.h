// VC6MFCDemoDlg.h : header file
//

#if !defined(AFX_VC6MFCDEMODLG_H__8A2DFC8E_D5C9_4FCE_AFC8_F522AA34DCB3__INCLUDED_)
#define AFX_VC6MFCDEMODLG_H__8A2DFC8E_D5C9_4FCE_AFC8_F522AA34DCB3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CVC6MFCDemoDlg dialog

class CVC6MFCDemoDlg : public CDialog
{
// Construction
public:
	CVC6MFCDemoDlg(CWnd* pParent = NULL);	// standard constructor
	void set_DropDownSize(CComboBox& box, UINT LinesToDisplay);

// Dialog Data
	//{{AFX_DATA(CVC6MFCDemoDlg)
	enum { IDD = IDD_VC6MFCDEMO_DIALOG };
	CComboBox	m_cboExcType;
	CComboBox	m_cboThread;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVC6MFCDemoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CVC6MFCDemoDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnbtnCrash();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VC6MFCDEMODLG_H__8A2DFC8E_D5C9_4FCE_AFC8_F522AA34DCB3__INCLUDED_)
