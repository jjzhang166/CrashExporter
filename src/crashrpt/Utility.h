
/*! \file  Utility.h
*  \brief  Miscellaneous helper functions
*/
#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "stdafx.h"

/*!
\namespace Utility 
\brief
	The main class that collects crash report files. 
*/
namespace Utility  
{
	/*! Returns base name of the EXE file that launched current process. */
	CString getAppName();

	/*! Returns path to directory where EXE or DLL module is located. */
	CString GetModulePath(HMODULE hModule);

	/*! Returns the absolute path and name of the module */
	CString GetModuleName(HMODULE hModule);

	/*! Generates unique identifier (GUID) */
	int GenerateGUID(CString& sGUID);  

	/*! Returns current system time as string. */
	void GetSystemTime(CString& sTime); 

	/*! Returns friendly name of operating system (name, version, service pack) */
	int GetOSFriendlyName(CString& sOSName);   

	/*! Returns TRUE if Windows is 64-bit */
	BOOL IsOS64Bit();  

	/*! Formats the error message. */
	CString FormatErrorMsg(DWORD dwErrorCode);

	/*! Creates a folder. If some intermediate folders in the path do not exist,
	 it creates them. */
	BOOL CreateFolder(CString sFolderName);

	/*! Converts system time to UINT64 */
	ULONG64 SystemTimeToULONG64( const SYSTEMTIME& st );

	/*!
	\brief	    AppendString - Appends the specified source string to the specified destination
				string. Allocates additional space so that the destination string "grows"
				as new strings are appended to it. This is accomplished by deleting the
				destination string after the new longer string is gets the copied contents
				of the destination and additional text. This function is fairly infrequently
				used so efficiency is not a major concern.
	\param[in]  dest - Address of the destination string. Receives the resulting
				combined string after the append operation.
	\param[in]  source - Source string to be appended to the destination string.
	\return	     The new concatenated string. 
	*/
	LPWSTR AppendString (LPWSTR dest, LPCWSTR source);

	/*! Return calling module by address. */
	HMODULE GetCallingModule(UINT_PTR pCaller);

	void OutDebugStrW (LPCWSTR format, ...);
	void OutDebugStrA (LPCSTR format, ...);

#ifdef UNICODE
#define OutDebugStr OutDebugStrW
#else
#define OutDebugStr OutDebugStrA
#endif //UNICODE

#ifdef _DEBUG
#define DbgTrace(...)  //OutDebugStr(__VA_ARGS__) //open while you need
#else
#define DbgTrace(...)  
#endif

};

#endif	// _UTILITY_H_
