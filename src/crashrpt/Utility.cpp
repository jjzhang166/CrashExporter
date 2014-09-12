
// File: Utility.cpp
// Description: Miscellaneous helper functions

#include "stdafx.h"
#include "Utility.h"
#include "strconv.h"
#include <time.h>

CString Utility::getAppName()
{
	TCHAR szFileName[_MAX_PATH];
	GetModuleFileName(NULL, szFileName, _MAX_FNAME);

	CString sAppName; // Extract from last '\' to '.'
	sAppName = szFileName;
	sAppName = sAppName.Mid(sAppName.ReverseFind(_T('\\')) + 1)
		.SpanExcluding(_T("."));

	return sAppName;
}

CString Utility::GetModuleName(HMODULE hModule)
{
	CString string;
	LPTSTR buf = string.GetBuffer(_MAX_PATH);
	GetModuleFileName(hModule, buf, _MAX_PATH);
	string.ReleaseBuffer();
	return string;
}

CString Utility::GetModulePath(HMODULE hModule)
{
	CString string;
	LPTSTR buf = string.GetBuffer(_MAX_PATH);
	GetModuleFileName(hModule, buf, _MAX_PATH);
	TCHAR* ptr = _tcsrchr(buf,'\\');
	if(ptr!=NULL)
		*(ptr)=0; // remove executable name
	string.ReleaseBuffer();
	return string;
}

void Utility::GetSystemTime(CString& sTime)
{
	sTime.Empty();

	WCHAR szCurrentDateTime[128];     
	time_t t = time(NULL);
	tm timeinfo = {0}; 
	localtime_s(&timeinfo, &t);
	_snwprintf_s(szCurrentDateTime, 128, _TRUNCATE, L"%.4d-%.2d-%.2d %.2d:%.2d:%.2d",     
		timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,     
		timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

	sTime = szCurrentDateTime;
}

int Utility::GenerateGUID(CString& sGUID)
{
	int status = 1;
	sGUID.Empty();

	strconv_t strconv;

	// Create GUID

	UCHAR *pszUuid = 0; 
	GUID *pguid = NULL;
	pguid = new GUID;
	if(pguid!=NULL)
	{
		HRESULT hr = CoCreateGuid(pguid);
		if(SUCCEEDED(hr))
		{
			// Convert the GUID to a string
			hr = UuidToStringA(pguid, &pszUuid);
			if(SUCCEEDED(hr) && pszUuid!=NULL)
			{ 
				status = 0;
				sGUID = strconv.a2t((char*)pszUuid);
				RpcStringFreeA(&pszUuid);
			}
		}
		delete pguid; 
	}

	return status;
}

int Utility::GetOSFriendlyName(CString& sOSName)
{
	sOSName.Empty();
	CRegKey regKey;
	LONG lResult = regKey.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), KEY_READ);
	if(lResult==ERROR_SUCCESS)
	{    
		TCHAR buf[1024];
		ULONG buf_size = 0;

		TCHAR* PRODUCT_NAME = _T("ProductName");
		TCHAR* CURRENT_BUILD_NUMBER = _T("CurrentBuildNumber");
		TCHAR* CSD_VERSION = _T("CSDVersion");

#pragma warning(disable:4996)

		buf_size = 1023;
		if(ERROR_SUCCESS == regKey.QueryValue(buf, PRODUCT_NAME, &buf_size))
		{
			sOSName += buf;
		}

		buf_size = 1023;
		if(ERROR_SUCCESS == regKey.QueryValue(buf, CURRENT_BUILD_NUMBER, &buf_size))
		{
			sOSName += _T(" Build ");
			sOSName += buf;
		}

		buf_size = 1023;
		if(ERROR_SUCCESS == regKey.QueryValue(buf, CSD_VERSION, &buf_size))
		{
			sOSName += _T(" ");
			sOSName += buf;
		}

#pragma warning(default:4996)

		regKey.Close();    
		return 0;
	}

	return 1;
}

