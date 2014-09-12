
// File: CrashHandler.cpp
// Description: Exception handling and report generation functionality.

#include "stdafx.h"
#include "CrashHandler.h"
#include "Utility.h"
#include "strconv.h"

using namespace Utility;

#define OutputErrorStr(ErrorStr) \
	DbgTrace(_T("%s%s\n"), _T("[CrashExporter] [CCrashHandler] "), ErrorStr)

#ifndef _AddressOfReturnAddress

// Taken from: http://msdn.microsoft.com/en-us/library/s975zw7k(VS.71).aspx
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

// _ReturnAddress and _AddressOfReturnAddress should be prototyped before use 
EXTERNC void * _AddressOfReturnAddress(void);
EXTERNC void * _ReturnAddress(void);

#endif 

extern HANDLE g_hModuleCrashRpt;
CCrashHandler* CCrashHandler::m_pProcessCrashHandler = NULL;

CCrashHandler::CCrashHandler()
	: m_bInitialized(FALSE), m_dwFlags(0), m_MinidumpType(MiniDumpNormal),     
	m_bAddScreenshot(FALSE), m_dwScreenshotFlags(0), m_hEvent(NULL), 
	m_hEvent2(NULL), m_pCrashDesc(NULL), m_hExportProcess(NULL), m_bContinueExecution(TRUE)
{
	// Init exception handler pointers
	InitPrevExceptionHandlerPointers();
}

CCrashHandler::~CCrashHandler()
{
	// Clean up
	Destroy();
}

// This method initializes configuration parameters, 
// creates shared memory buffer and saves the configuration parameters there,
// installs process-wide exception handlers.
int CCrashHandler::Init(DWORD dwFlags, MINIDUMP_TYPE MiniDumpType, LPCTSTR lpcszRestartCmdLine)
{ 
	// Save flags
	m_dwFlags = dwFlags;

	// Save minidump type  
	m_MinidumpType = MiniDumpType;

	// Get process image name
	m_sImageName = Utility::GetModuleName(NULL);

	// Save restart command line
	m_sRestartCmdLine = lpcszRestartCmdLine;
	// Get the name of CrashRpt DLL
	LPTSTR pszCrashRptModule = NULL;

#ifndef CRASHRPT_LIB
#ifdef _DEBUG
	pszCrashRptModule = _T("CrashRptd.dll");
#else
	pszCrashRptModule = _T("CrashRpt.dll");
#endif //_DEBUG
#else //!CRASHRPT_LIB
	pszCrashRptModule = NULL;
#endif

	// Save path to crashExporter.exe
	m_sPathTocrashExporter = Utility::GetModulePath((HMODULE)g_hModuleCrashRpt);  

	// Get crashExporter EXE name
	CString scrashExporterName;

#ifdef _DEBUG
	scrashExporterName = _T("crashExporterd.exe");
#else
	scrashExporterName = _T("crashExporter.exe");
#endif //_DEBUG

	// Check that crashExporter.exe file exists
	if(m_sPathTocrashExporter.Right(1)!='\\')
		m_sPathTocrashExporter+="\\";    

	HANDLE hFile = CreateFile(m_sPathTocrashExporter+scrashExporterName, FILE_GENERIC_READ, 
		FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);  
	if(hFile==INVALID_HANDLE_VALUE)
	{ 
		OutputErrorStr(_T("crashExporter.exe is not found in the specified path."));
		return 1;
	}
	else
	{
		CloseHandle(hFile);   
	}

	m_sPathTocrashExporter += scrashExporterName;

	// If path to dbghelp.dll not provided, use the default one
	m_sPathToDebugHelpDll = Utility::GetModulePath((HMODULE)g_hModuleCrashRpt); 

	CString sDebugHelpDLL_name = "dbghelp.dll";  

	if(m_sPathToDebugHelpDll.Right(1)!='\\')
		m_sPathToDebugHelpDll+="\\";

	// Load dbghelp.dll library
	HANDLE hDbgHelpDll = LoadLibrary(m_sPathToDebugHelpDll+sDebugHelpDLL_name);    
	// and check result
	if(!hDbgHelpDll)
	{
		//try again ... fallback to dbghelp.dll in path
		m_sPathToDebugHelpDll = _T("");
		hDbgHelpDll = LoadLibrary(sDebugHelpDLL_name);
		if(!hDbgHelpDll)
		{     
			OutputErrorStr(_T("couldn't load dbghelp.dll."));
			return 1;
		}    
	}

	m_sPathToDebugHelpDll += sDebugHelpDLL_name;

	if(hDbgHelpDll!=NULL)
	{
		FreeLibrary((HMODULE)hDbgHelpDll);
		hDbgHelpDll = NULL;
	}

	// Init some fields that should be reinitialized before each new crash.
	if(0!=PerCrashInit())
		return 1;

	// Associate this handler object with the caller process.
	m_pProcessCrashHandler =  this;

	// Set exception handlers with initial values (NULLs)
	InitPrevExceptionHandlerPointers();

	// Set exception handlers that work on per-process basis
	int nSetProcessHandlers = SetProcessExceptionHandlers(dwFlags);   
	if(nSetProcessHandlers!=0)
	{
		ATLASSERT(nSetProcessHandlers==0);
		OutputErrorStr(_T("couldn't set C++ exception handlers for current process."));
		return 1;
	}

	// Set exception handlers that work on per-thread basis
	int nSetThreadHandlers = SetThreadExceptionHandlers(dwFlags);
	if(nSetThreadHandlers!=0)
	{
		ATLASSERT(nSetThreadHandlers==0);
		OutputErrorStr(_T("couldn't set C++ exception handlers for main execution thread."));
		return 1;
	}    

	// The following code is intended to fix the issue with 32-bit applications in 64-bit environment.
	// http://support.microsoft.com/kb/976038/en-us
	// http://code.google.com/p/crashrpt/issues/detail?id=104

	typedef BOOL (WINAPI * SETPROCESSUSERMODEEXCEPTIONPOLICY)(DWORD dwFlags);
	typedef BOOL (WINAPI * GETPROCESSUSERMODEEXCEPTIONPOLICY)(LPDWORD lpFlags);
#define PROCESS_CALLBACK_FILTER_ENABLED     0x1

	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	if(hKernel32!=NULL)
	{
		SETPROCESSUSERMODEEXCEPTIONPOLICY pfnSetProcessUserModeExceptionPolicy = 
			(SETPROCESSUSERMODEEXCEPTIONPOLICY)GetProcAddress(hKernel32, "SetProcessUserModeExceptionPolicy");
		GETPROCESSUSERMODEEXCEPTIONPOLICY pfnGetProcessUserModeExceptionPolicy = 
			(GETPROCESSUSERMODEEXCEPTIONPOLICY)GetProcAddress(hKernel32, "GetProcessUserModeExceptionPolicy");

		if(pfnSetProcessUserModeExceptionPolicy!=NULL && 
			pfnGetProcessUserModeExceptionPolicy!=NULL)
		{
			DWORD dwFlags = 0;
			if(pfnGetProcessUserModeExceptionPolicy(&dwFlags))
			{
				pfnSetProcessUserModeExceptionPolicy(dwFlags & ~PROCESS_CALLBACK_FILTER_ENABLED); 
			}
		}

		FreeLibrary(hKernel32);
	}

	// Initialization OK.
	m_bInitialized = TRUE;
	return 0;
}

