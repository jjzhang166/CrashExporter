
#include "stdafx.h"
#include "StackWalker.h"
#include "Utility.h"

#ifdef CRASHRPT_LIB
#pragma comment(lib, "dbghelp.lib")  
#pragma comment(lib, "Rpcrt4.lib") 
#pragma comment(lib, "psapi.lib") 
#endif

using namespace Utility;

#define OutputErrorStr(ErrorStr) \
	DbgTrace(_T("%s%s\n"), _T("[CrashExporter] [StackWalker] "), ErrorStr)

StackWalker::StackWalker(DWORD dwProcessId, HANDLE hProcess)
	: m_hProcess(hProcess), m_dwProcessId(dwProcessId)
{
}

StackWalker::~StackWalker()
{
}

void StackWalker::ShowCallstack(const CONTEXT *context, HANDLE hThread)
{
	if(!InitSymbolHandler())
	{
		Report(L"WARNING!!: The symbol handler failed to initialize (error=%lu).\n", GetLastError());
		OutputErrorStr(_T("Failed to initialize symbol handler"));
	}
	if(!LoadModules(m_hProcess, m_dwProcessId))
	{
		Report(L"WARNING!!: LoadModules failed (error=%lu).\n", GetLastError());
		OutputErrorStr(_T("Failed to load modules"));
	}
	getStackTrace(context, hThread);
	dump(FALSE);
}


void StackWalker::Print(LPCWSTR szText)
{
	OutputDebugString(szText);
}

LPWSTR StackWalker::buildSymbolSearchPath ()
{
	// Oddly, the symbol handler ignores the link to the PDB embedded in the
	// executable image. So, we'll manually add the location of the executable
	// to the search path since that is often where the PDB will be located.
	WCHAR   directory [_MAX_DIR] = {0};
	WCHAR   drive [_MAX_DRIVE] = {0};
	LPWSTR  path = new WCHAR [MAX_PATH];
	path[0] = L'\0';

	HMODULE module = GetModuleHandleW(NULL);
	GetModuleFileName(module, path, MAX_PATH);
	_wsplitpath_s(path, drive, _MAX_DRIVE, directory, _MAX_DIR, NULL, 0, NULL, 0);
	wcsncpy_s(path, MAX_PATH, drive, _TRUNCATE);
	path = Utility::AppendString(path, directory);

	// When the symbol handler is given a custom symbol search path, it will no
	// longer search the default directories (working directory, system root,
	// etc). But we'd like it to still search those directories, so we'll add
	// them to our custom search path.
	//
	// Append the working directory.
	path = Utility::AppendString(path, L";.\\");

	// Append the Windows directory.
	WCHAR   windows [MAX_PATH] = {0};
	if (GetWindowsDirectory(windows, MAX_PATH) != 0) 
	{
		path = Utility::AppendString(path, L";");
		path = Utility::AppendString(path, windows);
	}

	// Append the system directory.
	WCHAR   system [MAX_PATH] = {0};
	if (GetSystemDirectory(system, MAX_PATH) != 0) 
	{
		path = Utility::AppendString(path, L";");
		path = Utility::AppendString(path, system);
	}

	// Append %_NT_SYMBOL_PATH%.
	DWORD   envlen = GetEnvironmentVariable(L"_NT_SYMBOL_PATH", NULL, 0);
	if (envlen != 0) 
	{
		LPWSTR env = new WCHAR [envlen];
		if (GetEnvironmentVariable(L"_NT_SYMBOL_PATH", env, envlen) != 0) {
			path = Utility::AppendString(path, L";");
			path = Utility::AppendString(path, env);
		}
		delete [] env;
	}

	// Append %_NT_ALT_SYMBOL_PATH%.
	envlen = GetEnvironmentVariable(L"_NT_ALT_SYMBOL_PATH", NULL, 0);
	if (envlen != 0) 
	{
		LPWSTR env = new WCHAR [envlen];
		if (GetEnvironmentVariable(L"_NT_ALT_SYMBOL_PATH", env, envlen) != 0) {
			path = Utility::AppendString(path, L";");
			path = Utility::AppendString(path, env);
		}
		delete [] env;
	}

	// Append Visual Studio 2010/2008 symbols cache directory.
	HKEY debuggerkey;
	WCHAR symbolCacheDir [MAX_PATH] = {0};
	LSTATUS regstatus = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\VisualStudio\\10.0\\Debugger", 0, KEY_QUERY_VALUE, &debuggerkey);
	if (regstatus != ERROR_SUCCESS) 
		regstatus = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\VisualStudio\\9.0\\Debugger", 0, KEY_QUERY_VALUE, &debuggerkey);

	if (regstatus == ERROR_SUCCESS)
	{
		DWORD valuetype;
		DWORD dirLength = MAX_PATH * sizeof(WCHAR);
		regstatus = RegQueryValueEx(debuggerkey, L"SymbolCacheDir", NULL, &valuetype, (LPBYTE)&symbolCacheDir, &dirLength);
		if (regstatus == ERROR_SUCCESS && valuetype == REG_SZ)
		{
			path = Utility::AppendString(path, L";srv*");
			path = Utility::AppendString(path, symbolCacheDir);
			path = Utility::AppendString(path, L"\\MicrosoftPublicSymbols;srv*");
			path = Utility::AppendString(path, symbolCacheDir);
		}
		RegCloseKey(debuggerkey);
	}

	// Remove any quotes from the path. The symbol handler doesn't like them.
	SIZE_T  pos = 0;
	SIZE_T  length = wcslen(path);
	while (pos < length) 
	{
		if (path[pos] == L'\"') 
		{
			for (SIZE_T  index = pos; index < length; index++) 
			{
				path[index] = path[index + 1];
			}
		}
		pos++;
	}

	return path;
}

