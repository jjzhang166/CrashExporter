// VC6MFCDemo.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "VC6MFCDemo.h"
#include "VC6MFCDemoDlg.h"
#include "CrashRpt.h"
#include "assert.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVC6MFCDemoApp

BEGIN_MESSAGE_MAP(CVC6MFCDemoApp, CWinApp)
	//{{AFX_MSG_MAP(CVC6MFCDemoApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVC6MFCDemoApp construction

CVC6MFCDemoApp::CVC6MFCDemoApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVC6MFCDemoApp object

CVC6MFCDemoApp theApp;

/////////////////////////////////////////////////////////////////////////////
void InitCrashExporter()
{
	CR_INSTALL_INFO info;
	// Install crash handlers
	int nInstResult = crInstall(&info);            
	assert(nInstResult==0);
	
	nInstResult = crAddScreenshot(CR_AS_MAIN_WINDOW);
	assert(nInstResult==0);
	
}

void UnInitCrashExporter()
{
	// Uninstall crash reporting
	crUninstall();
}

int CVC6MFCDemoApp::Run() 
{
	// Call your crInstall code here ...

	InitCrashExporter();

	return TRUE;
}

// CVC6MFCDemoApp initialization

BOOL CVC6MFCDemoApp::InitInstance()
{
	// Standard initialization
	Run();
	CVC6MFCDemoDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}
	UnInitCrashExporter();
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}