// Packs config info to shared mem.
CRASH_DESCRIPTION* CCrashHandler::PackCrashInfoIntoSharedMem(CSharedMem* pSharedMem)
{
	m_pTmpSharedMem = pSharedMem;

	CString sSharedMemName;
	sSharedMemName = m_sCrashGUID;

	if(!pSharedMem->IsInitialized())
	{
		// Initialize shared memory.
		BOOL bSharedMem = pSharedMem->Init(sSharedMemName, FALSE, SHARED_MEM_MAX_SIZE);
		if(!bSharedMem)
		{
			ATLASSERT(0);
			OutputErrorStr(_T("couldn't initialize shared memory."));
			return NULL; 
		}
	}

	// Create memory view.
	m_pTmpCrashDesc = 
		(CRASH_DESCRIPTION*)pSharedMem->CreateView(0, sizeof(CRASH_DESCRIPTION));  
	if(m_pTmpCrashDesc==NULL)
	{
		ATLASSERT(0);
		OutputErrorStr(_T("couldn't create shared memory view."));
		return NULL; 
	}

	// Pack config information to shared memory
	memset(m_pTmpCrashDesc, 0, sizeof(CRASH_DESCRIPTION));
	memcpy(m_pTmpCrashDesc->m_uchMagic, "CRD", 3);  
	m_pTmpCrashDesc->m_wSize = sizeof(CRASH_DESCRIPTION);
	m_pTmpCrashDesc->m_dwTotalSize = sizeof(CRASH_DESCRIPTION);  
	m_pTmpCrashDesc->m_dwInstallFlags = m_dwFlags;
	m_pTmpCrashDesc->m_MinidumpType = m_MinidumpType;
	m_pTmpCrashDesc->m_bAddScreenshot = m_bAddScreenshot;
	m_pTmpCrashDesc->m_dwScreenshotFlags = m_dwScreenshotFlags;      
	m_pTmpCrashDesc->m_dwProcessId = GetCurrentProcessId();
	m_pTmpCrashDesc->m_dwCrashGUIDOffs = PackString(m_sCrashGUID);
	m_pTmpCrashDesc->m_dwImageNameOffs = PackString(m_sImageName);
	m_pTmpCrashDesc->m_dwPathToDebugHelpDllOffs = PackString(m_sPathToDebugHelpDll);
	m_pTmpCrashDesc->m_dwRestartCmdLineOffs = PackString(m_sRestartCmdLine);

	return m_pTmpCrashDesc;
}

