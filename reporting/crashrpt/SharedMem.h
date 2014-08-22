
/*! \file  SharedMem.h
*  \brief  Shared memory class. Used for transferring data from crashed app to crash export process.
*/

#pragma once
#include "stdafx.h"
#include "CritSec.h"
#include <map>

/*! 
\struct STRING_DESC
\brief String block description.
	This structure is followed by (m_wSize-sizeof(STRING_DESC) bytes of string data.
*/
struct STRING_DESC
{
	BYTE m_uchMagic[3]; //!< Magic sequence "STR".  
	WORD m_wSize;       //!< Total bytes occupied by this block.
};

/*! 
\struct CRASH_DESCRIPTION
\brief Crash description. 
*/
struct CRASH_DESCRIPTION
{  
	BYTE m_uchMagic[3];				//!< Magic sequence "CRD"
	WORD m_wSize;					//!< Total bytes occupied by this block.
	DWORD m_dwTotalSize;			//!< Total size of the whole used shared mem.
	UINT m_uFileItems;				//!< Count of file item records.
	UINT m_uCustomProps;			//!< Count of user-defined properties.  
	DWORD m_dwInstallFlags;			//!< Flags passed to crInstall() function.
	MINIDUMP_TYPE m_MinidumpType;	//!< Minidump type.
	BOOL  m_bAddScreenshot;			//!< Add screenshot?
	DWORD m_dwScreenshotFlags;		//!< Screenshot flags.
	DWORD m_dwRestartCmdLineOffs;	//!< Offset of app restart command line.
	DWORD m_dwCrashGUIDOffs;		//!< Offset to crash GUID.
	DWORD m_dwPathToDebugHelpDllOffs; //!< Offset of dbghelp path.
	DWORD m_dwImageNameOffs;		//!< Offset to image name.
	DWORD m_dwProcessId;			//!< Process ID.
	DWORD m_dwThreadId;				//!< Thread ID.
	int m_nExceptionType;			//!< Exception type.
	DWORD m_dwExceptionCode;		//!< SEH exception code.
	DWORD m_dwInvParamExprOffs;		//!< Invalid parameter expression.
	DWORD m_dwInvParamFunctionOffs; //!< Invalid parameter function.
	DWORD m_dwInvParamFileOffs;		//!< Invalid parameter file.
	UINT  m_uInvParamLine;			//!< Invalid parameter line.
	UINT m_uFPESubcode;				//!< FPE subcode.
	PEXCEPTION_POINTERS m_pExceptionPtrs; //!< Exception pointers.    
	DWORD m_dwStackWalkerOffs;		 //!< StackWalker info
};

#define SHARED_MEM_MAX_SIZE 10*1024*1024   /* 10 MB */

/*! 
\class CSharedMem
\brief Used to share memory between CrashRpt.dll and crashExporter.exe 
*/
class CSharedMem
{
public:

	// Construction/destruction
	CSharedMem();  
	~CSharedMem();  

	/*! 
	\brief Initializes shared memory
	*/
	BOOL Init(LPCTSTR szName, BOOL bOpenExisting, ULONG64 uSize);

	/*! 
	\brief Whether initialized or not
	*/
	BOOL IsInitialized() { return NULL != m_hFileMapping; }

	/*! 
	\brief Destroys the object
	*/
	void Destroy();

	/*! 
	\brief  Returns file mapping name
	*/
	CString GetName() { return m_sName; }

	/*! 
	\brief  Returns file mapping size
	*/
	ULONG64 GetSize() { return m_uSize; }

	/*! 
	\brief  Creates a view and returns its start pointer
	*/
	LPBYTE CreateView(DWORD dwOffset, DWORD dwLength);

	/*! 
	\brief  Destroys a view
	*/
	void DestroyView(LPBYTE pViewPtr);

private:

	CString		m_sName;					//!< Name of the file mapping.
	HANDLE		m_hFileMapping;				//!< Memory mapped object
	DWORD		m_dwAllocGranularity;		//!< System allocation granularity  	  
	ULONG64		m_uSize;	      			//!< Size of the file mapping.		  
	std::map<LPBYTE, LPBYTE> m_aViewStartPtrs; //!< Base of the view of the file mapping.    
};


