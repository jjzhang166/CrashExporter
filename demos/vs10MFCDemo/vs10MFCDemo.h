
// vs10MFCDemo.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CVS10MFCDemoApp:
// �йش����ʵ�֣������ vs10MFCDemo.cpp
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