// Packs a string to shared memory
DWORD CCrashHandler::PackString(CString str)
{
	DWORD dwTotalSize = m_pTmpCrashDesc->m_dwTotalSize;
	int nStrLen = str.GetLength()*sizeof(TCHAR);
	WORD wLength = (WORD)(sizeof(STRING_DESC)+nStrLen);

	LPBYTE pView = m_pTmpSharedMem->CreateView(dwTotalSize, wLength);  
	STRING_DESC* pStrDesc = (STRING_DESC*)pView;
	memcpy(pStrDesc->m_uchMagic, "STR", 3);
	pStrDesc->m_wSize = wLength;
	memcpy(pView+sizeof(STRING_DESC), str.GetBuffer(0), nStrLen); 

	m_pTmpCrashDesc->m_dwTotalSize += wLength;

	m_pTmpSharedMem->DestroyView(pView);
	return dwTotalSize;
}

// Destroys the object
int CCrashHandler::Destroy()
{
	if(!m_bInitialized)
	{
		OutputErrorStr(_T("can't destroy not initialized crash handler."));
		return 1;
	}  

	// Free handle to crashExporter.exe process.
	if(m_hExportProcess!=NULL)
		CloseHandle(m_hExportProcess);

	// Free events
	if(m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}

	if(m_hEvent2)
	{
		CloseHandle(m_hEvent2);
		m_hEvent2 = NULL;
	}

	// Reset SEH exception filter
	if (m_oldSehHandler)
		SetUnhandledExceptionFilter(m_oldSehHandler);

	m_oldSehHandler = NULL;

	// All installed per-thread C++ exception handlers should be uninstalled 
	// using crUninstallFromCurrentThread() before calling Destroy()

	{
		CAutoLock lock(&m_csThreadExceptionHandlers);
		ATLASSERT(m_ThreadExceptionHandlers.size()==0);          
	}

	// 
	m_pProcessCrashHandler = NULL;

	// OK.
	m_bInitialized = FALSE;
	return 0;
}

// Sets internal pointers to previously used exception handlers to NULL
void CCrashHandler::InitPrevExceptionHandlerPointers()
{
	m_oldSehHandler = NULL;

	m_prevPurec = NULL;
	m_prevNewHandler = NULL;
	m_prevInvpar = NULL;

	m_prevSigABRT = NULL;  
	m_prevSigINT = NULL;  
	m_prevSigTERM = NULL;
}

// Returns singleton of the crash handler
CCrashHandler* CCrashHandler::GetCurrentProcessCrashHandler()
{   
	return m_pProcessCrashHandler;
}

// Releases the crash handler pointer
void CCrashHandler::ReleaseCurrentProcessCrashHandler()
{
	if(m_pProcessCrashHandler!=NULL)
	{
		delete m_pProcessCrashHandler;
		m_pProcessCrashHandler = NULL;
	}
}

