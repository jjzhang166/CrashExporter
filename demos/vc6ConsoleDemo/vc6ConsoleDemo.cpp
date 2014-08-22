
#include <stdio.h>
#include <conio.h>
#include <assert.h>

#include "..\EmulateCrash.h"
#include "CrashRpt.h" // Include CrashRpt header

HANDLE hWorkingThread = NULL;
CrashThreadInfo _CrashThreadInfo;

int main()
{
	// Install crash reporting
	CR_INSTALL_INFO info;
	// memset(&info, 0, sizeof(CR_INSTALL_INFO));
	// info.cb = sizeof(CR_INSTALL_INFO);             // Size of the structure
	// info.pszRestartCmdLine = _T("/restart"); 

	//info.dwFlags |= CR_INST_SHOW_MESSAGEBOX;
	// Install crash handlers
	int nInstResult = crInstall(&info);            
	assert(nInstResult==0);

	nInstResult = crAddScreenshot(CR_AS_MAIN_WINDOW);
	assert(nInstResult==0);

	// Check result
	if(nInstResult!=0)
	{
		printf("Install crash exporter failed\n");
		_getch();
		return -1;
	}

	printf("Choose an exception type:\n");
	printf("0 - SEH exception\n");
	printf("12 - RaiseException\n");
	printf("13 - throw C++ typed exception\n");
	printf("14 - Manual report\n");
	printf("Your choice >  ");

	int ExceptionType = 0;
	scanf("%d", &ExceptionType);
	if (ExceptionType != 0 || ExceptionType != 12 || ExceptionType != 13 || ExceptionType != 14)
	{
		printf("Unknown exception type specified.");       
		_getch();
	}

#if 1		//The main thread
	EmulateCrash(ExceptionType);
#else		//Worker thread
	/* Create another thread */
	_CrashThreadInfo.m_bStop = false;
	_CrashThreadInfo.m_hWakeUpEvent = CreateEvent(NULL, FALSE, FALSE, "WakeUpEvent");
	assert(_CrashThreadInfo.m_hWakeUpEvent != NULL);

	DWORD dwThreadId = 0;
	hWorkingThread = CreateThread(NULL, 0, CrashThread, (LPVOID)&_CrashThreadInfo, 0, &dwThreadId);
	assert(hWorkingThread != NULL);

	_CrashThreadInfo.m_ExceptionType = ExceptionType;
	SetEvent(_CrashThreadInfo.m_hWakeUpEvent); // wake up the working thread
	
	WaitForSingleObject(hWorkingThread, 10000);
#endif
	
	int nUninstRes = crUninstall(); // Uninstall exception handlers
	assert(nUninstRes==0);
	nUninstRes;
	return 0;
}


