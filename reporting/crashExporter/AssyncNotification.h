
/*! \file  AssyncNotification.h
*  \brief  Provides a way to communicate between worker thread and the main thread.
*/

#pragma once
#include "stdafx.h"
#include <vector>


/*!
\enum Progress 
\brief
	Action type. 
*/
enum ExportProgress  
{
	StartCollectCrashInfo	= 10,
	FinishTakeDesktopScreenshot,
	FinishSetDumpPrivileges,
	FinishCreateMiniDump,
	FinishCreateCrashInfo,
	FinishUnblockParentProcess,
	FinishCollectCrashInfo,
	FinishRestartApp,
};

/*!
\class AssyncNotification 
\brief
	Provides a way to communicate between worker thread and the main thread.
*/
class AssyncNotification
{
public:

    /* Constructor */
    AssyncNotification(); 
	~AssyncNotification(); 

    /*! Resets the event */
    void Reset();

	/*! Sets the progress message and percent completed */
    void SetProgress(CString sStatusMsg, int percentCompleted = 0);

	/*! Returns the current assynchronous operation progress */
	void GetProgress(int& nProgressPct, std::vector<CString>& msg_log);

	/*! Notifies about assynchronous operation completion */
    void SetCompleted(int nCompletionStatus);

	/*! Blocks until assynchronous operation is completed */
    int WaitForCompletion();

private:

    CComAutoCriticalSection m_cs; //!< Protects internal state
    int m_nCompletionStatus;      //!< Completion status of the assync operation
    HANDLE m_hCompletionEvent;    //!< Completion event
    HANDLE m_hCancelEvent;        //!< Cancel event
    HANDLE m_hFeedbackEvent;      //!< Feedback event
    int m_nPercentCompleted;      //!< Percent completed
    std::vector<CString> m_statusLog; //!< Status log
};

