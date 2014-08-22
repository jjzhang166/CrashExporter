
#include "stdafx.h"
#include "ErrorReportExporter.h"
#include "CrashRpt.h"
#include "Utility.h"
#include "CrashInfoReader.h"
#include "strconv.h"
#include "ScreenCap.h"
#include <sys/stat.h>
#include "dbghelp.h"

#define OutputErrorStr(...) \
	Utility::OutDebugStr(_T("%s%s"), _T("[CrashExporter] [CErrorReportExporter] "), __VA_ARGS__)

CErrorReportExporter* CErrorReportExporter::m_pInstance = NULL;

// Constructor
CErrorReportExporter::CErrorReportExporter()
	: m_hThread(NULL), m_Action(COLLECT_CRASH_INFO)
{       
}

// Destructor
CErrorReportExporter::~CErrorReportExporter()
{
	Finalize();
}

CErrorReportExporter* CErrorReportExporter::GetInstance()
{
	// Return singleton object
	if(m_pInstance==NULL)	
		m_pInstance = new CErrorReportExporter();
	return m_pInstance;
}

BOOL CErrorReportExporter::Init(LPCTSTR szFileMappingName)
{		
	// Read crash information from the file mapping object.
	int nInit = m_CrashInfo.Init(szFileMappingName);
	if(0 != nInit)
	{
		OutputErrorStr(_T("Error reading crash info"));
		return FALSE;
	}

	CString     sCrashReportsFolder = _T("crashrpt"); // Path to CrashReports folder for the application.
	// Create CrashReports folder (if doesn't exist yet).
	BOOL bCreateFolder = Utility::CreateFolder(sCrashReportsFolder);
	if(!bCreateFolder)
	{
		OutputErrorStr(_T("Create folder %s failed."), sCrashReportsFolder);
		return 1; 
	}

	time_t t = time(NULL);
	tm timeinfo = {0}; 
	localtime_s(&timeinfo, &t);
	WCHAR achFolderName[128] = {0};
	_tcsftime(achFolderName, 128, _T("%Y-%m-%d_%H-%M-%S"), &timeinfo);

	m_sErrorReportDirName = sCrashReportsFolder + _T("\\") + achFolderName;

	bCreateFolder = Utility::CreateFolder(m_sErrorReportDirName);	
	if(!bCreateFolder)
	{
		OutputErrorStr(_T("Create folder %s failed."), m_sErrorReportDirName);
		return 2; 
	}
		
	SetProcessDefaultLayout(LAYOUT_RTL); 

	// Start crash info collection work assynchronously
	DoWorkAssync(COLLECT_CRASH_INFO);

	return TRUE;
}
// This method performs crash files collection and/or
// error report exporting work in a worker thread.
BOOL CErrorReportExporter::DoWorkAssync(int nAction)
{
	// Save the action code
	m_Action = nAction;

	// Create worker thread which will do all work assynchronously
	m_hThread = CreateThread(NULL, 0, WorkerThread, (LPVOID)this, 0, NULL);

	// Check if the thread was created ok
	if(m_hThread==NULL)
		return FALSE; // Failed to create worker thread

	// Done, return
	return TRUE;
}

// This method is the worker thread procedure that delegates further work 
// back to the CErrorReportExporter class
DWORD WINAPI CErrorReportExporter::WorkerThread(LPVOID lpParam)
{
	// Delegate the action to the CErrorReportExporter::DoWorkAssync() method
	CErrorReportExporter* pExport = (CErrorReportExporter*)lpParam;
	pExport->DoWork(pExport->m_Action);
	pExport->m_hThread = NULL; // clean up
	// Exit code can be ignored
	return 0;
}

// This method unblocks the parent process
void CErrorReportExporter::UnblockParentProcess()
{
	// Notify the parent process that we have finished with minidump,
	// so the parent process is able to unblock and terminate itself.

	m_Assync.SetProgress(_T("[Start UnblockParentProcess...]"));

	// Open the event the parent process had created for us
	CString sEventName;
	sEventName.Format(_T("Local\\CrashRptEvent_%s"), m_CrashInfo.m_sCrashGUID);
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, sEventName);
	if(NULL != hEvent)
		SetEvent(hEvent); // Signal event

	m_Assync.SetProgress(_T("[Finish UnblockParentProcess]"), FinishUnblockParentProcess);
}