// Sets exception handlers that work on per-process basis
int CCrashHandler::SetProcessExceptionHandlers(DWORD dwFlags)
{
	// If 0 is specified as dwFlags, assume all handlers should be
	// installed
	if((dwFlags&CR_INST_ALL_POSSIBLE_HANDLERS)==0)
		dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS;

	if(dwFlags&CR_INST_STRUCTURED_EXCEPTION_HANDLER)
	{
		// Install top-level SEH handler
		m_oldSehHandler = SetUnhandledExceptionFilter(SehHandler);    
	}

	_set_error_mode(_OUT_TO_STDERR);

	if(dwFlags&CR_INST_PURE_CALL_HANDLER)
	{
		// Catch pure virtual function calls.
		// Because there is one _purecall_handler for the whole process, 
		// calling this function immediately impacts all threads. The last 
		// caller on any thread sets the handler. 
		// http://msdn.microsoft.com/en-us/library/t296ys27.aspx
		m_prevPurec = _set_purecall_handler(PureCallHandler);    
	}

	if(dwFlags&CR_INST_NEW_OPERATOR_ERROR_HANDLER)
	{
		// Catch new operator memory allocation exceptions
		_set_new_mode(1); // Force malloc() to call new handler too
		m_prevNewHandler = _set_new_handler(NewHandler);
	}

	if(dwFlags&CR_INST_INVALID_PARAMETER_HANDLER)
	{
		// Catch invalid parameter exceptions.
		m_prevInvpar = _set_invalid_parameter_handler(InvalidParameterHandler); 
	}


	// Set up C++ signal handlers


	if(dwFlags&CR_INST_SIGABRT_HANDLER)
	{

		_set_abort_behavior(_CALL_REPORTFAULT, _CALL_REPORTFAULT);
		// Catch an abnormal program termination
		m_prevSigABRT = signal(SIGABRT, SigabrtHandler);  
	}

	if(dwFlags&CR_INST_SIGINT_HANDLER)
	{
		// Catch illegal instruction handler
		m_prevSigINT = signal(SIGINT, SigintHandler);     
	}

	if(dwFlags&CR_INST_TERMINATE_HANDLER)
	{
		// Catch a termination request
		m_prevSigTERM = signal(SIGTERM, SigtermHandler);          
	}
	return 0;
}

// Unsets exception pointers that work on per-process basis
int CCrashHandler::UnSetProcessExceptionHandlers()
{
	// Unset all previously set handlers

	if(m_prevPurec!=NULL)
		_set_purecall_handler(m_prevPurec);

	if(m_prevNewHandler!=NULL)
		_set_new_handler(m_prevNewHandler);

	if(m_prevInvpar!=NULL)
		_set_invalid_parameter_handler(m_prevInvpar); 

	if(m_prevSigABRT!=NULL)
		signal(SIGABRT, m_prevSigABRT);  

	if(m_prevSigINT!=NULL)
		signal(SIGINT, m_prevSigINT);     

	if(m_prevSigTERM!=NULL)
		signal(SIGTERM, m_prevSigTERM);    

	return 0;
}

// Installs C++ exception handlers that function on per-thread basis
int CCrashHandler::SetThreadExceptionHandlers(DWORD dwFlags)
{
	// If 0 is specified as dwFlags, assume all available exception handlers should be
	// installed  
	if((dwFlags&CR_INST_ALL_POSSIBLE_HANDLERS)==0)
		dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS;

	// Get current thread ID.
	DWORD dwThreadId = GetCurrentThreadId();

	// Lock the critical section.
	CAutoLock lock(&m_csThreadExceptionHandlers);

	// Try and find our thread ID in the list of threads.
	std::map<DWORD, ThreadExceptionHandlers>::iterator it = 
		m_ThreadExceptionHandlers.find(dwThreadId);

	if(it!=m_ThreadExceptionHandlers.end())
	{
		// handlers are already set for the thread    
		OutputErrorStr(_T("can't install handlers for current thread twice."));
		return 1; // failed
	}

	ThreadExceptionHandlers handlers;

	if(dwFlags&CR_INST_TERMINATE_HANDLER)
	{
		// Catch terminate() calls. 
		// In a multithreaded environment, terminate functions are maintained 
		// separately for each thread. Each new thread needs to install its own 
		// terminate function. Thus, each thread is in charge of its own termination handling.
		// http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx
		handlers.m_prevTerm = set_terminate(TerminateHandler);       
	}

	if(dwFlags&CR_INST_UNEXPECTED_HANDLER)
	{
		// Catch unexpected() calls.
		// In a multithreaded environment, unexpected functions are maintained 
		// separately for each thread. Each new thread needs to install its own 
		// unexpected function. Thus, each thread is in charge of its own unexpected handling.
		// http://msdn.microsoft.com/en-us/library/h46t5b69.aspx  
		handlers.m_prevUnexp = set_unexpected(UnexpectedHandler);    
	}

	if(dwFlags&CR_INST_SIGFPE_HANDLER)
	{
		// Catch a floating point error
		typedef void (*sigh)(int);
		handlers.m_prevSigFPE = signal(SIGFPE, (sigh)SigfpeHandler);     
	}


	if(dwFlags&CR_INST_SIGILL_HANDLER)
	{
		// Catch an illegal instruction
		handlers.m_prevSigILL = signal(SIGILL, SigillHandler);     
	}

	if(dwFlags&CR_INST_SIGSEGV_HANDLER)
	{
		// Catch illegal storage access errors
		handlers.m_prevSigSEGV = signal(SIGSEGV, SigsegvHandler);   
	}

	// Insert the structure to the list of handlers  
	m_ThreadExceptionHandlers[dwThreadId] = handlers;

	// OK.
	return 0;
}

