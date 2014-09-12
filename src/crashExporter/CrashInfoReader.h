
/*! \file  CrashInfoReader.h
*  \brief  Retrieves crash information passed from CrashRpt.dll.
*/

#pragma once
#include "stdafx.h"
#include "..\crashrpt\SharedMem.h"
#include "ScreenCap.h"
 
/*!
\class CCrashInfoReader 
\brief
	Class responsible for reading the crash info passed by the crashed application.
*/
class CCrashInfoReader
{
public:

	/* Public member variables. */

	CString     m_sCrashGUID;			//!< Crash GUID.
	CString     m_sImageName;			//!< Path to the application executable file.
	CString		m_sCallstack;			//!< Callstack string of the special thread Id. \sa m_dwThreadId
	CString     m_sDbgHelpPath;         //!< Path to dbghelp.dll.
	CString     m_sAppName;             //!< Application name.   
	BOOL        m_bShowMeeagebox;		//!< Should we show Meeagebox to notify user?
	BOOL        m_bAppRestart;          //!< Should we restart the crashed application?
	CString     m_sRestartCmdLine;      //!<  Command line for crashed app restart.
	BOOL        m_bGenerateMinidump;    //!<  Should we generate crash minidump file?
	BOOL		m_bGenerateCrashWalk;   //!<  Should we generate crash CrashWalk file?
	MINIDUMP_TYPE m_MinidumpType;       //!<  Minidump type.
	BOOL        m_bAddScreenshot;       //!<  Should we add a desktop screenshot to error report?
	DWORD       m_dwScreenshotFlags;    //!<  Screenshot taking options.
	// Below are exception information fields.
	DWORD       m_dwProcessId;          //!<  Parent process ID (used for minidump generation).
	DWORD       m_dwThreadId;           //!<  Parent thread ID (used for minidump generation).
	PEXCEPTION_POINTERS m_pExInfo;      //!<  Address of exception info (used for minidump generation).
	int         m_nExceptionType;       //!<  Exception type (what handler caught the exception).
	DWORD       m_dwExceptionCode;      //!<  SEH exception code.
	UINT        m_uFPESubcode;          //!<  FPE exception subcode.
	CString     m_sInvParamExpr;        //!<  Invalid parameter expression.
	CString     m_sInvParamFunction;    //!<  Invalid parameter function.
	CString     m_sInvParamFile;        //!<  Invalid parameter file.
	UINT        m_uInvParamLine;        //!<  Invalid parameter line.

	// Below are system info.
	CString     m_sSystemTime;			//!< The time when crash occurred.
	ULONG64     m_dwExceptionAddress;	//!< Exception address (taken from exception info structure).  
	CString     m_sMemUsage;			//!< Memory usage.
	CString     m_sOSName;				//!< Operating system friendly name.
	BOOL        m_bOSIs64Bit;			//!< Is operating system 64-bit?

	/* Member functions */

	// Constructor
	CCrashInfoReader();
	
	/*! Gets crash info from shared memory. */
	int Init(LPCTSTR szFileMappingName);
	
	/*! This method unpacks crash description data from shared memory. */
	int UnpackCrashDescription();
	
private:

	/*! Unpacks a string. */
	int UnpackString(DWORD dwOffset, CString& str);

	/*! Collects misc info about the crash. */
	void CollectMiscCrashInfo();
	CSharedMem m_SharedMem;                 //!< Shared memory
	CRASH_DESCRIPTION* m_pCrashDesc;        //!< Pointer to crash descritpion
};

