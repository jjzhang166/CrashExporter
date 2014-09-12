
/*! \file  ErrorReportExporter.h
*  \brief  Export Crash Error information, eg. minidump, stack walk, Screenshot...
*/

#pragma once
#include "AssyncNotification.h"
#include "CrashInfoReader.h"

/*!
\class CErrorReportExporter 
\brief
	The main class that collects crash report files. 
*/
class CErrorReportExporter
{
public:

	// Constructor.
	CErrorReportExporter();

	// Destructor.
	virtual ~CErrorReportExporter();

	/*! Returns singleton of this class. */
	static CErrorReportExporter* GetInstance();

	/*! Performs initialization. */	
	BOOL Init(LPCTSTR szFileMappingName);
		
	/*! Cleans up all temp files and does other finalizing work. */
	BOOL Finalize();

	/*! Returns pointer to object containing crash information. */
	CCrashInfoReader* GetCrashInfo() { return &m_CrashInfo; }

	/*! Return name of the directory where error report files are located. */
	CString GetErrorReportDir() { return m_sErrorReportDirName; }

	/*! Blocks until an assync operation finishes. */
	void WaitForCompletion();

	/*! Gets current operation status. */
	void GetCurOpStatus(int& nProgressPct, std::vector<CString>& msg_log);

	/*! Exports crash report to disc. */
	void ExportReport();
	
	// This method finds and terminates all instances of crashExporter.exe process.
	static int TerminateAllcrashExporterProcesses();
		
private:

	/*! This method performs an action or several actions. */
	BOOL DoWork(int Action);
		
	/*! Worker thread proc. */
	static DWORD WINAPI WorkerThread(LPVOID lpParam);  

	/*! Runs an action or several actions in assync mode. */
	BOOL DoWorkAssync(int Action);
		
	/*! Takes desktop screenshot. */
	BOOL TakeDesktopScreenshot();

	/*! Creates crash dump file. */
	BOOL CreateMiniDump();  

	/*! Create CrashInfo. */
	BOOL CreateCrashInfo();

	/*! Return CrashInfo string. */
	CString BuildCrashInfo();

	/*! This method is used to have the current process be able to call MiniDumpWriteDump. */
	BOOL SetDumpPrivileges();

	/*! Minidump callback. */
	static BOOL CALLBACK MiniDumpCallback(PVOID CallbackParam, PMINIDUMP_CALLBACK_INPUT CallbackInput,
		PMINIDUMP_CALLBACK_OUTPUT CallbackOutput); 

	/*! Minidump callback. */
	BOOL OnMinidumpProgress(const PMINIDUMP_CALLBACK_INPUT CallbackInput,
		PMINIDUMP_CALLBACK_OUTPUT CallbackOutput);

	/*! Restarts the application. */
	BOOL RestartApp();

	/*! Unblocks parent process. */
	void UnblockParentProcess();

	// Internal variables
	static CErrorReportExporter* m_pInstance;	//!< Singleton
	CCrashInfoReader			m_CrashInfo;	//!< Contains crash information.
	HANDLE						m_hThread;		//!< Handle to the worker thread.
	AssyncNotification			m_Assync;       //!< Used for communication with the main thread.
	int							m_Action;       //!< Current assynchronous action.
	CString						m_sErrorReportDirName; //!< Name of the directory where error report files are located.

	/*!
	\enum ActionType 
	\brief
		Action type. 
	*/
	enum ActionType  
	{
		COLLECT_CRASH_INFO  = 0x01, //!< Crash info should be collected.
		RESTART_APP         = 0x02, //!< Crashed app should be restarted.
	};
};


