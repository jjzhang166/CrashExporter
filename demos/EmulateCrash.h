#pragma once
#include <Windows.h>

#if _MSC_VER == 1200	//vc6
int EmulateCrash(unsigned ExceptionType);
#elif _MSC_VER == 1600	//vs2010
int EmulateCrash(unsigned ExceptionType) throw (...);
#endif

struct CrashThreadInfo
{  
	HANDLE m_hWakeUpEvent;
	bool m_bStop;
	int m_ExceptionType;
};

DWORD WINAPI CrashThread(LPVOID pParam);