// Unsets exception handlers for the current thread
int CCrashHandler::UnSetThreadExceptionHandlers()
{
	DWORD dwThreadId = GetCurrentThreadId();

	CAutoLock lock(&m_csThreadExceptionHandlers);

	std::map<DWORD, ThreadExceptionHandlers>::iterator it = 
		m_ThreadExceptionHandlers.find(dwThreadId);

	if(it==m_ThreadExceptionHandlers.end())
	{
		// No exception handlers were installed for the caller thread?    
		OutputErrorStr(_T("crash handler wasn't previously installed for current thread."));
		return 1;
	}

	ThreadExceptionHandlers* handlers = &(it->second);

	if(handlers->m_prevTerm!=NULL)
		set_terminate(handlers->m_prevTerm);

	if(handlers->m_prevUnexp!=NULL)
		set_unexpected(handlers->m_prevUnexp);

	if(handlers->m_prevSigFPE!=NULL)
		signal(SIGFPE, handlers->m_prevSigFPE);     

	if(handlers->m_prevSigILL!=NULL)
		signal(SIGINT, handlers->m_prevSigILL);     

	if(handlers->m_prevSigSEGV!=NULL)
		signal(SIGSEGV, handlers->m_prevSigSEGV); 

	// Remove from the list
	m_ThreadExceptionHandlers.erase(it);

	// OK.
	return 0;
}

// Adds a screen shot to the error report
int CCrashHandler::AddScreenshot(DWORD dwFlags)
{ 
	m_bAddScreenshot	= TRUE;
	m_dwScreenshotFlags = dwFlags;

	// Pack this info into shared memory
	m_pCrashDesc->m_bAddScreenshot		= TRUE;
	m_pCrashDesc->m_dwScreenshotFlags	= dwFlags;

	return 0;
}

// Generates error report
int CCrashHandler::GenerateErrorReport(PCR_EXCEPTION_INFO pExceptionInfo)
{  
	// Allocate memory in stack for storing exception pointers.
	EXCEPTION_RECORD	ExceptionRecord;
	CONTEXT				ContextRecord;    
	EXCEPTION_POINTERS	ExceptionPointers;

	ExceptionPointers.ExceptionRecord	= &ExceptionRecord;
	ExceptionPointers.ContextRecord		= &ContextRecord;  

	// Validate input parameters 
	if(pExceptionInfo==NULL)
	{
		OutputErrorStr(_T("Exception info is NULL."));

		return 1;
	}

	// Get exception pointers if they were not provided by the caller. 
	if(pExceptionInfo->pexcptrs==NULL)
	{
		GetExceptionPointers(pExceptionInfo->code, &ExceptionPointers);
		pExceptionInfo->pexcptrs = &ExceptionPointers;
	}

	// If error report is being generated manually, 
	// temporarily disable app restart feature.
	if(pExceptionInfo->bManual)
	{		
		// Force disable app restart.
		m_pCrashDesc->m_dwInstallFlags &= ~CR_INST_APP_RESTART;
	}

	//recode Callstack
	m_StackWalker.ShowCallstack(pExceptionInfo->pexcptrs->ContextRecord, GetCurrentThread());

	m_pCrashDesc->m_dwStackWalkerOffs = PackString(m_StackWalker.GetStackWalkerInfo());

	// Save current process ID, thread ID and exception pointers address to shared mem.
	m_pCrashDesc->m_dwProcessId = GetCurrentProcessId();
	m_pCrashDesc->m_dwThreadId = GetCurrentThreadId();
	m_pCrashDesc->m_pExceptionPtrs = pExceptionInfo->pexcptrs;  
	m_pCrashDesc->m_nExceptionType = pExceptionInfo->exctype;

	if(pExceptionInfo->exctype==CR_SEH_EXCEPTION)
	{
		// Set SEH exception code
		m_pCrashDesc->m_dwExceptionCode = pExceptionInfo->code;
	}
	else if(pExceptionInfo->exctype==CR_CPP_SIGFPE)
	{
		// Set FPE (floating point exception) subcode
		m_pCrashDesc->m_uFPESubcode = pExceptionInfo->fpe_subcode;
	}
	else if(pExceptionInfo->exctype==CR_CPP_INVALID_PARAMETER)
	{
		// Set invalid parameter exception info fields
		m_pCrashDesc->m_dwInvParamExprOffs = PackString(pExceptionInfo->expression);
		m_pCrashDesc->m_dwInvParamFunctionOffs = PackString(pExceptionInfo->function);
		m_pCrashDesc->m_dwInvParamFileOffs = PackString(pExceptionInfo->file);
		m_pCrashDesc->m_uInvParamLine = pExceptionInfo->line;
	}

	// Start the crashExporter.exe process which will take the dekstop screenshot, 
	// create minidump, create stackwalk file and export the error report. 

	int result = 0; // result of launching crashExporter.exe

	// If we are not recording video or video recording process has been terminated by some reason...
	if(!IsExportProcessAlive()) 
	{
		// Run new crashExporter.exe process
		result = LaunchcrashExporter(m_sCrashGUID, TRUE, &pExceptionInfo->hExportProcess);
	}

	// Check if the client program requests to continue its execution
	// after crash.
	if(m_bContinueExecution)
	{
		// Prepare for the next crash
		PerCrashInit();
	}

	// Check the result of launching the crash export process.
	if(result!=0)
	{
		ATLASSERT(result==0);
		OutputErrorStr(_T("Error launching crashExporter.exe."));

		// Failed to launch crash export process.
		// Try notifying user about crash using message box.
		CString szCaption;
		szCaption.Format(_T("%s has stopped working"), Utility::getAppName());
		CString szMessage;
		szMessage.Format(_T("Error launching crashExporter.exe"));
		MessageBox(NULL, szMessage, szCaption, MB_OK|MB_ICONERROR);    
		return 3;
	}

	// OK
	return 0; 
}

