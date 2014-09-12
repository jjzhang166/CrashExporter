

/*! \file  StackWalker.h
*  \brief  display the callstack of the thread which you are interested.
*/

#pragma once

#include <vector>
/*!
\class StackWalker 
\brief
	Display the call stack at a given point in the human readable form.
*/
class StackWalker
{
public:

	StackWalker(DWORD dwProcessId = GetCurrentProcessId(), HANDLE hProcess = GetCurrentProcess());
	virtual ~StackWalker();
	
	/*! 
	\brief recode Callstack
	*/
	void	ShowCallstack(const CONTEXT *context, HANDLE hThread = GetCurrentThread());	

protected:

	/*!
	\brief	    Print - Sends a message to the debugger for display
				and/or to a file.
	\remarks	A message longer than MAXREPORTLENGTH characters will be truncated
				to MAXREPORTLENGTH.
	\param[in]  message - Arguments to be formatted using the specified format string.
	*/
	virtual void Print(LPCWSTR szText);

private:

	/*! 
	\brief	Builds the symbol search path for the symbol handler.
		This helps the symbol handler find the symbols for the application being
		debugged.
	\return
		Returns a string containing the search path. The caller is responsible for
		freeing the string.
	\remarks
	*/
	LPWSTR	buildSymbolSearchPath();

	/*! 
	\brief Initialize the symbol handler. We use it for obtaining source file/line
			number information and function names for the memory leak report 
	*/
	BOOL	InitSymbolHandler();

	/*! 
	\brief first try toolhelp32, then try psapi.
	*/
	BOOL	LoadModules(HANDLE hProcess, DWORD dwProcessId);

	/*!
	\brief		Pushes a frame's program counter onto the CallStack. Pushes are
				always appended to the back of the chunk list (aka the "top" chunk).
	*/
	void	getStackTrace (const CONTEXT *context, HANDLE hThread);

	/*!
	\brief		Dumps a nicely formatted rendition of the CallStack, including
				symbolic information (function names and line numbers) if available.
	\remarks	The symbol handler must be initialized prior to calling this
				function.
	\param[in]	showinternalframes If true, then all frames in the CallStack will be
				dumped. Otherwise, frames internal will not be dumped.
	*/
	void	dump (BOOL showinternalframes);

	/*!
	\brief		Get module list by tlhelp32
	*/
	BOOL	GetModuleListTH32(HANDLE hProcess, DWORD pid);

	/*!
	\brief		Get module list by psapi
	*/
	BOOL	GetModuleListPSAPI(HANDLE hProcess);

	/*!
	\brief	Check if filename is compiler's internal modules
	\see	dump and resolve
	\param[in]	filename - the module file need to be check.
	\return		return ture,if filename is compiler's internal modules
	*/
	bool	isInternalModule(const PWSTR filename);

	/*!
	\brief	    Report - Sends a printf-style formatted message to the debugger for display
				and/or to a file.
	\remarks	A message longer than MAXREPORTLENGTH characters will be truncated
				to MAXREPORTLENGTH.
	\param[in]  format - Specifies a printf-compliant format string containing the
				message to be sent to the debugger.
	\return	    None.
*/
	void	Report (LPCWSTR format, ...);

	std::vector<UINT_PTR>	m_frameArray;	//!< STACKFRAME64 array build by \sa getStackTrace
	HANDLE					m_hProcess;		
	DWORD					m_dwProcessId;

};  // class StackWalker


/*!
\class COutStackWalker 
\brief
	Overwrite the print function of the base class StackWalker.
	Return the callstack string. 
*/
class COutStackWalker : public StackWalker
{
public:
	CString GetStackWalkerInfo(){ return m_sOutStackWalker; }
	COutStackWalker() : StackWalker() {}
	COutStackWalker(DWORD dwProcessId, HANDLE hProcess) : StackWalker(dwProcessId, hProcess) {}

	virtual void Print(LPCWSTR szText) 
	{ 
		m_sOutStackWalker += szText;
		StackWalker::Print(szText); 	
	}

private:
	CString m_sOutStackWalker; 
};

