
#include "stdafx.h"
#include "ScreenCap.h"
#include "..\crashrpt\Utility.h"

// Disable warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning(disable:4611)

using namespace Utility;

#define OutputErrorStr(ErrorStr) \
	DbgTrace(_T("%s%s\n"), _T("[CrashExporter] [CScreenCapture] "), ErrorStr)

CScreenCapture::CScreenCapture()
	: m_fp(NULL), m_png_ptr(NULL), m_info_ptr(NULL), m_nIdStartFrom(0)
{
}

CScreenCapture::~CScreenCapture()
{
	// Free used resources
}

BOOL CScreenCapture::TakeDesktopScreenshot(	
			LPCTSTR szSaveToDir,
			ScreenshotInfo& ssi, 
			SCREENSHOT_TYPE type, 
			DWORD dwProcessId,
			BOOL bGrayscale,
			int nIdStartFrom)
{   
	// This method takes the desktop screenshot (screenshot of entire virtual screen
	// or screenshot of the main window, or screenshot of all process windows). 

	// First, we need to calculate the area rectangle to capture
	std::vector<CRect> wnd_list; // List of window rectangles

	if(type == SCREENSHOT_TYPE_MAIN_WINDOW) // We need to capture the main window
	{     
		// Take screenshot of the main window
		std::vector<WindowInfo> aWindows; 
		FindWindows(dwProcessId, FALSE, &aWindows);           
		
		if(aWindows.size() > 0)
		{
			wnd_list.push_back(aWindows[0].m_rcWnd);
			ssi.m_aWindows.push_back(aWindows[0]);
		}
	}
	else if(type == SCREENSHOT_TYPE_ALL_PROCESS_WINDOWS) // Capture all process windows
	{          
		std::vector<WindowInfo> aWindows; 
		FindWindows(dwProcessId, TRUE, &aWindows);
			
		int i;
		for(i=0; i<(int)aWindows.size(); i++)
			wnd_list.push_back(aWindows[i].m_rcWnd);
		ssi.m_aWindows = aWindows;
	}
	else // (dwFlags&CR_AS_VIRTUAL_SCREEN)!=0 // Capture the virtual screen
	{
		// Take screenshot of the entire desktop
		CRect rcScreen;
		GetScreenRect(&rcScreen);    
		wnd_list.push_back(rcScreen);
	}

	// Mark screenshot information as valid
	ssi.m_bValid = TRUE;
	// Save virtual screen rect
	GetScreenRect(&ssi.m_rcVirtualScreen);  

	// Save current timestamp
	time(&ssi.m_CreateTime);

	// Capture screen rectangle	
	BOOL bTakeScreenshot = CaptureScreenRect(
		wnd_list, 
		szSaveToDir, 
		nIdStartFrom, 
		bGrayscale, 
		ssi.m_aMonitors);

	if(FALSE == bTakeScreenshot)
	{
		OutputErrorStr(_T("CaptureScreenRect failed"));
		return FALSE;
	}
		
	// Done
	return TRUE;
}

BOOL CScreenCapture::CaptureScreenRect(
									   std::vector<CRect> arcCapture,  
									   CString sSaveDirName,   
									   int nIdStartFrom, 
									   BOOL bGrayscale,
									   std::vector<MonitorInfo>& monitor_list)
{	
	// Init output variables
	monitor_list.clear();
	
	// Set internal variables
	m_nIdStartFrom	= nIdStartFrom;
	m_sSaveDirName	= sSaveDirName;
	m_bGrayscale	= bGrayscale;
	m_arcCapture	= arcCapture;
	m_monitor_list.clear();

	// Get cursor information
	GetCursorPos(&m_ptCursorPos);
	m_CursorInfo.cbSize = sizeof(CURSORINFO);
	GetCursorInfo(&m_CursorInfo);

	// Perform actual capture task inside of EnumMonitorsProc
	EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, (LPARAM)this);	

	// Return
	monitor_list = m_monitor_list;
	return TRUE;
}

