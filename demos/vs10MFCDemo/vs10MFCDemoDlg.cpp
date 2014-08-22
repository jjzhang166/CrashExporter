
// VS10MFCDemoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "vs10MFCDemo.h"
#include "vs10MFCDemoDlg.h"
#include "afxdialogex.h"
#include "CrashRpt.h"
#include "..\EmulateCrash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVS10MFCDemoDlg 对话框




CVS10MFCDemoDlg::CVS10MFCDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CVS10MFCDemoDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVS10MFCDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_THREAD, m_cboThread);
	DDX_Control(pDX, IDC_EXCTYPE, m_cboExcType);
}

BEGIN_MESSAGE_MAP(CVS10MFCDemoDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_CRASH, &CVS10MFCDemoDlg::OnBnClickedCrash)
END_MESSAGE_MAP()



void CVS10MFCDemoDlg::set_DropDownSize(CComboBox& box, UINT LinesToDisplay) 

{ 
	ASSERT(IsWindow(box)); // Window must exist or SetWindowPos won't work 

	CRect cbSize; // current size of combo box 
	int Height; // new height for drop-down portion of combo box 

	box.GetClientRect(cbSize); 
	Height = box.GetItemHeight(-1); // start with size of the edit-box portion 
	Height += box.GetItemHeight(0) * LinesToDisplay; // add height of lines of text 

	// Note: The use of SM_CYEDGE assumes that we're using Windows '95 
	// Now add on the height of the border of the edit box 
	Height += GetSystemMetrics(SM_CYEDGE) * 2; // top & bottom edges 

	// The height of the border of the drop-down box 
	Height += GetSystemMetrics(SM_CYEDGE) * 2; // top & bottom edges 

	// now set the size of the window 
	box.SetWindowPos(NULL, // not relative to any other windows 
		0, 0, // TopLeft corner doesn't change 
		cbSize.right, Height, // existing width, new height 
		SWP_NOMOVE | SWP_NOZORDER // don't move box or change z-ordering. 
		); 
}

BOOL CVS10MFCDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	int nItem = 0;

	set_DropDownSize(m_cboThread, 3);
	nItem = m_cboThread.InsertString(0,_T("Main thread"));
	m_cboThread.SetItemData(nItem, 0);

	nItem = m_cboThread.InsertString(1, _T("Worker thread")); 
	m_cboThread.SetItemData(nItem, 1);

	m_cboThread.SetCurSel(0); 

	set_DropDownSize(m_cboExcType, 8);
	nItem = m_cboExcType.InsertString(0,_T("0 - SEH exception"));
	m_cboExcType.SetItemData(nItem, CR_SEH_EXCEPTION);

	nItem = m_cboExcType.InsertString(1, _T("1 - terminate"));
	m_cboExcType.SetItemData(nItem, CR_CPP_TERMINATE_CALL);

	nItem = m_cboExcType.InsertString(2, _T("2 - unexpected"));
	m_cboExcType.SetItemData(nItem, CR_CPP_PURE_CALL);

	nItem = m_cboExcType.InsertString(3, _T("3 - pure virtual method call")); 
	m_cboExcType.SetItemData(nItem, CR_CPP_PURE_CALL);

	nItem = m_cboExcType.InsertString(4, _T("4 - invalid parameter"));
	m_cboExcType.SetItemData(nItem, CR_CPP_INVALID_PARAMETER);

	nItem = m_cboExcType.InsertString(5, _T("5 - new operator fault"));
	m_cboExcType.SetItemData(nItem, CR_CPP_NEW_OPERATOR_ERROR);

	nItem = m_cboExcType.InsertString(6, _T("6 - SIGABRT")); 
	m_cboExcType.SetItemData(nItem, CR_CPP_SIGABRT);

	nItem = m_cboExcType.InsertString(7, _T("7 - SIGFPE"));
	m_cboExcType.SetItemData(nItem, CR_CPP_SIGFPE);

	nItem = m_cboExcType.InsertString(8, _T("8 - SIGILL")); 
	m_cboExcType.SetItemData(nItem, CR_CPP_SIGILL);

	nItem = m_cboExcType.InsertString(9, _T("9 - SIGINT"));
	m_cboExcType.SetItemData(nItem, CR_CPP_SIGINT);

	nItem = m_cboExcType.InsertString(10, _T("10 - SIGSEGV")); 
	m_cboExcType.SetItemData(nItem, CR_CPP_SIGSEGV);

	nItem = m_cboExcType.InsertString(11,_T("11 - SIGTERM"));
	m_cboExcType.SetItemData(nItem, CR_CPP_SIGTERM);

	nItem = m_cboExcType.InsertString(12, _T("12 - RaiseException")); 
	m_cboExcType.SetItemData(nItem, CR_NONCONTINUABLE_EXCEPTION);

	nItem = m_cboExcType.InsertString(13, _T("13 - throw C++ typed exception")); 
	m_cboExcType.SetItemData(nItem, CR_THROW);

	nItem = m_cboExcType.InsertString(14, _T("14 - Manual report")); 
	m_cboExcType.SetItemData(nItem, MANUAL_REPORT);

	m_cboExcType.SetCurSel(0); 

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CVS10MFCDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CVS10MFCDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//////////////////////////////////////////////////////////////////////////


void CVS10MFCDemoDlg::OnBnClickedCrash()
{
	int nSel = m_cboThread.GetCurSel();
	int nThread = (int)m_cboThread.GetItemData(nSel);

	nSel = m_cboExcType.GetCurSel();
	int nExcType = (int)m_cboExcType.GetItemData(nSel);
	
	if(0 == nThread) // The main thread
	{    
		EmulateCrash(nExcType);
	}
	else if(1 == nThread)// Worker thread
	{
		HANDLE hWorkingThread = NULL;
		CrashThreadInfo _CrashThreadInfo;
		/* Create another thread */
		_CrashThreadInfo.m_bStop = false;
		_CrashThreadInfo.m_hWakeUpEvent = CreateEvent(NULL, FALSE, FALSE, _T("WakeUpEvent"));
		ASSERT(_CrashThreadInfo.m_hWakeUpEvent != NULL);

		DWORD dwThreadId = 0;
		hWorkingThread = CreateThread(NULL, 0, CrashThread, (LPVOID)&_CrashThreadInfo, 0, &dwThreadId);
		ASSERT(hWorkingThread != NULL);

		_CrashThreadInfo.m_ExceptionType = nExcType;
		SetEvent(_CrashThreadInfo.m_hWakeUpEvent); // wake up the working thread	

		WaitForSingleObject(hWorkingThread, 10000);
		_CrashThreadInfo.m_bStop = true;
	}
}
