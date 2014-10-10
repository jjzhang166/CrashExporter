
/*! \file  stdafx.h
*  \brief  include file for standard system include files, 
	or project specific include files that are used frequently, but are changed infrequently
*/

#pragma once

#include <atldef.h>

#include <atlbase.h>

#include <atlapp.h>
// CString-related includes
#define _WTL_USE_CSTRING
#include <atlmisc.h>

#if _MSC_VER == 1200
#include "dbghelp_VC6.h"
#else
#include "dbghelp.h"
#endif


#include <Psapi.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <signal.h>
#include <exception>