// This method collects required crash report files (minidump, screenshot etc.)
// and then export the error report.
BOOL CErrorReportExporter::DoWork(int Action)
{
	// Reset the completion event
	m_Assync.Reset();

	if(Action&COLLECT_CRASH_INFO) // Collect crash report files
	{
		m_Assync.SetProgress(_T("[Start COLLECT_CRASH_INFO...]"));

		// First take a screenshot of user's desktop (if needed).
		TakeDesktopScreenshot();

		// Create crash dump.
		CreateMiniDump();

		// Create Crash Info.
		CreateCrashInfo();	

		// Notify the parent process that we have finished with minidump,
		// so the parent process is able to unblock and terminate itself.
		UnblockParentProcess();
	
		m_Assync.SetProgress(_T("[Finish COLLECT_CRASH_INFO]"), FinishCollectCrashInfo);
	}

	if(Action&RESTART_APP) // We need to restart the parent process
	{ 
		m_Assync.SetProgress(_T("[Start RESTART_APP...]"));
		RestartApp();
		m_Assync.SetProgress(_T("[Finish RESTART_APP]"), FinishRestartApp);
	}

	// Done
	return TRUE;
}

// This method blocks until worker thread is exited
void CErrorReportExporter::WaitForCompletion()
{	
	if(m_hThread!=NULL)
		WaitForSingleObject(m_hThread, INFINITE);	
}

BOOL CErrorReportExporter::Finalize()
{  
	// Wait until worker thread exits.
	WaitForCompletion();

	// If needed, restart the application
	DoWork(RESTART_APP); 

	m_Assync.SetProgress(_T("[Finish ErrorReportExport]"), 100); 

	return TRUE;
}

// This method takes the desktop screenshot (screenshot of entire virtual screen
// or screenshot of the main window). 
BOOL CErrorReportExporter::TakeDesktopScreenshot()
{
	CScreenCapture sc; // Screen capture object
	ScreenshotInfo ssi; // Screenshot params    

	m_Assync.SetProgress(_T("[taking_screenshot]"));    

	// Check if screenshot capture is allowed
	if(!m_CrashInfo.m_bAddScreenshot)
	{
		// Add a message to log
		m_Assync.SetProgress(_T("Desktop screenshot generation disabled; skipping."));    
		// Exit, nothing to do here
		return TRUE;
	}

	// Add a message to log
	m_Assync.SetProgress(_T("Taking desktop screenshot"));    

	// Get screenshot flags passed by the parent process
	DWORD dwFlags = m_CrashInfo.m_dwScreenshotFlags;

	// Determine what to use - color or grayscale image
	BOOL bGrayscale = (dwFlags&CR_AS_GRAYSCALE_IMAGE)!=0;

	SCREENSHOT_TYPE type = SCREENSHOT_TYPE_VIRTUAL_SCREEN;
	if(0 != (dwFlags&CR_AS_MAIN_WINDOW)) // We need to capture the main window
		type = SCREENSHOT_TYPE_MAIN_WINDOW;
	else if(0 != (dwFlags&CR_AS_PROCESS_WINDOWS)) // Capture all process windows
		type = SCREENSHOT_TYPE_ALL_PROCESS_WINDOWS;
	else // (dwFlags&CR_AS_VIRTUAL_SCREEN)!=0 // Capture the virtual screen
		type = SCREENSHOT_TYPE_VIRTUAL_SCREEN;
	
	// Take the screen shot
	BOOL bTakeScreenshot = sc.TakeDesktopScreenshot(
		m_sErrorReportDirName, ssi, type, m_CrashInfo.m_dwProcessId, bGrayscale);
	if(FALSE == bTakeScreenshot)
	{
		OutputErrorStr(_T("TakeDesktopScreenshot failed."));
		return FALSE;
	}
	m_Assync.SetProgress(_T("Take desktop screenshot done"), FinishTakeDesktopScreenshot); 

	// Done
	return TRUE;
}

// This callback function is called by MinidumpWriteDump
BOOL CALLBACK CErrorReportExporter::MiniDumpCallback(
	PVOID CallbackParam,
	PMINIDUMP_CALLBACK_INPUT CallbackInput,
	PMINIDUMP_CALLBACK_OUTPUT CallbackOutput )
{
	// Delegate back to the CErrorReportExporter
	CErrorReportExporter* pErrorReportExporter = (CErrorReportExporter*)CallbackParam;  
	return pErrorReportExporter->OnMinidumpProgress(CallbackInput, CallbackOutput);  
}