BOOL CCrashHandler::IsExportProcessAlive()
{
	// If process handle is still accessible, check its exit code
	DWORD dwExitCode = 1;
	BOOL bRes = GetExitCodeProcess(m_hExportProcess, &dwExitCode);
	if(!bRes || (bRes && dwExitCode!=STILL_ACTIVE))
	{
		return FALSE; // Process seems to exit!
	}

	return TRUE;
}


// The following code gets exception pointers using a workaround found in CRT code.
void CCrashHandler::GetExceptionPointers(DWORD dwExceptionCode, 
	EXCEPTION_POINTERS* pExceptionPointers)
{
	// The following code was taken from VC++ 8.0 CRT (invarg.c: line 104)

	CONTEXT ContextRecord;
	memset(&ContextRecord, 0, sizeof(CONTEXT));

#ifdef _X86_

	__asm {
		mov dword ptr [ContextRecord.Eax], eax
			mov dword ptr [ContextRecord.Ecx], ecx
			mov dword ptr [ContextRecord.Edx], edx
			mov dword ptr [ContextRecord.Ebx], ebx
			mov dword ptr [ContextRecord.Esi], esi
			mov dword ptr [ContextRecord.Edi], edi
			mov word ptr [ContextRecord.SegSs], ss
			mov word ptr [ContextRecord.SegCs], cs
			mov word ptr [ContextRecord.SegDs], ds
			mov word ptr [ContextRecord.SegEs], es
			mov word ptr [ContextRecord.SegFs], fs
			mov word ptr [ContextRecord.SegGs], gs
			pushfd
			pop [ContextRecord.EFlags]
	}

	ContextRecord.ContextFlags = CONTEXT_CONTROL;
#pragma warning(push)
#pragma warning(disable:4311)
	ContextRecord.Eip = (ULONG)_ReturnAddress();
	ContextRecord.Esp = (ULONG)_AddressOfReturnAddress();
#pragma warning(pop)
	ContextRecord.Ebp = *((ULONG *)_AddressOfReturnAddress()-1);

#elif defined (_IA64_) || defined (_AMD64_)

	/* Need to fill up the Context in IA64 and AMD64. */
	RtlCaptureContext(&ContextRecord);

#else  /* defined (_IA64_) || defined (_AMD64_) */

	ZeroMemory(&ContextRecord, sizeof(ContextRecord));

#endif  /* defined (_IA64_) || defined (_AMD64_) */

	memcpy(pExceptionPointers->ContextRecord, &ContextRecord, sizeof(CONTEXT));

	ZeroMemory(pExceptionPointers->ExceptionRecord, sizeof(EXCEPTION_RECORD));

	pExceptionPointers->ExceptionRecord->ExceptionCode = dwExceptionCode;
	pExceptionPointers->ExceptionRecord->ExceptionAddress = _ReturnAddress();    
}

