
/*! \file   CrashHandler.h
*  \brief  Exception handling functionality.
*/

#pragma once
#include "stdafx.h"
#include "CrashRpt.h"      
#include "Utility.h"
#include "CritSec.h"
#include "SharedMem.h"
#include "StackWalker.h"

#include <new.h>

/*! 
\struct ThreadExceptionHandlers
\brief This structure contains pointer to the exception handlers for a thread.
*/
struct ThreadExceptionHandlers
{
    ThreadExceptionHandlers()
    {
        m_prevTerm		= NULL;
        m_prevUnexp		= NULL;
        m_prevSigFPE	= NULL;
        m_prevSigILL	= NULL;
        m_prevSigSEGV	= NULL;
    }

    terminate_handler m_prevTerm;        //!< Previous terminate handler   
    unexpected_handler m_prevUnexp;      //!< Previous unexpected handler
    void (__cdecl *m_prevSigFPE)(int);   //!< Previous FPE handler
    void (__cdecl *m_prevSigILL)(int);   //!< Previous SIGILL handler
    void (__cdecl *m_prevSigSEGV)(int);  //!< Previous illegal storage access handler
};

/*! 
\class CCrashHandler
\brief This class is used to set exception handlers, catch exceptions and launch crash report export process.
*/
class CCrashHandler  
{
public:

	void CreateOutStackWalker();

    // Default constructor.
    CCrashHandler();

	// Destructor.
    virtual ~CCrashHandler();

	/*! 
	\brief Initializes the crash handler object.  
	*/
    int Init(         
        DWORD dwFlags = 0,
        MINIDUMP_TYPE MiniDumpType = MiniDumpNormal,
        __in_opt LPCTSTR lpcszRestartCmdLine = NULL);

	/*! 
	\brief Returns TRUE if object was initialized.
	*/
	BOOL IsInitialized() { return m_bInitialized; };

	/*! 
	\brief Frees all used resources. 
	*/
    int Destroy();

	/*! 
	\brief Adds desktop screenshot of crash into error report.
	*/
    int AddScreenshot(DWORD dwFlags);

	/*! 
	\brief Generates error report
	*/
    int GenerateErrorReport(__in_opt PCR_EXCEPTION_INFO pExceptionInfo = NULL);

	/*! 
	\brief Sets exception handlers for the entire process
	*/
    int SetProcessExceptionHandlers(DWORD dwFlags);

	/*! 
	\brief Unsets exception handlers for the entire process
	*/
    int UnSetProcessExceptionHandlers();

	/*! 
	\brief Sets exception handlers for the caller thread
	*/
    int SetThreadExceptionHandlers(DWORD dwFlags);

	/*! 
	\brief Unsets exception handlers for the caller thread
	*/
    int UnSetThreadExceptionHandlers();

	/*! 
	\brief Returns flags.
	*/
	DWORD GetFlags();

	/*! 
	\brief Returns the crash handler object (singleton).
	*/
    static CCrashHandler* GetCurrentProcessCrashHandler();
	
	/*! 
	\brief Releases the singleton of this crash handler object.
	*/
    static void ReleaseCurrentProcessCrashHandler();

private:
    /* Exception handler functions. */

	// Structured exception handler (SEH handler)
    static LONG WINAPI SehHandler(__in PEXCEPTION_POINTERS pExceptionPtrs);
	static DWORD WINAPI StackOverflowThreadFunction(LPVOID threadParameter);
	// C++ terminate handler
    static void __cdecl TerminateHandler();
	// C++ unexpected handler
    static void __cdecl UnexpectedHandler();

	// C++ pure virtual call handler
    static void __cdecl PureCallHandler();

	// C++ Invalid parameter handler.
    static void __cdecl InvalidParameterHandler(const wchar_t* expression, 
		const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved);

	// C++ new operator fault (memory exhaustion) handler
    static int __cdecl NewHandler(size_t);

	// Signal handlers
    static void SigabrtHandler(int);
    static void SigfpeHandler(int /*code*/, int subcode);
    static void SigintHandler(int);
    static void SigillHandler(int);
    static void SigsegvHandler(int);
    static void SigtermHandler(int);