// This method is called when MinidumpWriteDump notifies us about
// currently performed action
BOOL CErrorReportExporter::OnMinidumpProgress(const PMINIDUMP_CALLBACK_INPUT CallbackInput,
											PMINIDUMP_CALLBACK_OUTPUT CallbackOutput)
{
	CallbackOutput;		// avoid warning
	switch(CallbackInput->CallbackType)
	{
	case ModuleCallback:
		{
			// We are currently dumping some module
			strconv_t strconv;
			CString sMsg;
			sMsg.Format(_T("Dumping info for module %s"), 
				strconv.w2t(CallbackInput->Module.FullPath));
			// Update progress
			m_Assync.SetProgress(sMsg);
		}
		break;
	case ThreadCallback:
		{      
			// We are currently dumping some thread 
			CString sMsg;
			sMsg.Format(_T("Dumping info for thread 0x%X"), 
				CallbackInput->Thread.ThreadId);
			m_Assync.SetProgress(sMsg);
		}
		break;

	}

	return TRUE;
}
// Create CrashInfo.
BOOL CErrorReportExporter::CreateCrashInfo()
{
	m_Assync.SetProgress(_T("[Creating CrashInfo...]"));  

	if(FALSE == m_CrashInfo.m_bGenerateCrashWalk)
	{
		m_Assync.SetProgress(_T("Crash info generation disabled; skipping."));
		return FALSE;
	}

	BOOL bStatus = FALSE;
	HANDLE hFile = NULL;
	CString sStackWalkerFile = m_sErrorReportDirName + _T("\\crashinfo.txt");
	strconv_t strconv;
	CString sCrashInfo = BuildCrashInfo();

	// Create the minidump file
	hFile = CreateFile(
		sStackWalkerFile,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	// Check if file has been created
	if(INVALID_HANDLE_VALUE == hFile )
	{
		DWORD dwError = GetLastError();
		OutputErrorStr(_T("Couldn't create StackWalker file: %s"), Utility::FormatErrorMsg(dwError));

		goto cleanup;
	}

	DWORD dwSize = 0;
	WriteFile(hFile, strconv.w2a(sCrashInfo.GetBuffer(0)), 
		sCrashInfo.GetLength(), &dwSize, NULL);
	ATLASSERT(dwSize > 0);

	m_Assync.SetProgress(_T("Crash info has created ."), FinishCreateCrashInfo);
cleanup:

	// Close file
	if(hFile)
		CloseHandle(hFile);

	return bStatus;
}


CString CErrorReportExporter::BuildCrashInfo()
{
#ifdef _WIN64
#define ADDRESSFORMAT   _T("0x%.16X") // Format string for 64-bit addresses
#else
#define ADDRESSFORMAT   _T("0x%.8X")  // Format string for 32-bit addresses
#endif // _WIN64

	CString sCrashInfo;
	CString str;
	str.Format(_T("\nCrash time: %s \n"), m_CrashInfo.m_sSystemTime);
	sCrashInfo += str;

	str.Format(_T("Exception address: ") ADDRESSFORMAT _T("\n"), m_CrashInfo.m_dwExceptionAddress);
	sCrashInfo += str;

	str.Format(_T("Memory usage: %s(K)\n"), m_CrashInfo.m_sMemUsage);
	sCrashInfo += str;

	str.Format(_T("Operating system: %s \n"), m_CrashInfo.m_sOSName);
	sCrashInfo += str;

	str.Format(_T("Is operating system 64-bit: %s \n"), m_CrashInfo.m_bOSIs64Bit ? _T("TRUE"): _T("FALSE"));
	sCrashInfo += str;

	str.Format(_T("call stack:\n%s \n"), m_CrashInfo.m_sCallstack);
	sCrashInfo += str;

	return sCrashInfo;
#undef ADDRESSFORMAT
}

// This method creates the minidump of the process
BOOL CErrorReportExporter::CreateMiniDump()
{   
	m_Assync.SetProgress(_T("[Creating miniDump...]")); 

	if(FALSE == m_CrashInfo.m_bGenerateMinidump)
	{
		m_Assync.SetProgress(_T("Crash dump generation disabled; skipping."));
		return FALSE;
	}

	BOOL bStatus		= FALSE;
	HMODULE hDbgHelp	= NULL;
	HANDLE hFile		= NULL;
	MINIDUMP_EXCEPTION_INFORMATION mei;
	MINIDUMP_CALLBACK_INFORMATION mci;

	CString sMinidumpFile = m_sErrorReportDirName + _T("\\crashdump.dmp");

	// Update progress
	m_Assync.SetProgress(_T("Creating crash dump file..."));
	
	// Load dbghelp.dll
	hDbgHelp = LoadLibrary(m_CrashInfo.m_sDbgHelpPath);
	if(NULL == hDbgHelp)
	{
		// Try again ... fallback to dbghelp.dll in path
		const CString sDebugHelpDLL_name = "dbghelp.dll";
		hDbgHelp = LoadLibrary(sDebugHelpDLL_name);    
	}

	if(NULL == hDbgHelp)
	{
		OutputErrorStr(_T("dbghelp.dll couldn't be loaded."));
		goto cleanup;
	}

	// Try to adjust process privilegies to be able to generate minidumps.
	SetDumpPrivileges();

	// Create the minidump file
	hFile = CreateFile(
		sMinidumpFile,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	// Check if file has been created
	if(INVALID_HANDLE_VALUE == hFile)
	{
		DWORD dwError = GetLastError();
		OutputErrorStr(_T("Couldn't create minidump file: %s"), Utility::FormatErrorMsg(dwError));

		return FALSE;
	}

	// Set valid dbghelp API version  
	typedef LPAPI_VERSION (WINAPI* LPIMAGEHLPAPIVERSIONEX)(LPAPI_VERSION AppVersion);  
	LPIMAGEHLPAPIVERSIONEX lpImagehlpApiVersionEx = 
		(LPIMAGEHLPAPIVERSIONEX)GetProcAddress(hDbgHelp, "ImagehlpApiVersionEx");
	ATLASSERT(NULL != lpImagehlpApiVersionEx);

	if(NULL != lpImagehlpApiVersionEx)
	{    
		API_VERSION CompiledApiVer;
		CompiledApiVer.MajorVersion = 6;
		CompiledApiVer.MinorVersion = 1;
		CompiledApiVer.Revision = 11;    
		CompiledApiVer.Reserved = 0;
		LPAPI_VERSION pActualApiVer = lpImagehlpApiVersionEx(&CompiledApiVer);    
		pActualApiVer;
		ATLASSERT(CompiledApiVer.MajorVersion==pActualApiVer->MajorVersion);
		ATLASSERT(CompiledApiVer.MinorVersion==pActualApiVer->MinorVersion);
		ATLASSERT(CompiledApiVer.Revision==pActualApiVer->Revision);    
	}

	// Write minidump to the file
	mei.ThreadId = m_CrashInfo.m_dwThreadId;
	mei.ExceptionPointers = m_CrashInfo.m_pExInfo;
	mei.ClientPointers = TRUE;

	mci.CallbackRoutine = MiniDumpCallback;
	mci.CallbackParam = this;

	typedef BOOL (WINAPI *LPMINIDUMPWRITEDUMP)(
		HANDLE hProcess, 
		DWORD ProcessId, 
		HANDLE hFile, 
		MINIDUMP_TYPE DumpType, 
		CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, 
		CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam, 
		CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

	// Get address of MiniDumpWirteDump function
	LPMINIDUMPWRITEDUMP pfnMiniDumpWriteDump = 
		(LPMINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
	if(!pfnMiniDumpWriteDump)
	{    
		OutputErrorStr(_T("Bad MiniDumpWriteDump function"));
		return FALSE;
	}

	// Open client process
	HANDLE hProcess = OpenProcess(
		PROCESS_ALL_ACCESS, 
		FALSE, 
		m_CrashInfo.m_dwProcessId);

	// Now actually write the minidump
	BOOL bWriteDump = pfnMiniDumpWriteDump(
		hProcess,
		m_CrashInfo.m_dwProcessId,
		hFile,
		m_CrashInfo.m_MinidumpType,
		&mei,
		NULL,
		&mci);

	// Check result
	if(!bWriteDump)
	{    
		DWORD dwError = GetLastError();
		OutputErrorStr(_T("Error writing dump: %s."), Utility::FormatErrorMsg(dwError));
		goto cleanup;
	}

	// Update progress
	bStatus = TRUE;
	m_Assync.SetProgress(_T("[Finished create miniDump]."), FinishCreateMiniDump);

cleanup:

	// Close file
	if(hFile)
		CloseHandle(hFile);

	// Unload dbghelp.dll
	if(hDbgHelp)
		FreeLibrary(hDbgHelp);
	
	return bStatus;
}

BOOL CErrorReportExporter::SetDumpPrivileges()
{
	BOOL fSuccess		= FALSE;
	HANDLE TokenHandle	= NULL;
	TOKEN_PRIVILEGES TokenPrivileges;

	m_Assync.SetProgress(_T("[Setting DumpPrivileges]"));	

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle))
	{
		OutputErrorStr(_T("SetDumpPrivileges: Could not get the process token"));
		goto Cleanup;
	}

	TokenPrivileges.PrivilegeCount = 1;

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &TokenPrivileges.Privileges[0].Luid))
	{
		OutputErrorStr(_T("SetDumpPrivileges: Couldn't lookup SeDebugPrivilege name"));
		goto Cleanup;
	}

	TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	//Add privileges here.
	if (!AdjustTokenPrivileges(TokenHandle,
		FALSE,
		&TokenPrivileges,
		sizeof(TokenPrivileges),
		NULL,
		NULL))
	{
		OutputErrorStr(_T("SetDumpPrivileges: Could not revoke the debug privilege"));
		goto Cleanup;
	}

	m_Assync.SetProgress(_T("[Finish dumpPrivileges]"), FinishSetDumpPrivileges);	
	fSuccess = TRUE;

Cleanup:

	if (TokenHandle)
	{
		CloseHandle(TokenHandle);
	}

	return fSuccess;
}