BOOL StackWalker::InitSymbolHandler()
{
	BOOL bRet = TRUE;
	// Initialize the symbol handler. We use it for obtaining source file/line
	// number information and function names for the memory leak report
	LPWSTR symbolpath = buildSymbolSearchPath();
#ifdef NOISY_DBGHELP_DIAGOSTICS
	// From MSDN docs about SYMOPT_DEBUG:
	/* To view all attempts to load symbols, call SymSetOptions with SYMOPT_DEBUG. 
	This causes DbgHelp to call the OutputDebugString function with detailed 
	information on symbol searches, such as the directories it is searching and and error messages.
	In other words, this will really pollute the debug output window with extra messages.
	To enable this debug output to be displayed to the console without changing your source code, 
	set the DBGHELP_DBGOUT environment variable to a non-NULL value before calling the SymInitialize function. 
	To log the information to a file, set the DBGHELP_LOG environment variable to the name of the log file to be used.
	*/
	SymSetOptions(SYMOPT_DEBUG | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
#else
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
#endif
	if (!SymInitializeW(this->m_hProcess, symbolpath, FALSE)) 
	{
		bRet = FALSE;
	}
	delete [] symbolpath;

	return bRet;
}


BOOL StackWalker::LoadModules(HANDLE hProcess, DWORD dwProcessId)
{
	// first try toolhelp32
	if (GetModuleListTH32(hProcess, dwProcessId))
		return TRUE;

	// then try psapi
	return GetModuleListPSAPI(hProcess);
}


BOOL StackWalker::GetModuleListTH32(HANDLE hProcess, DWORD pid)
{
	MODULEENTRY32 me32;
	me32.dwSize = sizeof(me32);

	HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (!hModuleSnap)	return FALSE;

	if(Module32First(hModuleSnap, &me32))//get fist module 
	{ 
		do
		{ 
			if (SymLoadModuleExW(hProcess, NULL, me32.szExePath, me32.szModule, (DWORD64)me32.modBaseAddr, 
				me32.modBaseSize, NULL, 0) == 0)
				break; 
		}while(Module32Next(hModuleSnap,&me32)); 
	}	
	else
	{
		OutputErrorStr(_T("Failed to GetModuleListTH32"));
		return FALSE;
	}

	return TRUE;
}  

#define MODULE_SIZE		256
#define EXEPATH_SIZE	256
#define MOUDLE_MAX		1024

BOOL StackWalker::GetModuleListPSAPI(HANDLE hProcess)
{
	BOOL bRet = FALSE;
	HMODULE* phModule = NULL;
	DWORD need;;

	WCHAR   szModule[MODULE_SIZE];
	WCHAR   szExePath[EXEPATH_SIZE];
	MODULEINFO moduleInfo;

	phModule = new HMODULE[MOUDLE_MAX];
	if (!EnumProcessModules(hProcess, phModule, MOUDLE_MAX, &need))
		return FALSE;

	int count = need / sizeof(HMODULE);
	for(int i = 0; i < count; ++i)
	{
		memset(szModule, 0, MODULE_SIZE);
		memset(szExePath, 0, EXEPATH_SIZE);
		// base address, size
		if (!GetModuleInformation(hProcess, phModule[i], &moduleInfo, sizeof(moduleInfo)))
			goto cleanup;

		if (!GetModuleFileNameEx(hProcess, phModule[i], szExePath, EXEPATH_SIZE))
			goto cleanup;

		if (!GetModuleBaseName(hProcess, phModule[i], szModule, MODULE_SIZE))
			goto cleanup;

		if (!SymLoadModuleExW(hProcess, NULL, szExePath, szModule, (DWORD64)moduleInfo.lpBaseOfDll, 
			moduleInfo.SizeOfImage, NULL, 0))
			goto cleanup;
	}
	bRet = TRUE;

cleanup:
	if (phModule)	
	{
		delete phModule;
		phModule = NULL;
	}

	if (!bRet)
		OutputErrorStr(_T("Failed to GetModuleListPSAPI"));
		
	return bRet;
}
#undef MODULE_SIZE
#undef EXEPATH_SIZE
#undef MOUDLE_MAX

void StackWalker::getStackTrace (const CONTEXT *context, HANDLE hThread)
{
	CONTEXT	currentContext = *context;

	// init STACKFRAME for first call
	STACKFRAME64 frame; // in/out stackframe
	memset(&frame, 0, sizeof(frame));
	DWORD machineType;
#ifdef _M_IX86
	// normally, call ImageNtHeader() and use machine info from PE header
	machineType = IMAGE_FILE_MACHINE_I386;
	frame.AddrPC.Offset = currentContext.Eip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = currentContext.Ebp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = currentContext.Esp;
	frame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
	machineType = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset = currentContext.Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = currentContext.Rsp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = currentContext.Rsp;
	frame.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
	machineType = IMAGE_FILE_MACHINE_IA64;
	frame.AddrPC.Offset = currentContext.StIIP;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = currentContext.IntSp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrBStore.Offset = currentContext.RsBSP;
	frame.AddrBStore.Mode = AddrModeFlat;
	frame.AddrStack.Offset = currentContext.IntSp;
	frame.AddrStack.Mode = AddrModeFlat;
#else
#error "Platform not supported!"
#endif
	// Walk the stack.
	//CriticalSectionLocker cs(g_stackWalkLock);

	while (TRUE) 
	{
		if (!StackWalk64(machineType, this->m_hProcess, hThread, &frame, &currentContext, NULL,
			SymFunctionTableAccess64, SymGetModuleBase64, NULL)) 
		{	
			break;	// Couldn't trace back through any more frames.
		}
		if (frame.AddrFrame.Offset == 0)
		{	
			break;	// End of stack.
		}

		// Push this frame's program counter onto the CallStack.
		m_frameArray.push_back((UINT_PTR)frame.AddrPC.Offset);
	}
}

#define MAX_SYMBOL_NAME_LENGTH  256 //!< Maximum symbol name length that we will allow. Longer names will be truncated.
#define MAX_SYMBOL_NAME_SIZE    ((MAX_SYMBOL_NAME_LENGTH * sizeof(WCHAR)) - 1)
#define MAXREPORTLENGTH 511        // Maximum length, in characters, of "report" messages.
void StackWalker::dump(BOOL showInternalFrames)
{

#ifdef _WIN64
#define ADDRESSFORMAT   L"0x%.16X" // Format string for 64-bit addresses
#else
#define ADDRESSFORMAT   L"0x%.8X"  // Format string for 32-bit addresses
#endif // _WIN64

	IMAGEHLP_LINEW64  sourceInfo = { 0 };
	sourceInfo.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);

	BYTE symbolBuffer [sizeof(SYMBOL_INFO) + MAX_SYMBOL_NAME_SIZE] = { 0 };

	WCHAR lowerCaseName [MAX_PATH];
	WCHAR callingModuleName [MAX_PATH];

	const size_t max_size = MAXREPORTLENGTH + 1;

	// Iterate through each frame in the call stack.
	for (UINT32 frame = 0; frame < m_frameArray.size(); frame++)
	{
		// Try to get the source file and line number associated with
		// this program counter address.
		SIZE_T programCounter = m_frameArray[frame];

		BOOL             foundline = FALSE;
		DWORD            displacement = 0;
		foundline = SymGetLineFromAddrW64(this->m_hProcess, programCounter, &displacement, &sourceInfo);
		if (foundline && !showInternalFrames) 
		{
			wcscpy_s(lowerCaseName, sourceInfo.FileName);
			_wcslwr_s(lowerCaseName, wcslen(lowerCaseName) + 1);
			if (isInternalModule(lowerCaseName))
			{
				// Don't show frames in files internal to the heap.
				continue;
			}
		}

		// Initialize structures passed to the symbol handler.
		SYMBOL_INFOW* functionInfo = (SYMBOL_INFOW*)&symbolBuffer;
		functionInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
		functionInfo->MaxNameLen = MAX_SYMBOL_NAME_LENGTH;

		// Try to get the name of the function containing this program
		// counter address.
		DWORD64          displacement64 = 0;
		LPWSTR           functionName;
		if (SymFromAddrW(m_hProcess, programCounter, &displacement64, functionInfo)) 
		{
			functionName = functionInfo->Name;
		}
		else 
		{
			// GetFormattedMessage( GetLastError() );
			functionName = L"(Function name unavailable)";
			displacement64 = 0;
		}


		HMODULE hCallingModule = Utility::GetCallingModule(programCounter);
		LPWSTR moduleName = L"(Module name unavailable)";
		if (hCallingModule && 
			GetModuleFileName(hCallingModule, callingModuleName, _countof(callingModuleName)) > 0)
		{
			moduleName = wcsrchr(callingModuleName, L'\\');
			if (moduleName == NULL)
				moduleName = wcsrchr(callingModuleName, L'/');
			if (moduleName != NULL)
				moduleName++;
			else
				moduleName = callingModuleName;
		}

		// Use static here to increase performance, and avoid heap allocs. Hopefully this won't
		// prove to be an issue in thread safety. If it does, it will have to be simply non-static.
		static WCHAR stack_line[MAXREPORTLENGTH + 1] = L"";
		// Display the current stack frame's information.
		if (foundline)
		{
			if (displacement == 0)
				_snwprintf_s(stack_line, max_size, _TRUNCATE, L"    %s (%d): %s!%s\n", 
				sourceInfo.FileName, sourceInfo.LineNumber, moduleName, functionName);
			else
				_snwprintf_s(stack_line, max_size, _TRUNCATE, L"    %s (%d): %s!%s + 0x%X bytes\n", 
				sourceInfo.FileName, sourceInfo.LineNumber, moduleName, functionName, displacement);
		}
		else
		{
			if (displacement64 == 0)
				_snwprintf_s(stack_line, max_size, _TRUNCATE, L"    " ADDRESSFORMAT L" (File and line number not available): %s!%s\n", 
				programCounter, moduleName, functionName);
			else
				_snwprintf_s(stack_line, max_size, _TRUNCATE, L"    " ADDRESSFORMAT L" (File and line number not available): %s!%s + 0x%X bytes\n", 
				programCounter, moduleName, functionName, (DWORD)displacement64);	

		}
		Print(stack_line);
	}
}

