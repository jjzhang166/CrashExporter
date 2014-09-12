
/*! \file  ScreenCap.h
*  \brief  provide a way to capture current screen shot.
*/

#ifndef __SCREENCAP_H__
#define __SCREENCAP_H__

#include "stdafx.h"
#include <vector>

extern "C" {
#include "png.h"
}

/*!
\struct WindowInfo 
\brief
	Window information
*/
struct WindowInfo
{
    CString m_sTitle; //!< Window title
    CRect m_rcWnd;    //!< Window rect
    DWORD dwStyle;
    DWORD dwExStyle;
};

/*!
\struct MonitorInfo 
\brief
	Monitor info
*/
struct MonitorInfo
{
    CString m_sDeviceID; //!< Device ID
    CRect m_rcMonitor;   //!< Monitor rectangle in screen coordinates
    CString m_sFileName; //!< Image file name corresponding to this monitor
};

/*!
\struct ScreenshotInfo 
\brief
	Desktop screen shot info
*/
struct ScreenshotInfo
{
	// Constructor
    ScreenshotInfo()
    {
        m_bValid = FALSE;
    }

    BOOL	m_bValid;				//!< If TRUE, this structure's fields are valid.
	time_t	m_CreateTime;			//!< Time of screenshot capture.
    CRect	m_rcVirtualScreen;		//!< Coordinates of virtual screen rectangle.
    std::vector<MonitorInfo>	m_aMonitors;	//!< The list of monitors captured.
    std::vector<WindowInfo>		m_aWindows;		//!< The list of windows captured.	
};

/*!
\enum SCREENSHOT_TYPE 
\brief
	Screenshot type
*/
enum SCREENSHOT_TYPE
{
	SCREENSHOT_TYPE_VIRTUAL_SCREEN      = 0, //!< Screenshot of entire desktop.
	SCREENSHOT_TYPE_MAIN_WINDOW         = 1, //!< Screenshot of given process' main window.
	SCREENSHOT_TYPE_ALL_PROCESS_WINDOWS = 2  //!< Screenshot of all process windows.
};

/*!
\class CScreenCapture 
\brief
	Desktop screenshot capture
*/
class CScreenCapture
{
public:

    // Constructor.
    CScreenCapture();

	// Destructor.
    ~CScreenCapture();
	
	/*!
	\brief
		Takes desktop screenshot and returns information about it.
	*/
	BOOL TakeDesktopScreenshot(	
			LPCTSTR szSaveToDir,
			ScreenshotInfo& ssi, 
			SCREENSHOT_TYPE type=SCREENSHOT_TYPE_VIRTUAL_SCREEN, 
			DWORD dwProcessId = 0, 
			BOOL bGrayscale=FALSE,
			int nIdStartFrom=0);

private:

	/*! Returns current virtual screen rectangle */
    void GetScreenRect(LPRECT rcScreen);

    /*! Returns an array of visible windows for the specified process or 
     the main window of the process.  */
    BOOL FindWindows(DWORD dwProcessId, BOOL bAllProcessWindows, std::vector<WindowInfo>* paWindows);

    /*! Captures the specified screen area and returns the list of image files */
    BOOL CaptureScreenRect(
        std::vector<CRect> arcCapture, 
        CString sSaveDirName, 
        int nIdStartFrom, 
        BOOL bGrayscale,
        std::vector<MonitorInfo>& monitor_list);

	/*! Monitor enumeration callback. */
	static BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC /*hdcMonitor*/, LPRECT lprcMonitor, LPARAM dwData);

	/*! Window enumeration callback. */
	static BOOL CALLBACK EnumWndProc(HWND hWnd, LPARAM lParam);

    /* PNG management functions */

    /*! Initializes PNG file header */
    BOOL PngInit(int nWidth, int nHeight, BOOL bGrayscale, CString sFileName);

    /*! Writes a scan line to the PNG file */
    BOOL PngWriteRow(LPBYTE pRow, int nRowLen, BOOL bGrayscale);

    // Closes PNG file
    BOOL PngFinalize();

	/*!
	\struct FindWindowData 
	\brief
		The following structure stores window find data.
	*/
	struct FindWindowData
	{
		DWORD	dwProcessId;					//!< Process ID.
		BOOL	bAllProcessWindows;             //!< If TRUE, finds all process windows, else only the main one
		std::vector<WindowInfo>* paWindows;		//!< Output array of window handles
	};

    /* Internal member variables. */

    CPoint				m_ptCursorPos;      //!< Current mouse cursor pos
    std::vector<CRect>	m_arcCapture;		//!< Array of capture rectangles
    CURSORINFO			m_CursorInfo;       //!< Cursor info  
    int					m_nIdStartFrom;     //!< An ID for the current screenshot image 
    CString				m_sSaveDirName;     //!< Directory name to save screenshots toy
    BOOL				m_bGrayscale;       //!< Create grayscale image or not
    FILE*				m_fp;               //!< Handle to the file
    png_structp			m_png_ptr;          //!< libpng stuff
    png_infop			m_info_ptr;         //!< libpng stuff
    std::vector<MonitorInfo> m_monitor_list; //!< The list of monitor devices   
};

#endif //__SCREENCAP_H__


