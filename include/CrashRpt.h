

/*! \file   CrashRpt.h
*  \brief  Defines the interface for the CrashRpt.DLL. 
*/

#ifndef _CRASHRPT_H_
#define _CRASHRPT_H_

#include <windows.h>
#include "dbghelp.h"

#ifdef __cplusplus // Use undecorated names
extern "C" {
#endif

#ifndef CRASHRPT_LIB // If CrashRpt is used as DLL
#define CRASHRPT_DLLEXPORT __declspec(dllexport) 
#define CRASHRPT_DLLIMPORT __declspec(dllimport) 
#else // If CrashRpt is used as static library
#define CRASHRPT_DLLEXPORT 
#define CRASHRPT_DLLIMPORT
#endif

// This is needed for exporting/importing functions from/to CrashRpt.dll
#ifdef CRASHRPT_EXPORTS
#define CRASHRPTAPI CRASHRPT_DLLEXPORT WINAPI 
#else 
#define CRASHRPTAPI CRASHRPT_DLLIMPORT WINAPI
#endif


/*!
\enum EXCEPTION_TYPES 
\brief
	Exception types used in CR_EXCEPTION_INFO::exctype structure member.
*/
enum EXCEPTION_TYPES
{
	 CR_SEH_EXCEPTION               = 0,	//!< SEH exception.
	 CR_CPP_TERMINATE_CALL           ,		//!< C++ terminate() call.
	 CR_CPP_UNEXPECTED_CALL          ,		//!< C++ unexpected() call.
	 CR_CPP_PURE_CALL                ,		//!< C++ pure virtual function call (VS .NET and later).
	 CR_CPP_NEW_OPERATOR_ERROR       ,		//!< C++ new operator fault (VS .NET and later).
	 CR_CPP_INVALID_PARAMETER       = 5,	//!< Invalid parameter exception (VS 2005 and later).
	 CR_CPP_SIGABRT                  ,		//!< C++ SIGABRT signal (abort).
	 CR_CPP_SIGFPE                   ,		//!< C++ SIGFPE signal (flotating point exception).
	 CR_CPP_SIGILL                   ,		//!< C++ SIGILL signal (illegal instruction).
	 CR_CPP_SIGINT                   ,		//!< C++ SIGINT signal (CTRL+C).
	 CR_CPP_SIGSEGV                = 10,	//!< C++ SIGSEGV signal (invalid storage access).
	 CR_CPP_SIGTERM                  ,		//!< C++ SIGTERM signal (termination request).
	 CR_NONCONTINUABLE_EXCEPTION     ,		//!< Non continuable sofware exception. 
	 CR_THROW						 ,		//!< Throw C++ typed exception.
	 MANUAL_REPORT					 		//!< Manual report .
};

/*! 
*   \brief This structure contains information about the crash.
*  The information provided by this structure includes the exception type, exception code, 
*  exception pointers and so on. These are needed to generate crash minidump file and
*  provide the developer with other information about the error. This structure is used by
*  the crGenerateErrorReport() function.
*
*  Structure members details are provided below:
*  \b cb [in] 
*  Size of this structure in bytes.
*
*  \b pexcptrs [in, optional]
*    Should contain the exception pointers. If this parameter is NULL, 
*    the current CPU state is used to generate exception pointers.
*
*  \b exctype [in] 
*    The type of exception. This parameter may be one of the following:
*     - \ref CR_SEH_EXCEPTION             SEH (Structured Exception Handling) exception
*     - \ref CR_CPP_TERMINATE_CALL        C++ terminate() function call
*     - \ref CR_CPP_UNEXPECTED_CALL       C++ unexpected() function call
*     - \ref CR_CPP_PURE_CALL             Pure virtual method call (Visual Studio .NET 2003 and later) 
*     - \ref CR_CPP_NEW_OPERATOR_ERROR    C++ 'new' operator error (Visual Studio .NET 2003 and later)
*     - \ref CR_CPP_SECURITY_ERROR        Buffer overrun (Visual Studio .NET 2003 only) 
*     - \ref CR_CPP_INVALID_PARAMETER     Invalid parameter error (Visual Studio 2005 and later) 
*     - \ref CR_CPP_SIGABRT               C++ SIGABRT signal 
*     - \ref CR_CPP_SIGFPE                C++ floating point exception
*     - \ref CR_CPP_SIGILL                C++ illegal instruction
*     - \ref CR_CPP_SIGINT                C++ SIGINT signal
*     - \ref CR_CPP_SIGSEGV               C++ invalid storage access
*     - \ref CR_CPP_SIGTERM               C++ termination request
* 
*   \b code [in, optional]
*      Used if \a exctype is \ref CR_SEH_EXCEPTION and represents the SEH exception code. 
*      If \a pexptrs is NULL, this value is used when generating exception information for initializing
*      \c pexptrs->ExceptionRecord->ExceptionCode member, otherwise it is ignored
*
*   \b fpe_subcode [in, optional]
*      Used if \a exctype is equal to \ref CR_CPP_SIGFPE. It defines the floating point
*      exception subcode (see \c signal() function ducumentation in MSDN).
* 
*   \b expression, \b function, \b file and \b line [in, optional]
*     These parameters are used when \a exctype is \ref CR_CPP_INVALID_PARAMETER. 
*     These members are typically non-zero when using debug version of CRT.
*
*  \b bManual [in]
*     \a bManual parameter should be equal to TRUE if the report is generated manually. 
*     The value of \a bManual parameter affects the automatic application restart behavior. If the application
*     restart is requested by the \ref CR_INST_APP_RESTART flag of CR_INSTALL_INFO::dwFlags structure member, 
*     and if \a bManual is FALSE, the application will be
*     restarted after error report generation. If \a bManual is TRUE, the application won't be restarted.
*
*  \b hExportProcess [out]
*
*     \a hExportProcess parameter contain the handle to the <b>crashExporter.exe</b> process when 
*     \ref crGenerateErrorReport function returns. The caller may use this handle to wait until <b>crashExporter.exe</b> 
*     process exits and check the exit code. When the handle is not needed anymore, release it with the \b CloseHandle() function.
*/
typedef struct tagCR_EXCEPTION_INFO
{
	WORD cb;                   //!< Size of this structure in bytes; should be initialized before using.
	PEXCEPTION_POINTERS pexcptrs; //!< Exception pointers.
	int exctype;               //!< Exception type.
	DWORD code;                //!< Code of SEH exception.
	unsigned int fpe_subcode;  //!< Floating point exception subcode.
	const wchar_t* expression; //!< Assertion expression.
	const wchar_t* function;   //!< Function in which assertion happened.
	const wchar_t* file;       //!< File in which assertion happened.
	unsigned int line;         //!< Line number.
	BOOL bManual;              //!< Flag telling if the error report is generated manually or not.
	HANDLE hExportProcess;     //!< Handle to the crashExporter.exe process.
}
CR_EXCEPTION_INFO;

typedef CR_EXCEPTION_INFO* PCR_EXCEPTION_INFO;


// Flags for CR_INSTALL_INFO::dwFlags
#define CR_INST_STRUCTURED_EXCEPTION_HANDLER      0x1 //!< Install SEH handler (deprecated name, use \ref CR_INST_SEH_EXCEPTION_HANDLER instead).
#define CR_INST_SEH_EXCEPTION_HANDLER             0x1 //!< Install SEH handler.
#define CR_INST_TERMINATE_HANDLER                 0x2 //!< Install terminate handler.
#define CR_INST_UNEXPECTED_HANDLER                0x4 //!< Install unexpected handler.
#define CR_INST_PURE_CALL_HANDLER                 0x8 //!< Install pure call handler (VS .NET and later).
#define CR_INST_NEW_OPERATOR_ERROR_HANDLER       0x10 //!< Install new operator error handler (VS .NET and later).
#define CR_INST_SECURITY_ERROR_HANDLER           0x20 //!< Install security error handler (VS .NET and later).
#define CR_INST_INVALID_PARAMETER_HANDLER        0x40 //!< Install invalid parameter handler (VS 2005 and later).
#define CR_INST_SIGABRT_HANDLER                  0x80 //!< Install SIGABRT signal handler.
#define CR_INST_SIGFPE_HANDLER                  0x100 //!< Install SIGFPE signal handler.   
#define CR_INST_SIGILL_HANDLER                  0x200 //!< Install SIGILL signal handler.  
#define CR_INST_SIGINT_HANDLER                  0x400 //!< Install SIGINT signal handler.  
#define CR_INST_SIGSEGV_HANDLER                 0x800 //!< Install SIGSEGV signal handler.
#define CR_INST_SIGTERM_HANDLER                0x1000 //!< Install SIGTERM signal handler.  

#define CR_INST_ALL_POSSIBLE_HANDLERS          0x1FFF //!< Install all possible exception handlers.
#define CR_INST_CRT_EXCEPTION_HANDLERS         0x1FFE //!< Install exception handlers for the linked CRT module.

#define CR_INST_SHOW_MESSAGEBOX                0x2000 //!< Show error exporter Messagebox.
#define CR_INST_APP_RESTART                    0x6000 //!< Restart the application on crash.
#define CR_INST_NO_MINIDUMP					   0x8000 //!< Do not include minidump file to crash report.
#define CR_INST_NO_STACKWALK				  0x10000 //!< Do not include stackwalk file to crash report.
#define CR_INST_AUTO_THREAD_HANDLERS          0x40000 //!< If this flag is set, installs exception handlers for newly created threads automatically.

/*! 
*  \struct CR_INSTALL_INFOW()
*
*  \brief This structure defines the general information used by crInstallW() function.
*
*  \remarks
*    \ref CR_INSTALL_INFOW and \ref CR_INSTALL_INFOA structures are wide-character and multi-byte character 
*    versions of \ref CR_INSTALL_INFO. \ref CR_INSTALL_INFO typedef defines character set independent mapping.
*
*    Below, structure members are described in details. Required parameters must always be specified, while optional
*    ones may be set with 0 (zero) or NULL. Most of parameters are optional.
*
*    \b cb [in, optional] 
*    The size of this structure in bytes. 
*
*    \b dwFlags [in, optional]
*    Define behavior parameters. This can be a combination of the following values:
*    <table>
*    <tr><td colspan="2"> <i>Use the combination of the following constants to specify what exception handlers to install:</i>
*    <tr><td> \ref CR_INST_ALL_POSSIBLE_HANDLERS      <td> Install all available exception handlers.
*    <tr><td> \ref CR_INST_SEH_EXCEPTION_HANDLER      <td> Install SEH exception handler.
*    <tr><td> \ref CR_INST_PURE_CALL_HANDLER          <td> Install pure call handler (VS .NET and later).
*    <tr><td> \ref CR_INST_NEW_OPERATOR_ERROR_HANDLER <td> Install new operator error handler (VS .NET and later).
*    <tr><td> \ref CR_INST_SECURITY_ERROR_HANDLER     <td> Install security errror handler (VS .NET and later).
*    <tr><td> \ref CR_INST_INVALID_PARAMETER_HANDLER  <td> Install invalid parameter handler (VS 2005 and later).
*    <tr><td> \ref CR_INST_SIGABRT_HANDLER            <td> Install SIGABRT signal handler.
*    <tr><td> \ref CR_INST_SIGINT_HANDLER             <td> Install SIGINT signal handler.  
*    <tr><td> \ref CR_INST_SIGTERM_HANDLER            <td> Install SIGTERM signal handler.  
*   </table>
*
*   \b uMiniDumpType [in, optional] 
*     This parameter defines the minidump type. For the list of available minidump
*     types, see the documentation for the MiniDumpWriteDump() function in MSDN. 
*     It is recommended to set this 
*     parameter with zero (equivalent of MiniDumpNormal constant). Other values may increase the minidump 
*     size significantly. 
*
*   \b pszRestartCmdLine [in, optional] 
*     This arameter defines the string that specifies the 
*     command-line arguments for the application when it is restarted (when using \ref CR_INST_APP_RESTART flag). 
*     Do not include the name of the executable in the command line; it is added automatically. This parameter 
*     can be NULL. 
*/
typedef struct tagCR_INSTALL_INFOW
{
	WORD cb;                        //!< Size of this structure in bytes; must be initialized before using!
	DWORD dwFlags;                  //!< Flags.
	MINIDUMP_TYPE uMiniDumpType;    //!< Minidump type.
	LPCWSTR pszRestartCmdLine;      //!< Command line for application restart (without executable name).

	tagCR_INSTALL_INFOW()
	{
		cb = 0;
		dwFlags = 0;
		uMiniDumpType = MiniDumpNormal;
		pszRestartCmdLine = NULL;
	}
}
CR_INSTALL_INFOW;

typedef CR_INSTALL_INFOW* PCR_INSTALL_INFOW;

/*! 
*  \struct CR_INSTALL_INFOA
*  \copydoc CR_INSTALL_INFOW
*/

typedef struct tagCR_INSTALL_INFOA
{
	WORD cb;                       //!< Size of this structure in bytes; must be initialized before using!
	DWORD dwFlags;                 //!< Flags.
	MINIDUMP_TYPE uMiniDumpType;   //!< Mini dump type.
	LPCSTR pszRestartCmdLine;      //!< Command line for application restart (without executable name).

	tagCR_INSTALL_INFOA()
	{
		cb = 0;
		dwFlags = 0;
		uMiniDumpType = MiniDumpNormal;
		pszRestartCmdLine = NULL;
	}
}
CR_INSTALL_INFOA;

typedef CR_INSTALL_INFOA* PCR_INSTALL_INFOA;

/*! \brief Character set-independent mapping of CR_INSTALL_INFOW and CR_INSTALL_INFOA structures. 
*/
#ifdef UNICODE
typedef CR_INSTALL_INFOW CR_INSTALL_INFO;
typedef PCR_INSTALL_INFOW PCR_INSTALL_INFO;
#else
typedef CR_INSTALL_INFOA CR_INSTALL_INFO;
typedef PCR_INSTALL_INFOA PCR_INSTALL_INFO; 
#endif // UNICODE

/*!
*  \brief  Installs exception handlers for the caller process.
*
*  \return
*    This function returns zero if succeeded.
*
*  \param[in] pInfo General congiration information.
*
*  \remarks
*
*    This function installs unhandled exception filter for the caller process.
*    It also installs various CRT exception/error handlers that function for all threads of the caller process.
*    For more information, see \ref exception_handling
*
*    Below is the list of installed handlers:
*     - Top-level SEH exception filter [ \c SetUnhandledExceptionFilter() ]
*     - C++ pure virtual call handler (Visual Studio .NET 2003 and later) [ \c _set_purecall_handler() ]
*     - C++ invalid parameter handler (Visual Studio .NET 2005 and later) [ \c _set_invalid_parameter_handler() ]
*     - C++ new operator error handler (Visual Studio .NET 2003 and later) [ \c _set_new_handler() ]
*     - C++ buffer overrun handler (Visual Studio .NET 2003 only) [ \c _set_security_error_handler() ]
*     - C++ abort handler [ \c signal(SIGABRT) ]
*     - C++ illegal instruction handler [ \c signal(SIGINT) ]
*     - C++ termination request [ \c signal(SIGTERM) ]
*
*    In a multithreaded program, additionally use crInstallToCurrentThread() function for each execution
*    thread, except the main one.
* 
*    The \a pInfo parameter contains all required information needed to install CrashRpt.
*
*    On crash, the crash minidump file is created, which contains CPU information and 
*    stack trace information. 
*
*    When crash information is collected, another process, <b>crashExporter.exe</b>, is launched 
*    and the process where crash had occured is terminated. The crashExporter process is 
*    responsible for letting the user know about the crash and export the error report.
*
*    crInstallW() and crInstallA() are wide-character and multi-byte character versions of crInstall()
*    function. The \ref crInstall macro defines character set independent mapping for these functions.
*
*  \sa crInstallW(), crInstallA(), crInstall(), CR_INSTALL_INFOW, 
*      CR_INSTALL_INFOA, CR_INSTALL_INFO, crUninstall()
*/
int CRASHRPTAPI crInstallW(PCR_INSTALL_INFOW pInfo);

/*! 
*  \copydoc crInstallW()
*/
int CRASHRPTAPI crInstallA(PCR_INSTALL_INFOA pInfo);

/*! \brief Character set-independent mapping of crInstallW() and crInstallA() functions. 
*/
#ifdef UNICODE
#define crInstall crInstallW
#else
#define crInstall crInstallA
#endif //UNICODE

/*! 
*  \brief Uninitializes the CrashRpt library and unsinstalls exception handlers previously installed with crInstall().
*
*  \return
*    This function returns zero if succeeded.
*
*  \remarks
*    Call this function on application exit to uninitialize the library and uninstall exception
*    handlers previously installed with crInstall(). After function call, the exception handlers
*    are restored to states they had before calling crInstall().
*
*    This function fails if crInstall() wasn't previously called in context of the
*    caller process.
*
*  \sa crInstallW(), crInstallA(), crInstall(),
*/

int CRASHRPTAPI crUninstall();

/*! 
*  \brief Installs exception handlers to the caller thread.
*  \return This function returns zero if succeeded.
*  \param[in] dwFlags Flags.
*
*  \remarks
*
*  The function sets exception handlers for the caller thread. If you have
*  several execution threads, you ought to call the function for each thread,
*  except the main one.
*   
*  \a dwFlags defines what exception handlers to install. Use zero value
*  to install all possible exception handlers. Or use a combination of the following constants:
*
*      - \ref CR_INST_TERMINATE_HANDLER              Install terminate handler
*      - \ref CR_INST_UNEXPECTED_HANDLER             Install unexpected handler
*      - \ref CR_INST_SIGFPE_HANDLER                 Install SIGFPE signal handler   
*      - \ref CR_INST_SIGILL_HANDLER                 Install SIGILL signal handler  
*      - \ref CR_INST_SIGSEGV_HANDLER                Install SIGSEGV signal handler 
* 
*  Example:
*
*   \code
*   DWORD WINAPI ThreadProc(LPVOID lpParam)
*   {
*     // Install exception handlers
*     crInstallToCurrentThread(0);
*
*     // Your code...
*
*     // Uninstall exception handlers
*     crUninstallFromCurrentThread();
*    
*     return 0;
*   }
*   \endcode
* 
*  \sa 
*    crInstall()
*/
int CRASHRPTAPI crInstallToCurrentThread(DWORD dwFlags);

/*!   
*  \brief Uninstalls C++ exception handlers from the current thread.
*  \return This function returns zero if succeeded.
*  
*  \remarks
*
*    This function unsets exception handlers from the caller thread. If you have
*    several execution threads, you ought to call the function for each thread.
*    After calling this function, the exception handlers for current thread are
*    replaced with the handlers that were before call of crInstallToCurrentThread().
*
*    This function fails if crInstallToCurrentThread() wasn't called for current thread.
*
*    No need to call this function for the main execution thread. The crUninstall()
*    will automatically uninstall C++ exception handlers for the main thread.
*/
int CRASHRPTAPI crUninstallFromCurrentThread();

// Flags for crAddScreenshot function.
#define CR_AS_VIRTUAL_SCREEN  0  //!< Take a screenshot of the virtual screen.
#define CR_AS_MAIN_WINDOW     1  //!< Take a screenshot of application's main window.
#define CR_AS_PROCESS_WINDOWS 2  //!< Take a screenshot of all visible process windows.
#define CR_AS_GRAYSCALE_IMAGE 4  //!< Make a grayscale image instead of a full-color one.

/*! 
*  \brief Adds a screenshot to the crash report.
*
*  \return This function returns zero if succeeded. 
*
*  \param[in] dwFlags Flags, optional.
*  
*  \remarks 
*
*  This function can be used to take a screenshot at the moment of crash and add it to the error report. 
*  Screenshot information may help the developer to better understand the state of the application
*  at the moment of crash and reproduce the error.
*
*  When this function is called, screenshot flags are saved, 
*  then the function returns control to the caller.
*  When crash occurs, screenshot is made by the \b crashExporter.exe process and added to the report. 
* 
*  \b dwFlags 
*
*    Use one of the following constants to specify what part of virtual screen to capture:
*    - \ref CR_AS_VIRTUAL_SCREEN  Use this to take a screenshot of the whole desktop (virtual screen).
*    - \ref CR_AS_MAIN_WINDOW     Use this to take a screenshot of the application's main window.
*    - \ref CR_AS_PROCESS_WINDOWS Use this to take a screenshot of all visible windows that belong to the process.
*
*  The main application window is a window that has a caption (\b WS_CAPTION), system menu (\b WS_SYSMENU) and
*  the \b WS_EX_APPWINDOW extended style. If CrashRpt doesn't find such window, it considers the first found process window as
*  the main window.
*
*  Screenshots are added in form of PNG files by default. 
*  In addition, you can specify the \ref CR_AS_GRAYSCALE_IMAGE flag to make a grayscale screenshot 
*  (by default color image is made). Grayscale image gives smaller file size.
*
*  When capturing entire desktop consisting of several monitors, 
*  one screenshot file is added per each monitor. 
*/
int CRASHRPTAPI crAddScreenshot(DWORD dwFlags);

/*! 
*  \brief Manually generates an error report.
*
*  \return This function returns zero if succeeded. 
*  
*  \param[in] pExceptionInfo Exception information. 
*
*  \remarks
*
*    Call this function to manually generate a crash report. When crash information is collected,
*    control is returned to the caller. The crGenerateErrorReport() doesn't terminate the caller process.
*
*    The crash report may contain the crash minidump file, crash info file. 
*    The exception information should be passed using \ref CR_EXCEPTION_INFO structure. 
*
*    The following example shows how to use crGenerateErrorReport() function.
*
*    \code
*    CR_EXCEPTION_INFO ei;
*    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
*    ei.cb = sizeof(CR_EXCEPTION_INFO);
*    ei.exctype = CR_SEH_EXCEPTION;
*    ei.code = 1234;
*    ei.pexcptrs = NULL;
*
*    int result = crGenerateErrorReport(&ei);
*
*    if(result!=0)
*    {
*      // If goes here, crGenerateErrorReport() has failed
*    }
*   
*    // Manually terminate program
*    ExitProcess(0);
*
*    \endcode
*/
int CRASHRPTAPI crGenerateErrorReport(CR_EXCEPTION_INFO* pExceptionInfo);

#ifdef __cplusplus
}
#endif

#endif //_CRASHRPT_H_


