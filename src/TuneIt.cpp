// TuneIt.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "TuneIt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTuneItApp

BEGIN_MESSAGE_MAP(CTuneItApp, CWinApp)
	//{{AFX_MSG_MAP(CTuneItApp)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTuneItApp construction

CTuneItApp::CTuneItApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTuneItApp object

CTuneItApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CTuneItApp initialization

BOOL CTuneItApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically  
#endif

    // TraceStart ();

    mainWin.Initialize ();
    m_pMainWnd = &mainWin;

    // Continue with message loop
    //
	return TRUE;
    }