// This function is used for monitor enumeration
BOOL CALLBACK CScreenCapture::EnumMonitorsProc(HMONITOR hMonitor, HDC /*hdcMonitor*/, LPRECT lprcMonitor, LPARAM dwData)
{	  
	CScreenCapture* psc = (CScreenCapture*)dwData;

	MONITORINFOEX mi;
	HDC hDC			= NULL;  
	HDC hCompatDC	= NULL;
	HBITMAP hBitmap = NULL;
	BITMAPINFO bmi;
	int nWidth		= 0;
	int nHeight		= 0;
	int nRowWidth	= 0;
	LPBYTE pRowBits = NULL;
	CString sFileName;
	MonitorInfo monitor_info;

	// Get monitor rect size
	nWidth	= lprcMonitor->right - lprcMonitor->left;
	nHeight = lprcMonitor->bottom - lprcMonitor->top;

	// Get monitor info
	mi.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &mi);

	// Get the device context for this monitor
	hDC = CreateDC(_T("DISPLAY"), mi.szDevice, NULL, NULL); 
	if(hDC==NULL)
		goto cleanup;

	hCompatDC = CreateCompatibleDC(hDC);
	if(NULL == hCompatDC)	goto cleanup;

	hBitmap = CreateCompatibleBitmap(hDC, nWidth, nHeight);
	if(NULL == hBitmap)		goto cleanup;

	SelectObject(hCompatDC, hBitmap);
	int i;
	for(i = 0; i < (int)psc->m_arcCapture.size(); i++)
	{
		CRect rc = psc->m_arcCapture[i];
		CRect rcIntersect;
		if(IntersectRect(&rcIntersect, lprcMonitor, &rc))
		{
			BOOL bBitBlt = BitBlt(hCompatDC, rc.left-lprcMonitor->left, rc.top-lprcMonitor->top, 
				rc.Width(), rc.Height(), hDC, rc.left-lprcMonitor->left, rc.top-lprcMonitor->top, SRCCOPY|CAPTUREBLT);
			if(!bBitBlt)
				goto cleanup;
		}
	}

	// Draw mouse cursor.
	if(PtInRect(lprcMonitor, psc->m_ptCursorPos))
	{						
		if(psc->m_CursorInfo.flags == CURSOR_SHOWING)
		{
			ICONINFO IconInfo;
			GetIconInfo((HICON)psc->m_CursorInfo.hCursor, &IconInfo);
			int x = psc->m_ptCursorPos.x - lprcMonitor->left - IconInfo.xHotspot;
			int y = psc->m_ptCursorPos.y - lprcMonitor->top  - IconInfo.yHotspot;
			DrawIcon(hCompatDC, x, y, (HICON)psc->m_CursorInfo.hCursor);
			DeleteObject(IconInfo.hbmMask);
			DeleteObject(IconInfo.hbmColor);
		}				
	}

	/* Write screenshot bitmap to an image file. */

	// Init PNG writer
	sFileName.Format(_T("%s\\screenshot%d.png"), psc->m_sSaveDirName, psc->m_nIdStartFrom++);
	BOOL bInit = psc->PngInit(nWidth, nHeight, psc->m_bGrayscale, sFileName);
	if(!bInit)
	{
		OutputErrorStr(_T("PngInit failed"));
		goto cleanup;
	}

	// We will get bitmap bits row by row
	nRowWidth	= nWidth*3;
	nRowWidth  += nRowWidth%4;
	pRowBits	= new BYTE[nRowWidth];
	if(NULL == pRowBits)	goto cleanup;

	memset(&bmi.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biSize	= sizeof(BITMAPINFOHEADER); 
	bmi.bmiHeader.biWidth	= nWidth;
	bmi.bmiHeader.biHeight	= -nHeight;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biPlanes	= 1;  

	i = nHeight-1;
	
	while(TRUE)
	{    
		int nFetched = GetDIBits(hCompatDC, hBitmap, i, 1, pRowBits, &bmi, DIB_RGB_COLORS);
		if(nFetched!=1)	break;

		BOOL bWrite = psc->PngWriteRow(pRowBits, nRowWidth, psc->m_bGrayscale);
		if(!bWrite)	goto cleanup;     

		i--;
		if(i < 0) break;
	}

	psc->PngFinalize();
 

	monitor_info.m_rcMonitor = mi.rcMonitor;
	monitor_info.m_sDeviceID = mi.szDevice;
	monitor_info.m_sFileName = sFileName;
	psc->m_monitor_list.push_back(monitor_info);

cleanup:

	// Clean up
	if(hDC)
		DeleteDC(hDC);

	if(hCompatDC)
		DeleteDC(hCompatDC);

	if(hBitmap)
		DeleteObject(hBitmap);

	if(pRowBits)
		delete [] pRowBits;

	// Next monitor
	return TRUE;
}

// Gets rectangle of the virtual screen
void CScreenCapture::GetScreenRect(LPRECT rcScreen)
{
	int nWidth	= GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int nHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	rcScreen->left		= GetSystemMetrics(SM_XVIRTUALSCREEN);
	rcScreen->top		= GetSystemMetrics(SM_YVIRTUALSCREEN);
	rcScreen->right		= rcScreen->left + nWidth;
	rcScreen->bottom	= rcScreen->top + nHeight;
}

