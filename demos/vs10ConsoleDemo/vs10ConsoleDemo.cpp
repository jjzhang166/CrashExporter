
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <conio.h>
#include <assert.h>

#include "..\EmulateCrash.h"
#include "CrashRpt.h" // Include CrashRpt header

HANDLE hWorkingThread = NULL;
CrashThreadInfo _CrashThreadInfo;

int main(int argc, char* argv[])
{
	argc; // this is to avoid C4100 unreferenced formal parameter warning
	argv; // this is to avoid C4100 unreferenced formal parameter warning

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
		return FALSE;
	}

	printf("Choose an exception type:\n");
	printf("0 - SEH exception\n");
	printf("1 - terminate\n");
	printf("2 - unexpected\n");
	printf("3 - pure virtual method call\n");
	printf("4 - invalid parameter\n");
	printf("5 - new operator fault\n");	
	printf("6 - SIGABRT\n");
	printf("7 - SIGFPE\n");
	printf("8 - SIGILL\n");
	printf("9 - SIGINT\n");
	printf("10 - SIGSEGV\n");
	printf("11 - SIGTERM\n");
	printf("12 - RaiseException\n");
	printf("13 - throw C++ typed exception\n");
	printf("14 - Manual report\n");
	printf("Your choice >  ");

	int ExceptionType = 0;
	scanf_s("%d", &ExceptionType);
	if (ExceptionType < 0 || ExceptionType > 14)
	{
		printf("Unknown exception type specified.");       
		_getch();
	}

#if 0		//The main thread
	EmulateCrash(ExceptionType);
#else		//Worker thread
	/* Create another thread */
	_CrashThreadInfo.m_bStop = false;
	_CrashThreadInfo.m_hWakeUpEvent = CreateEvent(NULL, FALSE, FALSE, L"WakeUpEvent");
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


