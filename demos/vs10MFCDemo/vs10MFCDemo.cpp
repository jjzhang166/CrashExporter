
// vs10MFCDemo.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "vs10MFCDemo.h"
#include "VS10MFCDemoDlg.h"
#include "CrashRpt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVS10MFCDemoApp

BEGIN_MESSAGE_MAP(CVS10MFCDemoApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CVS10MFCDemoApp 构造

CVS10MFCDemoApp::CVS10MFCDemoApp()
{

}


CVS10MFCDemoApp theApp;


int CVS10MFCDemoApp::Run() 
{
	// Call your crInstall code here ...

	CR_INSTALL_INFO info;
	// Install crash handlers
	int nInstResult = crInstall(&info);            
	//assert(nInstResult==0);

	nInstResult = crAddScreenshot(CR_AS_MAIN_WINDOW);
	//assert(nInstResult==0);

	// Check result
	if(nInstResult!=0)
	{
		AfxMessageBox(_T("Install crashrpt failed"));
		return FALSE;
	}

	return 0;
}

BOOL CVS10MFCDemoApp::InitInstance()
{
	CWinApp::InitInstance();
	Run();

	CVS10MFCDemoDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{

	}
	else if (nResponse == IDCANCEL)
	{
		// Uninstall crash reporting
		crUninstall();
	}

	return FALSE;
}