BOOL CScreenCapture::PngInit(int nWidth, int nHeight, BOOL bGrayscale, CString sFileName)
{  
	m_fp		= NULL;
	m_png_ptr	= NULL;
	m_info_ptr	= NULL;

	_tfopen_s(&m_fp, sFileName.GetBuffer(0), _T("wb"));

	if (!m_fp)
	{    
		return FALSE;
	}

	m_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 
		(png_voidp)NULL, NULL, NULL);
	if (!m_png_ptr)
		return FALSE;

	m_info_ptr = png_create_info_struct(m_png_ptr);
	if (!m_info_ptr)
	{
		png_destroy_write_struct(&m_png_ptr, (png_infopp)NULL);
		return FALSE;
	}

	/* Error handler*/
	if (setjmp(png_jmpbuf(m_png_ptr)))
	{
		png_destroy_write_struct(&m_png_ptr, &m_info_ptr);
		fclose(m_fp);
		return FALSE;
	}

	png_init_io(m_png_ptr, m_fp);

	/* set the zlib compression level */
	png_set_compression_level(m_png_ptr, Z_BEST_COMPRESSION);

	/* set other zlib parameters */
	png_set_compression_mem_level(m_png_ptr, 8);
	png_set_compression_strategy(m_png_ptr, Z_DEFAULT_STRATEGY);
	png_set_compression_window_bits(m_png_ptr, 15);
	png_set_compression_method(m_png_ptr, 8);
	png_set_compression_buffer_size(m_png_ptr, 8192);

	png_set_IHDR(
		m_png_ptr, 
		m_info_ptr, 
		nWidth, //width, 
		nHeight, //height,
		8, // bit_depth
		bGrayscale?PNG_COLOR_TYPE_GRAY:PNG_COLOR_TYPE_RGB, // color_type
		PNG_INTERLACE_NONE, // interlace_type
		PNG_COMPRESSION_TYPE_DEFAULT, 
		PNG_FILTER_TYPE_DEFAULT);

	png_set_bgr(m_png_ptr);

	/* write the file information */
	png_write_info(m_png_ptr, m_info_ptr);

	return TRUE;
}

BOOL CScreenCapture::PngWriteRow(LPBYTE pRow, int nRowLen, BOOL bGrayscale)
{
	// Convert RGB to BGR
	LPBYTE pRow2 = NULL;

	if(bGrayscale)
	{
		int i;
		pRow2 = new BYTE[nRowLen / 3];
		for(i=0; i < nRowLen / 3; i++)
			pRow2[i] = (pRow[i*3+0] + pRow[i*3+1] + pRow[i*3+2]) / 3;
	}
	else
	{
		pRow2 = pRow;    
	}

	png_bytep rows[1] = { pRow2 };
	png_write_rows(m_png_ptr, (png_bytepp)&rows, 1);

	if(bGrayscale)
		delete [] pRow2;
	return TRUE;
}

BOOL CScreenCapture::PngFinalize()
{
	/* end write */
	png_write_end(m_png_ptr, m_info_ptr);

	/* clean up */
	png_destroy_write_struct(&m_png_ptr, (png_infopp)&m_info_ptr);

	if(m_fp)
		fclose(m_fp);

	return TRUE;
}

BOOL CALLBACK CScreenCapture::EnumWndProc(HWND hWnd, LPARAM lParam)
{
	FindWindowData* pFWD = (FindWindowData*)lParam;

	// Get process ID
	DWORD dwMyProcessId = pFWD->dwProcessId;

	if(IsWindowVisible(hWnd)) // Get only wisible windows
	{
		// Determine the process ID of the current window
		DWORD dwProcessId = 0;
		GetWindowThreadProcessId(hWnd, &dwProcessId);

		// Compare window process ID to our process ID
		if(dwProcessId == dwMyProcessId)
		{     
			DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
			if((dwStyle&WS_CHILD)==0) // Get only non-child windows
			{
				DWORD dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
				WindowInfo wi;
				TCHAR szTitle[1024];
				GetWindowText(hWnd, szTitle, 1024);            
				wi.m_sTitle = szTitle;
				GetWindowRect(hWnd, &wi.m_rcWnd);
				wi.dwStyle = dwStyle;
				wi.dwExStyle = dwExStyle;
				pFWD->paWindows->push_back(wi);

				return TRUE;          
			}
		}
	}

	return TRUE;
}

BOOL CScreenCapture::FindWindows(DWORD dwProcessId, BOOL bAllProcessWindows, std::vector<WindowInfo>* paWindows)
{
	FindWindowData fwd;
	fwd.bAllProcessWindows = bAllProcessWindows;
	fwd.dwProcessId = dwProcessId;
	fwd.paWindows = paWindows;
	EnumWindows(EnumWndProc, (LPARAM)&fwd);

	if(!bAllProcessWindows) 
	{    
		// Find only the main window
		// The main window should have caption, system menu and WS_EX_APPWINDOW style.  
		WindowInfo MainWnd;
		BOOL bMainWndFound = FALSE;

		size_t i;
		for(i = 0; i < paWindows->size(); i++)
		{
			if(((*paWindows)[i].dwExStyle & WS_EX_APPWINDOW) && 
				((*paWindows)[i].dwStyle & WS_CAPTION) && 
				((*paWindows)[i].dwStyle & WS_SYSMENU))
			{      
				// Match!
				bMainWndFound = TRUE;
				MainWnd = (*paWindows)[i];
				break;
			}
		}

		if(!bMainWndFound && paWindows->size()>0)
		{
			MainWnd = (*paWindows)[0];
			bMainWndFound = TRUE;
		}

		if(bMainWndFound)
		{
			paWindows->clear();
			paWindows->push_back(MainWnd);
		}
	}

	return TRUE;
}