// Launches crashExporter.exe process
int CCrashHandler::LaunchcrashExporter(LPCTSTR szCmdLineParams, BOOL bWait, HANDLE* phProcess)
{
	/* Create crashExporter.exe process */

	STARTUPINFO si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));    

	// Format command line
	TCHAR szCmdLine[_MAX_PATH]=_T("");
	_tcscat_s(szCmdLine, _MAX_PATH, _T("\""));
	_tcscat_s(szCmdLine, _MAX_PATH, m_sPathTocrashExporter.GetBuffer(0));
	_tcscat_s(szCmdLine, _MAX_PATH, _T("\" \""));    
	_tcscat_s(szCmdLine, _MAX_PATH, szCmdLineParams);
	_tcscat_s(szCmdLine, _MAX_PATH, _T("\""));    

	BOOL bCreateProcess = CreateProcess(
		m_sPathTocrashExporter, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if(pi.hThread)
	{
		CloseHandle(pi.hThread);
		pi.hThread = NULL;
	}
	if(!bCreateProcess)
	{
		ATLASSERT(bCreateProcess);
		OutputErrorStr(_T("Error creating crashExporter process."));
		return 1;
	}

	if(bWait)
	{
		/* Wait until crashExporter finishes with making screenshot, 
		copying files, creating minidump. */  

		WaitForSingleObject(m_hEvent, INFINITE);  
	}

	// Return handle to the crashExporter.exe process.    
	if(phProcess!=NULL)
	{
		*phProcess = pi.hProcess;
	}
	else
	{
		// Handle not required by caller so close it.
		CloseHandle( pi.hProcess );
		pi.hProcess = NULL;
	}

	// Done
	return 0;
}

// Acquires the crash lock. Other threads that may crash while we are 
// inside of a crash handler function, will wait until we unlock.
void CCrashHandler::CrashLock(BOOL bLock)
{
	if(bLock)
		m_csCrashLock.Lock();
	else
		m_csCrashLock.Unlock();
}

int CCrashHandler::PerCrashInit()
{
	// This method performs per-crash initialization actions.
	// For example, we have to generate a new GUID string and repack 
	// configuration info into shared memory each time.

	// Consider the next crash as non-critical.
	m_bContinueExecution = TRUE;

	// Generate new GUID for new crash report 
	Utility::GenerateGUID(m_sCrashGUID);

	// Recreate the event that will be used to synchronize with crashExporter.exe process.
	if(NULL != m_hEvent)
		CloseHandle(m_hEvent); // Free old event
	CString sEventName;
	sEventName.Format(_T("Local\\CrashRptEvent_%s"), m_sCrashGUID);
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, sEventName);

	// Reset shared memory
	if(m_SharedMem.IsInitialized())
	{
		m_SharedMem.Destroy();
		m_pCrashDesc = NULL;
	}

	// Pack configuration info into shared memory.
	// It will be passed to crashExporter.exe later.
	m_pCrashDesc = PackCrashInfoIntoSharedMem(&m_SharedMem);

	// OK
	return 0;
}

// Structured exception handler (SEH)
LONG WINAPI CCrashHandler::SehHandler(PEXCEPTION_POINTERS pExceptionPtrs)
{ 
	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);  

	if (pExceptionPtrs != 0 && pExceptionPtrs->ExceptionRecord != 0 &&
		pExceptionPtrs->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) 
	{
		// Special case to handle the stack overflow exception.
		// The dump will be realized from another thread.
		// Create another thread that will do the dump.
		HANDLE thread = CreateThread(0, 0, &StackOverflowThreadFunction, pExceptionPtrs, 0, 0);
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		// Terminate process
		TerminateProcess(GetCurrentProcess(), 1);
	}

	if(pCrashHandler!=NULL)
	{
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. 
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Generate error report.
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_SEH_EXCEPTION;
		ei.pexcptrs = pExceptionPtrs;
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);
	}   

	// Unreacheable code  
	return EXCEPTION_EXECUTE_HANDLER;
}

//Vojtech: Based on martin.bis...@gmail.com comment in
// http://groups.google.com/group/crashrpt/browse_thread/thread/a1dbcc56acb58b27/fbd0151dd8e26daf?lnk=gst&q=stack+overflow#fbd0151dd8e26daf
// Thread procedure doing the dump for stack overflow.
DWORD WINAPI CCrashHandler::StackOverflowThreadFunction(LPVOID lpParameter)
{
	PEXCEPTION_POINTERS pExceptionPtrs =
		reinterpret_cast<PEXCEPTION_POINTERS>(lpParameter);

	CCrashHandler *pCrashHandler =
		CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler != NULL);

	if (pCrashHandler != NULL) 
	{
		// Acquire lock to avoid other threads (if exist) to crash while we	are inside.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Generate error report.
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_SEH_EXCEPTION;
		ei.pexcptrs = pExceptionPtrs;
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);
	}

	return 0;
}