    /* Crash report generation methods */

	/*! 
	\brief Collects current state of CPU registers.
	*/
    void GetExceptionPointers(DWORD dwExceptionCode, EXCEPTION_POINTERS* pExceptionPointers);

	/*! 
	\brief Packs crash description into shared memory.
	*/
    CRASH_DESCRIPTION* PackCrashInfoIntoSharedMem(__in CSharedMem* pSharedMem);

	/*! 
	\brief Packs a string.
	*/
    DWORD PackString(CString str);

	/*! 
	\brief Launches the crashExporter.exe process.
	*/
    int LaunchcrashExporter(LPCTSTR szCmdLineParams, BOOL bWait, __out_opt HANDLE* phProcess); 

	/*! 
	\brief Returns TRUE if crashExporter.exe process is still alive.
	*/
	BOOL IsExportProcessAlive();

	/*! 
	\brief Sets internal pointers to exception handlers to NULL.
	*/
    void InitPrevExceptionHandlerPointers();

	/*! 
	\brief Initializes several internal fields before each crash.
	*/
	int PerCrashInit();

	/*! 
	\brief Acquires the crash lock. Other threads that may crash while we are 
		inside of a crash handler function, will wait until we unlock.
	*/
    void CrashLock(BOOL bLock);

    /* Private member variables. */

    static CCrashHandler* m_pProcessCrashHandler;	//!< Singleton of the CCrashHandler class.

	
    LPTOP_LEVEL_EXCEPTION_FILTER  m_oldSehHandler;	//!< Previous SEH exception filter.

	_purecall_handler m_prevPurec;					//!< Previous pure virtual call exception filter.
	_PNH m_prevNewHandler;							//!< Previous new operator exception filter.
	_invalid_parameter_handler m_prevInvpar;		//!< Previous invalid parameter exception filter.

    void (__cdecl *m_prevSigABRT)(int);				//!< Previous SIGABRT handler.  
    void (__cdecl *m_prevSigINT)(int);				//!< Previous SIGINT handler.
    void (__cdecl *m_prevSigTERM)(int);				//!< Previous SIGTERM handler.

   
    std::map<DWORD, ThreadExceptionHandlers> m_ThreadExceptionHandlers;
										//!< List of exception handlers installed for worker threads of this process.

    CCritSec m_csThreadExceptionHandlers;			//!< Synchronization lock for m_ThreadExceptionHandlers.

    BOOL m_bInitialized;           //!< Flag telling if this object was initialized.  
    CString m_sCrashGUID;          //!< Crash GUID.
    CString m_sImageName;          //!< Process image name.
    DWORD m_dwFlags;               //!< Flags.
    MINIDUMP_TYPE m_MinidumpType;  //!< Minidump type.
    CString m_sRestartCmdLine;     //!< App restart command line.
	BOOL m_bAddScreenshot;         //!< Should we add screenshot?
    DWORD m_dwScreenshotFlags;     //!< Screenshot flags.
	CCritSec m_csCrashLock;        //!< Critical section used to synchronize thread access to this object. 
	HANDLE m_hEvent;               //!< Event used to synchronize CrashRpt.dll with crashExporter.exe.
	HANDLE m_hEvent2;              //!< Another event used to synchronize CrashRpt.dll with crashExporter.exe.
	CSharedMem m_SharedMem;        //!< Shared memory.  
	CRASH_DESCRIPTION* m_pCrashDesc; //!< Pointer to crash description shared mem view.
	CSharedMem* m_pTmpSharedMem;   //!< Used temporarily
	CRASH_DESCRIPTION* m_pTmpCrashDesc; //!< Used temporarily
	HANDLE m_hExportProcess;       //!< Handle to crashExporter.exe process.
	BOOL m_bContinueExecution;     //!< Whether to terminate process (the default) or to continue execution after crash.
	CString m_sPathToDebugHelpDll;	//!< Path to dbghelp.dll.
	CString m_sPathTocrashExporter;	//!< Path to crashExporter.exe
	COutStackWalker m_StackWalker;	//!< Display the callstack of the thread which you are interested.	
};


