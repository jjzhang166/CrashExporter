
// File: CrashRpt.cpp
// Description: CrashRpt API implementation.


#include "stdafx.h"
#include "CrashRpt.h"
#include "CrashHandler.h"
#include "Utility.h"
#include "strconv.h"

using namespace Utility;

HANDLE g_hModuleCrashRpt = NULL; // Handle to CrashRpt.dll module.

#define OutputErrorStr(ErrorStr) \
	DbgTrace(_T("%s%s\n"), _T("[CrashExporter] [CRASHRPTAPI] "), ErrorStr)


int CRASHRPTAPI crInstallW(CR_INSTALL_INFOW* pInfo)
{
    int nStatus = -1;
    strconv_t strconv;
    CCrashHandler *pCrashHandler = NULL;

    // Validate input parameters.
    if(pInfo == NULL || pInfo->cb != sizeof(CR_INSTALL_INFOW))     
    {   
		if (pInfo != NULL && pInfo->cb == 0)
		{
			pInfo->cb = sizeof(CR_INSTALL_INFOW);
		}
		else
		{
			OutputErrorStr(_T("pInfo is NULL or pInfo->cb member is not valid."));
			nStatus = 1;
			goto cleanup;
		}
    }

    // Check if crInstall() already was called for current process.
    pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();

    if(pCrashHandler != NULL && pCrashHandler->IsInitialized())
    {    
		OutputErrorStr(_T("Can't install crash handler to the same process twice."));
        nStatus = 2; 
        goto cleanup;
    }

    if(pCrashHandler == NULL)
    {
        pCrashHandler = new CCrashHandler();
        if(pCrashHandler == NULL)
        {    
            OutputErrorStr(_T("Error allocating memory for crash handler."));
            nStatus = 3; 
            goto cleanup;
        }
    }

    MINIDUMP_TYPE miniDumpType = pInfo->uMiniDumpType;
    LPCTSTR ptszRestartCmdLine = strconv.w2t((LPWSTR)pInfo->pszRestartCmdLine);

    int nInitResult = pCrashHandler->Init(pInfo->dwFlags, miniDumpType, ptszRestartCmdLine);

    if(nInitResult!=0)
    {    
        nStatus = 4;
        goto cleanup;
    }

    // OK.
    nStatus = 0;

cleanup:

    if(nStatus != 0) // If failed
    {
        if(pCrashHandler!=NULL && !pCrashHandler->IsInitialized())
        {
            // Release crash handler object
            CCrashHandler::ReleaseCurrentProcessCrashHandler();
        }
    }

    return nStatus;
}

int CRASHRPTAPI crInstallA(CR_INSTALL_INFOA* pInfo)
{
    int nStatus = -1;
    strconv_t strconv;
    CCrashHandler *pCrashHandler = NULL;

    // Validate input parameters.
    if(NULL == pInfo || pInfo->cb!=sizeof(CR_INSTALL_INFOA))     
    {     
		if (pInfo != NULL && pInfo->cb == 0)
		{
			pInfo->cb = sizeof(CR_INSTALL_INFOA);
		}
		else
		{
			OutputErrorStr(_T("pInfo is NULL or pInfo->cb member is not valid."));
			nStatus = 1;
			goto cleanup;
		}
    }

    // Check if crInstall() already was called for current process.
    pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();

    if(NULL != pCrashHandler && pCrashHandler->IsInitialized())
    {    
        OutputErrorStr(_T("Can't install crash handler to the same process twice."));
        nStatus = 2; 
        goto cleanup;
    }

    if(NULL == pCrashHandler)
    {
        pCrashHandler = new CCrashHandler();
        if(NULL == pCrashHandler)
        {    
            OutputErrorStr(_T("Error allocating memory for crash handler."));
            nStatus = 3; 
            goto cleanup;
        }
    }

    MINIDUMP_TYPE miniDumpType = pInfo->uMiniDumpType;
    LPCTSTR ptszRestartCmdLine = strconv.a2t((LPSTR)pInfo->pszRestartCmdLine);

    int nInitResult = pCrashHandler->Init(pInfo->dwFlags, miniDumpType, ptszRestartCmdLine);

    if(nInitResult!=0)
    {    
        nStatus = 4;
        goto cleanup;
    }

    // OK.
    nStatus = 0;

cleanup:

    if(nStatus!=0) // If failed
    {
        if(NULL != pCrashHandler && !pCrashHandler->IsInitialized())
        {
            // Release crash handler object
            CCrashHandler::ReleaseCurrentProcessCrashHandler();
        }
    }

    return nStatus;
}

int CRASHRPTAPI crUninstall()
{
	// Get crash handler singleton
    CCrashHandler *pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();

	// Check if found
    if(pCrashHandler == NULL || !pCrashHandler->IsInitialized())
    {     
        OutputErrorStr(_T("Crash handler wasn't preiviously installed for this process."));
        return 1; 
    }

    // Uninstall main thread's C++ exception handlers
    int nUnset = pCrashHandler->UnSetThreadExceptionHandlers();
    if(nUnset != 0)
        return 2;

	// Destroy the crash handler.
    int nDestroy = pCrashHandler->Destroy();
    if(nDestroy != 0)
        return 3;

	// Free the crash handler object.
    delete pCrashHandler;

    return 0;
}

// Sets C++ exception handlers for the calling thread
int CRASHRPTAPI crInstallToCurrentThread(DWORD dwFlags)
{
    CCrashHandler *pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();

    if(pCrashHandler == NULL)
    {    
        OutputErrorStr(_T("Crash handler was already installed for current thread."));
        return 1; 
    }

    int nResult = pCrashHandler->SetThreadExceptionHandlers(dwFlags);
    if(nResult != 0)
        return 2; // Error?

    // Ok.
    return 0;
}

// Unsets C++ exception handlers from the calling thread
int CRASHRPTAPI crUninstallFromCurrentThread()
{
    CCrashHandler *pCrashHandler = 
        CCrashHandler::GetCurrentProcessCrashHandler();

    if(pCrashHandler == NULL)
    {
        ATLASSERT(pCrashHandler != NULL);
        OutputErrorStr(_T("Crash handler wasn't previously installed for current thread."));
        return 1; // Invalid parameter?
    }

    int nResult = pCrashHandler->UnSetThreadExceptionHandlers();
    if(nResult != 0)
        return 2; // Error?

    // OK.
    return 0;
}

int CRASHRPTAPI crAddScreenshot(DWORD dwFlags)
{
    CCrashHandler *pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
    if(pCrashHandler == NULL)
    {    
        OutputErrorStr(_T("Crash handler wasn't previously installed for current process."));
        return 1; // Invalid parameter?
    }
    return pCrashHandler->AddScreenshot(dwFlags);
}

int CRASHRPTAPI crGenerateErrorReport(CR_EXCEPTION_INFO* pExceptionInfo)
{
    if(pExceptionInfo == NULL || pExceptionInfo->cb != sizeof(CR_EXCEPTION_INFO))
    {
        OutputErrorStr(_T("Exception info is NULL or invalid."));    
        return 1;
    }

    CCrashHandler *pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
    if(pCrashHandler == NULL)
    {    
        // Handler is not installed for current process 
        OutputErrorStr(_T("Crash handler wasn't previously installed for current process."));
        ATLASSERT(pCrashHandler!=NULL);
        return 2;
    } 

    return pCrashHandler->GenerateErrorReport(pExceptionInfo);  
}


