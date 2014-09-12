
// File: CrashExporter.cpp
// Description: Entry point to the application. 

#include "stdafx.h"
#include "ErrorReportExporter.h"
#include "CrashInfoReader.h"
#include "..\crashrpt\strconv.h"
#include "..\crashrpt\Utility.h"

CAppModule _Module;             // WTL's application module.

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int /*nCmdShow*/ = SW_SHOWDEFAULT)
{ 
	int nRet = 0; // Return code

	// Get command line parameters.
	LPCWSTR szCommandLine = GetCommandLineW();

	// Split command line.
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(szCommandLine, &argc);

	// Check parameter count.
	if(argc!=2)
		return 1; // No arguments passed, exit.

	if(_tcscmp(argv[1], _T("/terminate"))==0)
	{
		// User wants us to find and terminate all instances of CrashExporter.exe
		return CErrorReportExporter::TerminateAllcrashExporterProcesses();
	}

	// Extract file mapping name from command line arg.    
	CString sFileMappingName = CString(argv[1]);
		
	// Create the export model that will collect crash report data 
	// and export error report(s).
	CErrorReportExporter* pExport = CErrorReportExporter::GetInstance();

	// Init the export object
	BOOL bInit = pExport->Init(sFileMappingName.GetBuffer(0));
	if(!bInit)
	{
		// Failed to init 
		delete pExport;
		return 0;
	}      

	pExport->ExportReport();

	if(pExport->GetCrashInfo()->m_bShowMeeagebox)
	{
		CString str;
		WCHAR szPath[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, szPath);
		str.Format(_T(" An error report has generated in folder\n %s\\%s"), szPath, pExport->GetErrorReportDir());
		
		MessageBox(NULL, str.GetBuffer(0), _T("CrashExporter"), MB_OK);
	}
	
	// Delete sender object.
	delete pExport;
	pExport = NULL;

	// Exit.
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{  
	HRESULT hRes = ::CoInitialize(NULL);
	// If you are running on NT 4.0 or higher you can use the following call instead to 
	// make the EXE free threaded. This means that calls come in on a random RPC thread.
	//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}