void StackWalker::Report (LPCWSTR format, ...)
{
	va_list args;
	WCHAR   messagew [MAXREPORTLENGTH + 1];

	va_start(args, format);
	int result = _vsnwprintf_s(messagew, MAXREPORTLENGTH + 1, _TRUNCATE, format, args);
	va_end(args);
	messagew[MAXREPORTLENGTH] = L'\0';

	if (result >= 0)
		Print(messagew);
}

#undef  MAX_SYMBOL_NAME_LENGTH
#undef  MAX_SYMBOL_NAME_SIZE
#undef  MAXREPORTLENGTH
#undef  ADDRESSFORMAT

bool StackWalker::isInternalModule( const PWSTR filename )
{
	return wcsstr(filename, L"winocc.cpp") ||
		wcsstr(filename, L"dlgcore.cpp") ||
		wcsstr(filename, L"afxdialogex.cpp") ||
		wcsstr(filename, L"wincore.cpp") ||
		wcsstr(filename, L"thrdcore.cpp") ||
		wcsstr(filename, L"appmodul.cpp") ||
		wcsstr(filename, L"winmain.cpp") ||
		wcsstr(filename, L"cmdtarg.cpp") ||
		wcsstr(filename, L"afxstate.cpp") ||
		wcsstr(filename, L"winsig.c") ||
		wcsstr(filename, L"invarg.c") ||
		wcsstr(filename, L"printf.c") ||
		wcsstr(filename, L"purevirt.c") ||
		wcsstr(filename, L"crt0msg.c") ||
		wcsstr(filename, L"crtlib.c") ||
		wcsstr(filename, L"eh\\throw.cpp ") ||
		wcsstr(filename, L"eh\\hooks.cpp") ||
		wcsstr(filename, L"crashhandler.cpp")||
		wcsstr(filename, L"crashrpt.cpp");

}