BOOL Utility::IsOS64Bit()
{
	BOOL b64Bit = FALSE;

#ifdef _WIN64
	// 64-bit applications always run under 64-bit Windows
	return TRUE;
#endif

	// Check for 32-bit applications

	typedef BOOL (WINAPI *PFNISWOW64PROCESS)(HANDLE, PBOOL);

	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	if(hKernel32!=NULL)
	{
		PFNISWOW64PROCESS pfnIsWow64Process = 
			(PFNISWOW64PROCESS)GetProcAddress(hKernel32, "IsWow64Process");
		if(pfnIsWow64Process==NULL)
		{
			// If there is no IsWow64Process() API, than Windows is 32-bit for sure
			FreeLibrary(hKernel32);
			return FALSE;
		}

		pfnIsWow64Process(GetCurrentProcess(), &b64Bit);
		FreeLibrary(hKernel32);
	}

	return b64Bit;
}

CString Utility::FormatErrorMsg(DWORD dwErrorCode)
{
	LPTSTR msg = 0;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&msg, 0, NULL);
	CString str = msg;
	str.Replace(_T("\r\n"), _T(""));
	GlobalFree(msg);
	return str;
}

// Creates a folder. If some intermediate folders in the path do not exist,
// it creates them.
BOOL Utility::CreateFolder(CString sFolderName)
{  
	CString sIntermediateFolder;

	// Skip disc drive name "X:\" if presents
	int start = sFolderName.Find(':', 0);
	if(start>=0)
		start+=2; 

	int pos = start;  
	for(;;)
	{
		pos = sFolderName.Find('\\', pos);
		if(pos<0)
		{
			sIntermediateFolder = sFolderName;
		}
		else
		{
			sIntermediateFolder = sFolderName.Left(pos);
		}

		BOOL bCreate = CreateDirectory(sIntermediateFolder, NULL);
		if(!bCreate && GetLastError()!=ERROR_ALREADY_EXISTS)
			return FALSE;

		DWORD dwAttrs = GetFileAttributes(sIntermediateFolder);
		if((dwAttrs&FILE_ATTRIBUTE_DIRECTORY)==0)
			return FALSE;

		if(pos==-1)
			break;

		pos++;
	}

	return TRUE;
}

ULONG64 Utility::SystemTimeToULONG64( const SYSTEMTIME& st )
{
	FILETIME ft ;
	SystemTimeToFileTime( &st, &ft ) ;
	ULARGE_INTEGER integer ;
	integer.LowPart = ft.dwLowDateTime ;
	integer.HighPart = ft.dwHighDateTime ;
	return integer.QuadPart ;
}


LPWSTR Utility::AppendString (LPWSTR dest, LPCWSTR source)
{
	if ((source == NULL) || (source[0] == '\0'))
	{
		return dest;
	}
	SIZE_T length = wcslen(dest) + wcslen(source);
	LPWSTR new_str = new WCHAR [length + 1];
	wcsncpy_s(new_str, length + 1, dest, _TRUNCATE);
	wcsncat_s(new_str, length + 1, source, _TRUNCATE);
	delete [] dest;
	return new_str;
}

HMODULE Utility::GetCallingModule( UINT_PTR pCaller )
{
	HMODULE hModule = NULL;
	MEMORY_BASIC_INFORMATION mbi;
	if ( VirtualQuery((LPCVOID)pCaller, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == sizeof(MEMORY_BASIC_INFORMATION) )
	{
		// the allocation base is the beginning of a PE file 
		hModule = (HMODULE) mbi.AllocationBase;
	}
	return hModule;
}

#define MAX_MESSAGE_LEN	512
void Utility::OutDebugStrW(LPCWSTR format, ...)
{
	va_list args;
	WCHAR   messageW [MAX_MESSAGE_LEN];

	va_start(args, format);
	int result = _vsnwprintf_s(messageW, MAX_MESSAGE_LEN - 1, _TRUNCATE, format, args);
	va_end(args);
	messageW[MAX_MESSAGE_LEN - 1] = L'\0';

	if (result >= 0)
		OutputDebugStringW(messageW);
}

void Utility::OutDebugStrA(LPCSTR format, ...)
{
	va_list args;
	CHAR   messageA [MAX_MESSAGE_LEN];

	va_start(args, format);
	int result = _vsnprintf_s(messageA, MAX_MESSAGE_LEN - 1, _TRUNCATE, format, args);
	va_end(args);
	messageA[MAX_MESSAGE_LEN - 1] = '\0';

	if (result >= 0)
		OutputDebugStringA(messageA);
}

#undef MAX_MESSAGE_LEN