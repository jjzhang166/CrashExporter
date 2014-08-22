
#include "EmulateCrash.h"

#include "CrashRpt.h"
#include "dbghelp.h"
#include <signal.h>
#include <eh.h>
#include <assert.h>
#include <malloc.h>
#include <stdio.h>


// Code taken from http://msdn.microsoft.com/zh-cn/library/3c594ae3.aspx
#pragma warning(disable : 4355)
class CDerived;
class CBase
{
public:
	CBase(CDerived *derived): m_pDerived(derived) {};
	~CBase();
	virtual void function(void) = 0;

	CDerived * m_pDerived;
};

class CDerived : public CBase
{
public:
	CDerived() : CBase(this) {};   // C4355 "this" used in derived c'tor
	virtual void function(void) {};
};

CBase::~CBase()
{
	m_pDerived -> function();
}


#include <float.h>
void sigfpe_test()
{ 
	// Code taken from http://www.devx.com/cplus/Article/34993/1954

	//Set the x86 floating-point control word according to what
	//exceptions you want to trap. 
	_clearfp(); //Always call _clearfp before setting the control
	//word
	//Because the second parameter in the following call is 0, it
	//only returns the floating-point control word
	unsigned int cw; 
#if _MSC_VER<1400
	cw = _controlfp(0, 0); //Get the default control
#else
	_controlfp_s(&cw, 0, 0); //Get the default control
#endif 
	//word
	//Set the exception masks off for exceptions that you want to
	//trap.  When a mask bit is set, the corresponding floating-point
	//exception is //blocked from being generating.
	cw &=~(EM_OVERFLOW|EM_UNDERFLOW|EM_ZERODIVIDE|
		EM_DENORMAL|EM_INVALID);
	//For any bit in the second parameter (mask) that is 1, the 
	//corresponding bit in the first parameter is used to update
	//the control word.  
	unsigned int cwOriginal = 0;
#if _MSC_VER<1400
	cwOriginal = _controlfp(cw, MCW_EM); //Set it.
#else
	_controlfp_s(&cwOriginal, cw, MCW_EM); //Set it.
#endif
	//MCW_EM is defined in float.h.

	// Divide by zero

	float a = 1.0f;
	float b = 0.0f;
	float c = a/b;
	c;

	//Restore the original value when done:
	//_controlfp_s(cwOriginal, MCW_EM);
}

#define BIG_NUMBER 0x1fffffff
//#define BIG_NUMBER 0xf
#pragma warning(disable: 4717) // avoid C4717 warning
#pragma warning(disable: 4702)
int RecurseAlloc() 
{
	int *pi = NULL;
	for(;;)
		pi = new int[BIG_NUMBER];
	return 0;
}

void test_generate_report()
{
	CR_EXCEPTION_INFO ei;
	memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
	ei.cb = sizeof(CR_EXCEPTION_INFO);
	ei.exctype = CR_SEH_EXCEPTION;
	ei.code = 0x1234;
	ei.pexcptrs = NULL;
	ei.bManual = TRUE; // Signal the report is being generated manually.

	int nResult = crGenerateErrorReport(&ei);
	assert(nResult==0);
	nResult;
}

#if _MSC_VER == 1200	//vc6
int EmulateCrash(unsigned ExceptionType)
#elif _MSC_VER == 1600	//vs2010
int EmulateCrash(unsigned ExceptionType) throw (...)
#endif

{
	switch(ExceptionType)
	{
	case CR_SEH_EXCEPTION:
		{
			// Access violation
			int *p = 0;
			*p = 0;
		}
		break;
	case CR_CPP_TERMINATE_CALL:
		{
			// Call terminate
			terminate();
		}
		break;
	case CR_CPP_UNEXPECTED_CALL:
		{
			// Call unexpected
			unexpected();
		}
		break;
	case CR_CPP_PURE_CALL:
		{
			// pure virtual method call
			CDerived derived;
		}
		break;

	case CR_CPP_INVALID_PARAMETER:
		{      
			char* formatString;
			// Call printf_s with invalid parameters.
			formatString = NULL;
			printf(formatString);
		}
		break;
	case CR_CPP_NEW_OPERATOR_ERROR:
		{
			// Cause memory allocation error
			RecurseAlloc();
		}
		break;
	case CR_CPP_SIGABRT: 
		{
			// Call abort
			abort();
		}
		break;
	case CR_CPP_SIGFPE:
		{
			// floating point exception ( /fp:except compiler option)
			sigfpe_test();
			return 1;
		}    
		break;
	case CR_CPP_SIGILL: 
		{
			int result = raise(SIGILL);  
			assert(result==0);
			return result;
		} 
		break;
	case CR_CPP_SIGINT: 
		{
			int result = raise(SIGINT);  
			assert(result==0);
			return result;
		}   
		break;
	case CR_CPP_SIGSEGV: 
		{
			int result = raise(SIGSEGV);  
			assert(result==0);
			return result;
		} 
		break;
	case CR_CPP_SIGTERM: 
		{
			int result = raise(SIGTERM);  
			assert(result==0);     
			return result;
		}
		break;
	case CR_NONCONTINUABLE_EXCEPTION: 
		{
			// Raise noncontinuable software exception
			RaiseException(123, EXCEPTION_NONCONTINUABLE, 0, NULL);        
		}
		break;
	case CR_THROW: 
		{
			// Throw typed C++ exception.
			throw 13;
		}
		break;
	case MANUAL_REPORT:
		{
			test_generate_report();
		}
		break;
	default:
		{
			//AfxMessageBox(_T("Unknown exception type specified."));          
		}
		break;
	}

	return 1;
}


DWORD WINAPI CrashThread(LPVOID pParam)
{
	CrashThreadInfo* pInfo = (CrashThreadInfo*)pParam;

	crInstallToCurrentThread(0);  

	for(;;)
	{
		// Wait until wake up event is signaled
		WaitForSingleObject(pInfo->m_hWakeUpEvent, INFINITE);   

		if(pInfo->m_bStop)
			break; // Exit the loop

		else if(EmulateCrash(pInfo->m_ExceptionType)!=0)
		{
			//Error creating exception situation!    
			assert(false);
		}
	}

	crUninstallFromCurrentThread();
	// Exit this thread
	return 0;
}
