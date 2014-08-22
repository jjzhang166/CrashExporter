// VC6MFCDemo.h : main header file for the VC6MFCDEMO application
//

#if !defined(AFX_VC6MFCDEMO_H__46B32C3B_8F37_4959_9B8E_03969D258498__INCLUDED_)
#define AFX_VC6MFCDEMO_H__46B32C3B_8F37_4959_9B8E_03969D258498__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CVC6MFCDemoApp:
// See VC6MFCDemo.cpp for the implementation of this class
//

class CVC6MFCDemoApp : public CWinApp
{
public:
	CVC6MFCDemoApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVC6MFCDemoApp)
	public:
	virtual BOOL InitInstance();
	virtual int Run(); 
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CVC6MFCDemoApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VC6MFCDEMO_H__46B32C3B_8F37_4959_9B8E_03969D258498__INCLUDED_)
