
// vs10MFCDemo.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// CVS10MFCDemoApp:
// 有关此类的实现，请参阅 vs10MFCDemo.cpp
//

class CVS10MFCDemoApp : public CWinApp
{
public:
	CVS10MFCDemoApp();


public:
	virtual BOOL InitInstance();
	virtual int Run(); 


	DECLARE_MESSAGE_MAP()
};

extern CVS10MFCDemoApp theApp;