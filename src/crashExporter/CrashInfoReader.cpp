
// File: CrashInfoReader.cpp
// Description: Retrieves crash information passed from CrashRpt.dll in form of XML files.

#include "stdafx.h"
#include "CrashRpt.h"
#include "CrashInfoReader.h"
#include "..\crashrpt\strconv.h"
#include "..\crashrpt\Utility.h"
#include "..\crashrpt\SharedMem.h"

using namespace Utility;

#define OutputErrorStr(ErrorStr) \
	DbgTrace(_T("%s%s\n"), _T("[CrashExporter] [CCrashInfoReader] "), ErrorStr)


CCrashInfoReader::CCrashInfoReader()
	: m_bShowMeeagebox(FALSE), m_bAppRestart(FALSE)
	, m_bGenerateMinidump(TRUE), m_bGenerateCrashWalk(TRUE), m_MinidumpType(MiniDumpNormal)
	, m_bAddScreenshot(FALSE), m_dwScreenshotFlags(0)
	, m_dwProcessId(0), m_dwThreadId(0), m_pExInfo(NULL)
	, m_nExceptionType(0), m_dwExceptionCode(0), m_uFPESubcode(0)
	, m_uInvParamLine(0), m_pCrashDesc(NULL)
	, m_dwExceptionAddress(0), m_bOSIs64Bit(FALSE)
{
}

int CCrashInfoReader::Init(LPCTSTR szFileMappingName)
{ 
	// This method unpacks crash information from a shared memory (file-mapping)
	// and inits the internal variables.

	strconv_t strconv;

	// Init shared memory
	if(!m_SharedMem.IsInitialized())
	{
		// Init shared memory
		BOOL bInitMem = m_SharedMem.Init(szFileMappingName, TRUE, 0);
		if(!bInitMem)
		{
			OutputErrorStr(_T("Error initializing shared memory."));
			return 1;
		}
	}

	// Unpack crash description from shared memory
	m_pCrashDesc = (CRASH_DESCRIPTION*)m_SharedMem.CreateView(0, sizeof(CRASH_DESCRIPTION));

	int nUnpack = UnpackCrashDescription();
	if(0 != nUnpack)
	{
		OutputErrorStr(_T("Error unpacking crash description."));
		return 2;
	}

	CollectMiscCrashInfo();

	// Done
	return 0;
}
	
// This method unpacks crash description data from shared memory.
int CCrashInfoReader::UnpackCrashDescription()
{
	if(0 != memcmp(m_pCrashDesc->m_uchMagic, "CRD", 3))
		return 1; // Invalid magic word

	// Unpack process ID, thread ID and exception pointers address.
	m_dwProcessId		= m_pCrashDesc->m_dwProcessId;
	m_dwThreadId		= m_pCrashDesc->m_dwThreadId;
	m_pExInfo			= m_pCrashDesc->m_pExceptionPtrs;  
	m_nExceptionType	= m_pCrashDesc->m_nExceptionType;
	if(m_nExceptionType == CR_SEH_EXCEPTION)
	{
		m_dwExceptionCode = m_pCrashDesc->m_dwExceptionCode;    
	}
	else if(m_nExceptionType == CR_CPP_SIGFPE)
	{
		m_uFPESubcode = m_pCrashDesc->m_uFPESubcode;
	}
	else if(m_nExceptionType == CR_CPP_INVALID_PARAMETER)
	{
		UnpackString(m_pCrashDesc->m_dwInvParamExprOffs, m_sInvParamExpr);  
		UnpackString(m_pCrashDesc->m_dwInvParamFunctionOffs, m_sInvParamFunction);  
		UnpackString(m_pCrashDesc->m_dwInvParamFileOffs, m_sInvParamFile);      
		m_uInvParamLine = m_pCrashDesc->m_uInvParamLine;
	}
	//+ Unpack StackWalker info
	UnpackString(m_pCrashDesc->m_dwStackWalkerOffs, m_sCallstack);

	// Unpack other info 
	UnpackString(m_pCrashDesc->m_dwCrashGUIDOffs, m_sCrashGUID);  
	UnpackString(m_pCrashDesc->m_dwImageNameOffs, m_sImageName);  
	// Unpack install flags
	DWORD dwInstallFlags	= m_pCrashDesc->m_dwInstallFlags;       
	m_bShowMeeagebox				= (dwInstallFlags&CR_INST_SHOW_MESSAGEBOX)!=0;    
	m_bAppRestart			= (dwInstallFlags&CR_INST_APP_RESTART)!=0;
	m_bGenerateMinidump		= (dwInstallFlags&CR_INST_NO_MINIDUMP)==0;
	m_bGenerateCrashWalk	= (dwInstallFlags&CR_INST_NO_STACKWALK)==0;
	m_MinidumpType			= m_pCrashDesc->m_MinidumpType;    
	UnpackString(m_pCrashDesc->m_dwRestartCmdLineOffs,		m_sRestartCmdLine);
	UnpackString(m_pCrashDesc->m_dwPathToDebugHelpDllOffs,	m_sDbgHelpPath);
	m_bAddScreenshot		= m_pCrashDesc->m_bAddScreenshot;
	m_dwScreenshotFlags		= m_pCrashDesc->m_dwScreenshotFlags; 

	// Success
	return 0;
}