// This method restarts the client application
BOOL CErrorReportExporter::RestartApp()
{
	// Check our config - if we should restart the client app or not?
	if(FALSE == m_CrashInfo.m_bAppRestart)	return FALSE; // No need to restart

	// Reset restart flag to avoid restarting the app twice
	// (if app has been restarted already).
	m_CrashInfo.m_bAppRestart = FALSE;

	// Add a message to log and reset progress 
	m_Assync.SetProgress(_T("Restarting the application..."));

	// Set up process start up info
	STARTUPINFO si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	// Set up process information
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));  
	// Format command line
	CString sCmdLine;
	if(m_CrashInfo.m_sRestartCmdLine.IsEmpty())
	{
		// Format with double quotes to avoid first empty parameter
		sCmdLine.Format(_T("\"%s\""), m_CrashInfo.m_sImageName);
	}
	else
	{
		// Format with double quotes to avoid first empty parameters
		sCmdLine.Format(_T("\"%s\" %s"), m_CrashInfo.m_sImageName, 
			m_CrashInfo.m_sRestartCmdLine.GetBuffer(0));
	}

	// Create process using the command line prepared earlier
	BOOL bCreateProcess = CreateProcess(
		m_CrashInfo.m_sImageName, 
		sCmdLine.GetBuffer(0), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	
	// The following is to avoid a handle leak
	if(pi.hProcess)
	{
		CloseHandle(pi.hProcess);
		pi.hProcess = NULL;
	}

	// The following is to avoid a handle leak
	if(pi.hThread)
	{
		CloseHandle(pi.hThread);
		pi.hThread = NULL;
	}

	// Check if process was created
	if(!bCreateProcess)
	{    
		OutputErrorStr(_T("RestartApp: could not create process"));
		return FALSE;
	}

	// Success
	m_Assync.SetProgress(_T("Application restarted OK."));
	return TRUE;
}