// CRT terminate() call handler
void __cdecl CCrashHandler::TerminateHandler()
{
	// Abnormal program termination (terminate() function was called)

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);

	if(pCrashHandler!=NULL)
	{
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in the exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_CPP_TERMINATE_CALL;

		// Generate crash report
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);   
	}    
}

// CRT unexpected() call handler
void __cdecl CCrashHandler::UnexpectedHandler()
{
	// Unexpected error (unexpected() function was called)

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);

	if(pCrashHandler!=NULL)
	{
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in the exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_CPP_UNEXPECTED_CALL;

		// Generate crash report
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);   
	}    
}

// CRT Pure virtual method call handler
void __cdecl CCrashHandler::PureCallHandler()
{
	// Pure virtual function call

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);

	if(pCrashHandler!=NULL)
	{
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in the exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_CPP_PURE_CALL;

		// Generate error report.
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);   
	}  
}

// CRT invalid parameter handler
void __cdecl CCrashHandler::InvalidParameterHandler(
	const wchar_t* expression, 
	const wchar_t* function, 
	const wchar_t* file, 
	unsigned int line, 
	uintptr_t pReserved)
{
	pReserved;

	// Invalid parameter exception

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);

	if(pCrashHandler!=NULL)
	{
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in the exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_CPP_INVALID_PARAMETER;
		ei.expression = expression;
		ei.function = function;
		ei.file = file;
		ei.line = line;    

		// Generate error report.
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);   
	}   
}

// CRT new operator fault handler
int __cdecl CCrashHandler::NewHandler(size_t)
{
	// 'new' operator memory allocation exception

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);

	if(pCrashHandler!=NULL)
	{     
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in the exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_CPP_NEW_OPERATOR_ERROR;
		ei.pexcptrs = NULL;    

		// Generate error report.
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);
	}

	// Unreacheable code
	return 0;
}

// CRT SIGABRT signal handler
void CCrashHandler::SigabrtHandler(int)
{
	// Caught SIGABRT C++ signal

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);

	if(pCrashHandler!=NULL)
	{     
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in the exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_CPP_SIGABRT;    

		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE); 
	}
}

// CRT SIGFPE signal handler
void CCrashHandler::SigfpeHandler(int /*code*/, int subcode)
{
	// Floating point exception (SIGFPE)

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);

	if(pCrashHandler!=NULL)
	{     
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in the exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_CPP_SIGFPE;
		ei.pexcptrs = (PEXCEPTION_POINTERS)_pxcptinfoptrs;
		ei.fpe_subcode = subcode;

		//Generate crash report.
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);
	}
}

// CRT sigill signal handler
void CCrashHandler::SigillHandler(int)
{
	// Illegal instruction (SIGILL)

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);

	if(pCrashHandler!=NULL)
	{    
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in the exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_CPP_SIGILL;

		// Generate crash report
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);    
	}
}

// CRT sigint signal handler
void CCrashHandler::SigintHandler(int)
{
	// Interruption (SIGINT)

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);

	if(pCrashHandler!=NULL)
	{ 
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in the exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_CPP_SIGINT;

		// Generate crash report.
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);   
	}
}

// CRT SIGSEGV signal handler
void CCrashHandler::SigsegvHandler(int)
{
	// Invalid storage access (SIGSEGV)

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();

	if(pCrashHandler!=NULL)
	{     
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);    
		ei.exctype = CR_CPP_SIGSEGV;
		ei.pexcptrs = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

		// Generate crash report
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}
		// Free lock
		pCrashHandler->CrashLock(FALSE);   
	}
}
// CRT SIGTERM signal handler
void CCrashHandler::SigtermHandler(int)
{
	// Termination request (SIGTERM)

	CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
	ATLASSERT(pCrashHandler!=NULL);

	if(pCrashHandler!=NULL)
	{    
		// Acquire lock to avoid other threads (if exist) to crash while we are 
		// inside. We do not unlock, because process is to be terminated.
		pCrashHandler->CrashLock(TRUE);

		// Treat this type of crash critical by default
		pCrashHandler->m_bContinueExecution = FALSE;

		// Fill in the exception info
		CR_EXCEPTION_INFO ei;
		memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
		ei.cb = sizeof(CR_EXCEPTION_INFO);
		ei.exctype = CR_CPP_SIGTERM;

		// Generate crash report
		pCrashHandler->GenerateErrorReport(&ei);

		if(!pCrashHandler->m_bContinueExecution)
		{
			// Terminate process
			TerminateProcess(GetCurrentProcess(), 1);
		}

		// Free lock
		pCrashHandler->CrashLock(FALSE);  
	}
}

DWORD CCrashHandler::GetFlags()
{
	return m_dwFlags;
}