int CCrashInfoReader::UnpackString(DWORD dwOffset, CString& str)
{
	STRING_DESC* pStrDesc = (STRING_DESC*)m_SharedMem.CreateView(dwOffset, sizeof(STRING_DESC));
	if(0 != memcmp(pStrDesc, "STR", 3))
		return 1;

	WORD wLength = pStrDesc->m_wSize;
	if(wLength <= sizeof(STRING_DESC))
		return 2;

	WORD wStrLen = wLength - sizeof(STRING_DESC);

	m_SharedMem.DestroyView((LPBYTE)pStrDesc);
	LPBYTE pStrData = m_SharedMem.CreateView(dwOffset+sizeof(STRING_DESC), wStrLen);
	str = CString((LPCTSTR)pStrData, wStrLen/sizeof(TCHAR));
	m_SharedMem.DestroyView(pStrData);

	return 0;
}

void CCrashInfoReader::CollectMiscCrashInfo()
{   
	// Get crash time
	Utility::GetSystemTime(m_sSystemTime);

	// Get operating system friendly name from registry.
	Utility::GetOSFriendlyName(m_sOSName);

	// Determine if Windows is 64-bit.
	m_bOSIs64Bit = Utility::IsOS64Bit();

	// Open parent process handle
	HANDLE hProcess = OpenProcess(
		PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, 
		FALSE, 
		m_dwProcessId);

	if(hProcess == NULL)
	{
		OutputErrorStr(_T("[CollectMiscCrashInfo] Open parent process handle failed"));
	}
	else
	{	
		SIZE_T uBytesRead = 0;
		BYTE buff[1024];
		memset(&buff, 0, 1024);
		
		// Read exception information from process memory
		if (m_pExInfo == NULL)
		{
			OutputErrorStr(_T("[CollectMiscCrashInfo] address of exception info valid"));
		}
		else
		{			
			if(ReadProcessMemory(hProcess, m_pExInfo, &buff, sizeof(EXCEPTION_POINTERS), &uBytesRead) &&
				uBytesRead == sizeof(EXCEPTION_POINTERS))
			{
				EXCEPTION_POINTERS* pExcPtrs = (EXCEPTION_POINTERS*)buff;

				if(NULL != pExcPtrs->ExceptionRecord)
				{
					DWORD64 dwExcRecordAddr = (DWORD64)pExcPtrs->ExceptionRecord;
					if(ReadProcessMemory(hProcess, (LPCVOID)dwExcRecordAddr, &buff, sizeof(EXCEPTION_RECORD), &uBytesRead) &&
						uBytesRead==sizeof(EXCEPTION_RECORD))
					{
						EXCEPTION_RECORD* pExcRec = (EXCEPTION_RECORD*)buff;

						m_dwExceptionAddress = (DWORD64)pExcRec->ExceptionAddress;
					}
				}				
			}
		}

		// Get memory usage info
		PROCESS_MEMORY_COUNTERS meminfo;
		BOOL bGetMemInfo = GetProcessMemoryInfo(hProcess, &meminfo, sizeof(PROCESS_MEMORY_COUNTERS));
		if(bGetMemInfo)
		{    
			CString sMemUsage;
#ifdef _WIN64
			sMemUsage.Format(_T("%I64u"), meminfo.WorkingSetSize/1024);
#else
			sMemUsage.Format(_T("%lu"), meminfo.WorkingSetSize/1024);
#endif 
			m_sMemUsage = sMemUsage;
		}

		// Determine the period of time the process is working.
		FILETIME CreationTime, ExitTime, KernelTime, UserTime;
		/*BOOL bGetTimes = */GetProcessTimes(hProcess, &CreationTime, &ExitTime, &KernelTime, &UserTime);
		/*ATLASSERT(bGetTimes);*/
		SYSTEMTIME AppStartTime;
		FileTimeToSystemTime(&CreationTime, &AppStartTime);

		SYSTEMTIME CurTime;
		GetSystemTime(&CurTime);
		ULONG64 uCurTime = Utility::SystemTimeToULONG64(CurTime);
		ULONG64 uStartTime = Utility::SystemTimeToULONG64(AppStartTime);

		// Check that the application works for at least one minute before crash.
		// This might help to avoid cyclic error report generation when the applciation
		// crashes on startup.
		double dDiffTime = (double)(uCurTime-uStartTime)*10E-08;
		if(dDiffTime < 60)
		{
			m_bAppRestart = FALSE; // Disable restart.
		} 
	}
}