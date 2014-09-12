
#include "stdafx.h"
#include "AssyncNotification.h"
#include "..\crashrpt\strconv.h"

AssyncNotification::AssyncNotification()
{
	// Init variables
	m_hCompletionEvent	= CreateEvent(0, FALSE, FALSE, 0);
	m_hCancelEvent		= CreateEvent(0, FALSE, FALSE, 0);
	m_hFeedbackEvent	= CreateEvent(0, FALSE, FALSE, 0);
	
	Reset();
}

AssyncNotification::~AssyncNotification()
{
}

void AssyncNotification::Reset()
{ 
	// Reset the event

	m_cs.Lock(); // Acquire lock

	m_nCompletionStatus = -1;    
	m_nPercentCompleted = 0;
	m_statusLog.clear();

	ResetEvent(m_hCancelEvent);
	ResetEvent(m_hCompletionEvent);
	ResetEvent(m_hFeedbackEvent);

	m_cs.Unlock(); // Free lock
}

void AssyncNotification::SetProgress(CString sStatusMsg, int percentCompleted)
{
	m_cs.Lock(); // Acquire lock

	m_statusLog.push_back(sStatusMsg);


	m_nPercentCompleted = percentCompleted;

	m_cs.Unlock(); // Free lock
}

void AssyncNotification::GetProgress(int& nProgressPct, std::vector<CString>& msg_log)
{
	msg_log.clear(); // Init message log (clear it)

	m_cs.Lock(); // Acquire lock

	nProgressPct = m_nPercentCompleted;
	msg_log = m_statusLog;
	m_statusLog.clear();

	m_cs.Unlock(); // Free lock
}

void AssyncNotification::SetCompleted(int nCompletionStatus)
{
	// Notifies about assynchronious operation completion
	m_cs.Lock(); // Acquire lock
	m_nCompletionStatus = nCompletionStatus;
	m_cs.Unlock(); // Free lock
	SetEvent(m_hCompletionEvent); // Set event
}

int AssyncNotification::WaitForCompletion()
{
	// Blocks until assynchronous operation is completed
	WaitForSingleObject(m_hCompletionEvent, INFINITE);

	// Get completion status
	int status = -1;
	m_cs.Lock(); // Acquire lock
	status = m_nCompletionStatus;
	m_cs.Unlock(); // Free lock

	return status;
}