// Gets status of the local operation
void CErrorReportExporter::GetCurOpStatus(int& nProgressPct, std::vector<CString>& msg_log)
{
	m_Assync.GetProgress(nProgressPct, msg_log); 
}

void CErrorReportExporter::ExportReport()
{
	// Wait for completion of crash info collector.
	WaitForCompletion();	
	DoWorkAssync(RESTART_APP);  
}


int CErrorReportExporter::TerminateAllcrashExporterProcesses()
{
	// This method looks for all runing crashExporter.exe processes
	// and terminates each one. This may be needed when an application's installer
	// wants to shutdown all crash export processes running in background
	// to replace the locked files.
	
	// Format process name.
	CString sProcessName;
#ifdef _DEBUG
	sProcessName = _T("crashExporterd.exe");
#else
	sProcessName = _T("crashExporter.exe");
#endif

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	// Create the list of all processes in the system
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		// Walk through processes
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			// Compare process name
			if (_tcsicmp(entry.szExeFile, sProcessName) == 0 && entry.th32ProcessID != GetCurrentProcessId())
			{  				
				// Open process handle
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);

				// Terminate process.
				TerminateProcess(hProcess, 1);

				CloseHandle(hProcess);
			}
		}
	}

	// Clean up
	CloseHandle(snapshot);

	return 0;
}


