// CFFTWnd : implementation file
//

#include "StdAfx.h"
#include "TuneIt.h"
#include "Resource.h"

#include <math.h>
#include <float.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFFTWnd

CFFTWnd::CFFTWnd( void )
{
    Trace( "CFFTWnd::CFFTWnd()\n" );

    cpuUsage_process.SetProcess( GetCurrentProcess () );
    cpuUsage_display.SetThread( GetCurrentThread () );

    waveIn.SetParent( this );
    waveOut.SetParent( this );

    displayFreq = DISPLAY_FREQ_LOG;
    displayMagLog = true;
    displayFFT = true;
    displayPeakInfo = true;
    displayWaterfall = false;
    freezeWaterfall = false;
    specStartY = 25;
    specDeltaY = 1;
    specStartX = 12;

    fMoving = FALSE;
    m_hAccelTable = NULL;

    whiteBackground = false;

    if ( whiteBackground )
    {
        colorBackground      = RGB( 0xFF, 0xFF, 0xE0 );

        colorTextDefault     = RGB( 0x00, 0x00, 0x00 );
        colorTextHighlighted = RGB( 0x00, 0x00, 0xFF );
        colorTextDarkYellow  = RGB( 0x80, 0x80, 0x00 );
        colorTextGray        = RGB( 0x80, 0x80, 0x80 );
        colorTextCursor      = RGB( 0x50, 0x50, 0x30 );
        colorTextRed         = RGB( 0xE0, 0x00, 0x00 );
        colorTextCutoff      = RGB( 0x80, 0x80, 0x00 );

        colorGraphBackground = RGB( 0xFF, 0xFF, 0xFF );
        colorGraphGrid       = RGB( 0xB0, 0xB0, 0xB0 );
        colorGraphGridSub    = RGB( 0xE0, 0xE0, 0xE0 );
        colorGraphGridNote   = RGB( 0xF0, 0xF0, 0xF0 );
        colorGraphPkWindow   = RGB( 0xE0, 0xFF, 0xE0 );
        colorGraphPkMarks    = RGB( 0x00, 0xA0, 0x00 );
        colorGraphCents      = RGB( 0x60, 0x60, 0x00 );
        colorGraphCentsFrame = RGB( 0x50, 0xA0, 0xA0 );
        colorGraphCentsErr   = RGB( 0xFF, 0x00, 0x00 );
        colorGraphSelection  = RGB( 0xA0, 0xA0, 0xA0 );

        colorGraphSpectrAvg  = RGB( 0xE0, 0x00, 0x00 );
        colorGraphSpectrTop  = RGB( 0xFF, 0xE0, 0xE0 );
        colorGraphSpectrBot  = RGB( 0xFF, 0xE0, 0xE0 );
        colorGraphChromAvg   = RGB( 0xE0, 0x00, 0x00 );
        }
    else
    {
        colorBackground      = RGB( 0x00, 0x00, 0x30 );

        colorTextDefault     = RGB( 0x00, 0xA0, 0xA0 );
        colorTextHighlighted = RGB( 0xFF, 0xFF, 0xFF );
        colorTextDarkYellow  = RGB( 0x80, 0x80, 0x00 );
        colorTextGray        = RGB( 0x80, 0x80, 0x80 );
        colorTextCursor      = RGB( 0xE0, 0xE0, 0x80 );
        colorTextRed         = RGB( 0xFF, 0x00, 0x00 );
        colorTextCutoff      = RGB( 0xFF, 0xFF, 0x00 );

        colorGraphBackground = RGB( 0x00, 0x00, 0x00 );
        colorGraphGrid       = RGB( 0x00, 0x40, 0x40 );
        colorGraphGridSub    = RGB( 0x00, 0x30, 0x30 );
        colorGraphGridNote   = RGB( 0x00, 0x20, 0x30 );
        colorGraphPkWindow   = RGB( 0x40, 0x50, 0x00 );
        colorGraphPkMarks    = RGB( 0x00, 0xFF, 0x00 );
        colorGraphCents      = RGB( 0xFF, 0xFF, 0x00 );
        colorGraphCentsFrame = RGB( 0x30, 0x70, 0x70 );
        colorGraphCentsErr   = RGB( 0xFF, 0x00, 0x00 );
        colorGraphSelection  = RGB( 0xA0, 0xA0, 0xA0 );

        colorGraphSpectrAvg  = RGB( 0xE0, 0x00, 0x00 );
        colorGraphSpectrTop  = RGB( 0x50, 0x10, 0x00 );
        colorGraphSpectrBot  = RGB( 0x40, 0x00, 0x00 );
        colorGraphChromAvg   = RGB( 0xE0, 0x00, 0x00 );
        }

    penGrid       .CreatePen( PS_SOLID, 0, colorGraphGrid      );
    penGridSub    .CreatePen( PS_SOLID, 0, colorGraphGridSub   );
    penGridNote   .CreatePen( PS_SOLID, 0, colorGraphGridNote  );
    penGridNoteC  .CreatePen( PS_SOLID, 0, colorBackground     );
    penGridCutOff .CreatePen( PS_SOLID, 0, colorTextDarkYellow );
    penPeakLine   .CreatePen( PS_SOLID, 0, colorGraphPkMarks   );
    penSpectrAvg  .CreatePen( PS_SOLID, 0, colorGraphSpectrAvg );
    penSpectrTop  .CreatePen( PS_SOLID, 2, colorGraphSpectrTop );
    penSpectrBot  .CreatePen( PS_SOLID, 0, colorGraphSpectrBot );
    penChromAvg   .CreatePen( PS_SOLID, 0, colorGraphChromAvg  );
    }

void CFFTWnd::DestroyFFT( void )
{
    waveOut.TurnOffMeasureFFT ();
    waveIn.TurnOffMeasureFFT ();

    waveOut.SetGenerator ();

    if ( ! waveIn.StopThread () )
        waveIn.KillThread ();

    for ( int i = 0; i < 100 && waveOut.GetPlayingFreq () != 0.0; i++ )
        Sleep( 10 );

    if ( ! waveOut.StopThread () )
        waveOut.KillThread ();

    waveIn.WaitThread ();

    waveOut.WaitThread ();

    fft.Destroy ();
    }

CFFTWnd::~CFFTWnd()
{
    Trace( "CFFTWnd::~CFFTWnd ()\n" );
    DestroyFFT ();
    }


BEGIN_MESSAGE_MAP(CFFTWnd, CWnd)
	//{{AFX_MSG_MAP(CFFTWnd)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEWHEEL()
	ON_WM_DESTROY()
	ON_COMMAND(IDM_PLAY_TONE, OnPlayTone)
	ON_COMMAND(IDM_TOGGLE_VIEW, OnViewFreqToggle)
	ON_COMMAND(IDM_TUNE_VIOLIN_G, OnTuneViolinG)
	ON_COMMAND(IDM_TUNE_VIOLIN_D, OnTuneViolinD)
	ON_COMMAND(IDM_TUNE_VIOLIN_A, OnTuneViolinA)
	ON_COMMAND(IDM_TUNE_VIOLIN_E, OnTuneViolinE)
	ON_COMMAND(IDM_STOP_PLAYING, OnStopPlaying)
	ON_COMMAND(IDM_TUNE_AUTO, OnTuneAuto)
	ON_COMMAND(IDM_OCTAVE_UP, OnOctaveUp)
	ON_COMMAND(IDM_OCTAVE_DOWN, OnOctaveDown)
	ON_COMMAND(IDM_MEASURE_WAVE_IN, OnMeasureWaveIn)
	ON_COMMAND(IDM_MEASURE_WAVE_OUT, OnMeasureWaveOut)
	ON_COMMAND(IDM_CLEAR_LEARNED_NOISE, OnClearLearnedNoise)
	ON_COMMAND(IDM_START_LEARNING_NOISE, OnStartLearningNoise)
	ON_COMMAND(IDM_VIEW_CHROMATIC, OnViewChromatic)
	ON_COMMAND(IDM_VIEW_FREQ_LOG, OnViewFreqLog)
	ON_COMMAND(IDM_VIEW_FREQ_LIN, OnViewFreqLin)
	ON_WM_CAPTURECHANGED()
	ON_COMMAND(IDM_VIEW_LOG_TOGGLE, OnViewLogToggle)
	ON_WM_SETCURSOR()
	ON_WM_GETMINMAXINFO()
	ON_COMMAND(IDM_VIEW_ZOOM_OUT, OnViewZoomOut)
	ON_COMMAND(IDM_TOGGLE_PK_WIN, OnTogglePkWin)
	ON_COMMAND(IDM_TOGGLE_PLAYING, OnTogglePlaying)
    ON_COMMAND(IDM_TOGGLE_PLAYING, OnToggleTopMost)
	ON_WM_SYSCOMMAND()
	ON_COMMAND(IDM_VIEW_WATERFALL_TOGGLE, OnViewWaterfallToggle)
	ON_COMMAND(IDM_TOGGLE_TRACE_WINDOW, OnToggleTraceWindow)
	ON_COMMAND(IDM_FREEZE_WATERFALL, OnFreezeWaterfall)
	ON_COMMAND(IDM_WATERFALL_DEC_SPEED, OnWaterfallDecSpeed)
	ON_COMMAND(IDM_WATERFALL_INC_SPEED, OnWaterfallIncSpeed)
	ON_COMMAND(IDM_DISPLAY_FFT, OnDisplayFft)
	ON_COMMAND(IDM_DISPLAY_PEAK_INFO, OnDisplayPeakInfo)
	ON_COMMAND(IDM_FFTWIN_RECTANGULAR, OnFftwinRectangular)
	ON_COMMAND(IDM_FFTWIN_HANNING, OnFftwinHanning)
	ON_COMMAND(IDM_FFTWIN_BLACKMAN_HARRIS, OnFftwinBlackmanHarris)
	ON_COMMAND(IDM_FFTWIN_KAISER_BESSEL, OnFftwinKaiserBessel)
	ON_COMMAND(IDM_CURSOR_UP, OnCursorUp)
	ON_COMMAND(IDM_CUROSR_DOWN, OnCurosrDown)
	ON_COMMAND(IDM_CURSOR_LEFT, OnCursorLeft)
	ON_COMMAND(IDM_CURSOR_RIGHT, OnCursorRight)
	ON_COMMAND(IDM_CURSOR_UP10, OnCursorUp10)
	ON_COMMAND(IDM_CUROSR_DOWN10, OnCurosrDown10)
	ON_COMMAND(IDM_CURSOR_LEFT10, OnCursorLeft10)
	ON_COMMAND(IDM_CURSOR_RIGHT10, OnCursorRight10)
	ON_COMMAND(IDM_TEST_BUTTON, OnTestButton)
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFFTWnd message handlers

///////////////////////////////////////////////////////////////////////////////

BOOL CFFTWnd:: Initialize( void )
{
    int defWidth = 1024;
    int defHeight = 640;

    extern CTuneItApp theApp;
	m_hAccelTable = LoadAccelerators( theApp.m_hInstance, (LPCTSTR)IDC_ACCELERATOR);

    // Get the size of the screen.
    //
    HDC hdcScreen = ::GetDC( NULL );
    int iXRes = GetDeviceCaps(hdcScreen, HORZRES);
    int iYRes = GetDeviceCaps(hdcScreen, VERTRES);
    
    CString myWndClass = AfxRegisterWndClass(
        CS_DBLCLKS,               // clas style
        NULL, // cursor handle
        0,               // brush handle
	    AfxGetApp()->LoadIcon( IDR_MAINFRAME )  // icon handle
        );

    defWidth = iXRes < defWidth ? iXRes : defWidth;
    defHeight = iYRes < defHeight ? iYRes : defHeight;

    CString appTitle = 
        "Audio Spectrum Analyzer & Tuner, "
        "Free software released under GNU GPL "
        "- Copyright (c) 2007 by M.B.Kocic";

    BOOL rc = CreateEx(
        0, // extended window style
        myWndClass,      // window class name
        appTitle,     // window name
        WS_POPUPWINDOW | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX
            | WS_VISIBLE | WS_CAPTION, // window style
        ( iXRes - defWidth ) / 2, // x
        ( iYRes - defHeight ) / 2, // y
        defWidth, // width
        defHeight, // height
        NULL,            // parent window handle
        NULL,            // menu handle
        NULL             // parameter
        );

    if ( ! rc )
        return rc;

    ///////////////////////////////////////////////////////////////////////////
    LOGFONT lf = { 0 };

    switch( 0 )
    {
        case 1:
            lf.lfHeight = 20;
            strcpy(lf.lfFaceName, "Courier New");
            break;
        case 2:
            lf.lfHeight = 18;
            strcpy(lf.lfFaceName, "Bitstream Vera Sans Mono");
            break;
        default:
            lf.lfHeight = 16;
            strcpy(lf.lfFaceName, "Lucida Console");
            break;
        }

    fontText.CreateFontIndirect( &lf );

    ///////////////////////////////////////////////////////////////////////////

    lf.lfWeight = FW_BOLD; 
    fontTextBold.CreateFontIndirect( &lf );

    ///////////////////////////////////////////////////////////////////////////

    lf.lfHeight = 16;
    lf.lfWeight = FW_NORMAL;
    strcpy(lf.lfFaceName, "Verdana");
    fontScale.CreateFontIndirect( &lf );

    ///////////////////////////////////////////////////////////////////////////
    // Music Symbols

    lf.lfHeight = 40;
    strcpy(lf.lfFaceName, "Opus Text");
    fontMusic.CreateFontIndirect( &lf );
    strFlat = "b";
    strSharp = "#";

    bmpFlat.LoadBitmap( IDB_BITMAP_FLAT );
    bmpSharp.LoadBitmap( IDB_BITMAP_SHARP );

    ///////////////////////////////////////////////////////////////////////////
    // Control Buttons

    int x = 5;
    int y = 5;
    int w = 0;

    x += 2 + w; w =  45; btnViewFreqLog  .Create( "X-Log",   this, x, y, w, 20, IDM_VIEW_FREQ_LOG );
    x += 2 + w; w =  45; btnViewFreqLin  .Create( "X-Lin",   this, x, y, w, 20, IDM_VIEW_FREQ_LIN );
    x += 2 + w; w =  45; btnViewFreqChrom.Create( "Chrom",   this, x, y, w, 20, IDM_VIEW_CHROMATIC );
    x += 10;
    x += 2 + w; w =  45; btnViewLog      .Create( "Y-Log",   this, x, y, w, 20, IDM_VIEW_LOG_TOGGLE );
    x += 10;
    x += 2 + w; w =  40; btnViewWaterfall.Create( "WFL",     this, x, y, w, 20, IDM_VIEW_WATERFALL_TOGGLE );
    x += 2 + w; w =  40; btnViewPkWin    .Create( "PkW",     this, x, y, w, 20, IDM_TOGGLE_PK_WIN );
    x += 15;
    x += 2 + w; w =  25; btnTuneG        .Create( "G",       this, x, y, w, 20, IDM_TUNE_VIOLIN_G );
    x += 2 + w; w =  25; btnTuneD        .Create( "D",       this, x, y, w, 20, IDM_TUNE_VIOLIN_D );
    x += 2 + w; w =  25; btnTuneA        .Create( "A",       this, x, y, w, 20, IDM_TUNE_VIOLIN_A );
    x += 2 + w; w =  25; btnTuneE        .Create( "E",       this, x, y, w, 20, IDM_TUNE_VIOLIN_E );
    x += 2 + w; w =  50; btnTuneAuto     .Create( "Auto",    this, x, y, w, 20, IDM_TUNE_AUTO   );
    x += 10;
    x += 2 + w; w =  50; btnPlayTone     .Create( "Play",    this, x, y, w, 20, IDM_TOGGLE_PLAYING );
    x += 2 + w; w =  50; btn8vaMinus     .Create( "8va-1",   this, x, y, w, 20, IDM_OCTAVE_DOWN );
    x += 2 + w; w =  50; btn8vaPlus      .Create( "8va+1",   this, x, y, w, 20, IDM_OCTAVE_UP );
    x += 10;
    x += 2 + w; w =  60; btnNoiseLearn   .Create( "Learn N", this, x, y, w, 20, IDM_START_LEARNING_NOISE );
    x += 2 + w; w =  60; btnNoiseClear   .Create( "Noise F", this, x, y, w, 20, IDM_CLEAR_LEARNED_NOISE );
    x += 10;
    x += 2 + w; w =  35; btnDebug        .Create( "DBG",     this, x, y, w, 20, IDM_TOGGLE_TRACE_WINDOW );
    x += 10;

	// Add Top Most menu item to system menu
    //
	ASSERT((IDM_TOGGLE_TOP_MOST & 0xFFF0) == IDM_TOGGLE_TOP_MOST);
	ASSERT(IDM_TOGGLE_TOP_MOST < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
        pSysMenu->AppendMenu(MF_SEPARATOR);
        pSysMenu->AppendMenu(MF_STRING | MF_UNCHECKED, IDM_TOGGLE_TOP_MOST, "&Top Most" );
	    }

    ///////////////////////////////////////////////////////////////////////////
    // Intialize FFT

    fft.Initialize( 48000, 16384, 4096, "A", 4, 440.0 );

    ///////////////////////////////////////////////////////////////////////////
    // Intialize Display

    curFreq = 0.0;
    curMagDb = 0.0;

    maxMagDb = 0.0;
    maxMagLin = Db2Lin( maxMagDb );
    minMagDb = -91.0;

    minFreqK = fft.F2K( fft.pkwin.f1 );
    maxFreqK = fft.F2K( fft.pkwin.f2 );

    kLen = maxFreqK - minFreqK + 1;
    logMinFreq = minFreqK == 0 ? 0.0 : log( fft.K2F( minFreqK ) );
    logFreqRange = log( fft.K2F( maxFreqK ) ) - logMinFreq;

    ///////////////////////////////////////////////////////////////////////////

    // TraceStart ();

    ///////////////////////////////////////////////////////////////////////////
    // Start audio input/output, FFT and FFT display

    // Audio input
    //
    waveIn.StartThread ();

    // Audio output
    //
    waveOut.StartThread ();

    // Timer
    //
    SetTimer( 0, 20, NULL ); // 50 Hz

    ::SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );

    // Start FFT
    //
    fft.StartThread ();

    // Update Display
    CPoint point;
    GetCursorPos( &point );
    ScreenToClient( &point );
    OnSetCursorPosition( point );

    OnData_Changed ();
    OnUI_Changed ();

    return rc;
    }

void CFFTWnd::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
    lpMMI->ptMinTrackSize.x = 800;
    lpMMI->ptMinTrackSize.y = 480;
	
	CWnd::OnGetMinMaxInfo(lpMMI);
}

void CFFTWnd::OnUI_Changed ()
{
    fft.mutex.Lock ();

    btnViewWaterfall.SetHighlighted( displayWaterfall );

    btnViewFreqLog.SetHighlighted( displayFreq == DISPLAY_FREQ_LOG );
    btnViewFreqLin.SetHighlighted( displayFreq == DISPLAY_FREQ_LIN );
    btnViewFreqChrom.SetHighlighted( displayFreq == DISPLAY_CHROMATIC );

    btnViewLog.SetHighlighted( displayMagLog );

    btnViewPkWin.SetHighlighted( fft.pkwin.active );

    btnTuneG.SetHighlighted( fft.refNoteLocked && stricmp( fft.noteName[ fft.refNote ], "G" ) == 0 );
    btnTuneD.SetHighlighted( fft.refNoteLocked && stricmp( fft.noteName[ fft.refNote ], "D" ) == 0 );
    btnTuneA.SetHighlighted( fft.refNoteLocked && stricmp( fft.noteName[ fft.refNote ], "A" ) == 0 );
    btnTuneE.SetHighlighted( fft.refNoteLocked && stricmp( fft.noteName[ fft.refNote ], "E" ) == 0 );
    btnTuneAuto.SetHighlighted( ! fft.refNoteLocked );

    btnNoiseLearn.SetHighlighted( fft.noiseFilter == FFT::NOISEF_LEARNING );
    btnNoiseClear.SetHighlighted( fft.noiseFilter == FFT::NOISEF_ENABLED );

    btnPlayTone.SetHighlighted( waveOut.GetPlayingFreq () > 0 );

    btnDebug.SetHighlighted( IsTraceStarted () );

    fft.mutex.Unlock ();
    }

void CFFTWnd::OnData_Changed ()
{
    fft.mutex.Lock ();

    CRect rect = rectClient;
    rect.top = rectHeader.top;

    static FFT::NOISE_FILTER lastNoiseFilter = FFT::NOISEF_DISABLED;

    if ( fft.noiseFilter != lastNoiseFilter )
    {
        lastNoiseFilter = fft.noiseFilter;

        if ( fft.noiseFilter == FFT::NOISEF_ENABLED )
        {
            if ( displayMagLog )
                maxMagDb = 0.0;
            else
                maxMagDb = fft.cutOffMagDb;
            maxMagLin = Db2Lin( maxMagDb );

            if ( ! fft.pkwin.active )
            {
                fft.pkwin.m1 = fft.cutOffMagDb;
                fft.pkwin.m2 = 0.0;
                }
            }

        OnUI_Changed ();
        }

    fft.mutex.Unlock ();

    InvalidateRect( &rect, FALSE );
    }

BOOL CFFTWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
    if ( nHitTest == HTCLIENT )
    {
        CPoint point;
        GetCursorPos( &point );
        ScreenToClient( &point );

        if ( rectXY.PtInRect( point ) )
        {
            SetCursor( AfxGetApp()->LoadCursor( IDC_POINTER_CROSS ) );
            }
        else
        {
            SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
            }

        return TRUE;
        }

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CFFTWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	// only paint the rect that needs repainting
    //
	CRect client;
	dc.GetClipBox( client );

    client.NormalizeRect ();
    client.InflateRect( 1, 1 );

	CRect rect = client;
	dc.LPtoDP( &rect );
    rect.NormalizeRect ();

	CBitmap* pOldBitmap;

	CDC dc2;
	CDC* pDrawDC = &dc;

    // Keep offscreen bitmap having the same width/height as screen
    //
    if ( HBITMAP( bmpDC ) )
    {
        BITMAP bmpb;
        bmpDC.GetBitmap( &bmpb );

        if ( bmpb.bmWidth     != dc.GetDeviceCaps( HORZRES )
          || bmpb.bmHeight    != dc.GetDeviceCaps( VERTRES ) 
          || bmpb.bmBitsPixel != dc.GetDeviceCaps( BITSPIXEL ) 
          || bmpb.bmPlanes    != dc.GetDeviceCaps( PLANES ) 
        )
            bmpChromWaterfall.DeleteObject ();
        }

    if ( ! HBITMAP( bmpDC ) )
    {
        bmpDC.CreateCompatibleBitmap( &dc, 
            dc.GetDeviceCaps( HORZRES ), dc.GetDeviceCaps( VERTRES ) );
        }

	// Draw to offscreen bitmap for fast looking repaints
    //
	if ( HBITMAP( bmpDC ) && dc2.CreateCompatibleDC( &dc ) )
	{
		pDrawDC = &dc2;

		// offset origin more because bitmap is just piece of the whole drawing
        //
		dc2.OffsetViewportOrg( - rect.left, - rect.top );
		pOldBitmap = dc2.SelectObject( &bmpDC );

        dc2.SetBrushOrg( rect.left % 8, rect.top % 8 );

		// might as well clip to the same rectangle
        //
		dc2.IntersectClipRect( client );
		}

    // Solid Background
    //
    pDrawDC->FillSolidRect( &rect, colorBackground );

    fft.mutex.Lock ();

    // Draw document
    //
    GetClientRect( &client );
    OnClientRectChanged( client );

    OnPaintHeader( *pDrawDC );
    OnPaintFooter( *pDrawDC );

    OnPaintVUMeter( *pDrawDC );

    switch ( displayFreq )
    {
        case DISPLAY_CHROMATIC:
            OnPaintChromatic( *pDrawDC );
            break;
        case DISPLAY_FREQ_LOG:
        case DISPLAY_FREQ_LIN:
            OnPaintSpectral( *pDrawDC );
            break;
        }
    OnPaintCursorInfo( *pDrawDC );

    fft.mutex.Unlock ();

	if ( pDrawDC != &dc )
	{
		dc.SetViewportOrg( 0, 0 );
		dc.SetWindowOrg( 0, 0 );
		dc.SetMapMode( MM_TEXT );

		dc2.SetViewportOrg( 0, 0 );
		dc2.SetWindowOrg( 0,0 );
		dc2.SetMapMode( MM_TEXT );

		dc.BitBlt( 
            rect.left, rect.top, rect.Width(), rect.Height(), 
            &dc2, 0, 0, SRCCOPY );

		dc2.SelectObject( pOldBitmap );
	    }
    }

void CFFTWnd::OnTimer(UINT nIDEvent) 
{
    if ( nIDEvent != 0 )
        return;

    OnData_Changed ();
    }

void CFFTWnd::OnClientRectChanged( CRect& rect )
{
    fft.mutex.Lock ();

    rect.NormalizeRect ();

    rectClient = rect;

    rectHeader = rect;
    rectHeader.top += 30;
    rectHeader.left += 20;
    rectHeader.right -= 20;
    rectHeader.bottom = rectHeader.top + 75;

    rectFooter = rect;
    rectFooter.bottom -= 5;
    rectFooter.top = rectFooter.bottom - 30;
    rectFooter.left += 20;
    rectFooter.right -= 20;

    rectGraph = rect;
    rectGraph.left += 20;
    rectGraph.right -= 20;
    rectGraph.top = rectHeader.bottom;
    rectGraph.bottom = rectFooter.top - 5;

    rectScale = rectGraph;
    rectScale.left += 20;
    rectScale.bottom -= 20;

    rectXY = rectScale;
    rectXY.right -= 12;

    if ( displayFreq == DISPLAY_CHROMATIC )
    {
        rectScale.top += 20;
        rectXY.top += 20;
        }

    if ( displayWaterfall )
    {
        rectXY.top += specStartY;
        rectXY.left += specStartX;
        }

    fft.mutex.Unlock ();
    }

void CFFTWnd::OnSize(UINT nType, int cx, int cy) 
{
    fft.mutex.Lock ();

    CRect rect;
    GetClientRect( &rect );
    OnClientRectChanged( rect );

    OnData_Changed ();

    fft.mutex.Unlock ();

	CWnd::OnSize(nType, cx, cy);
    }

void CFFTWnd::OnSizing(UINT fwSide, LPRECT pRect) 
{
    fft.mutex.Lock ();

    CRect rect;
    GetClientRect( &rect );
    OnClientRectChanged( rect );

    OnData_Changed ();

    fft.mutex.Unlock ();

	CWnd::OnSizing(fwSide, pRect);
    }

void CFFTWnd::OnMouseMove(UINT nFlags, CPoint point) 
{
    fft.mutex.Lock ();

    OnSetCursorPosition( point );

    if ( fMoving && ! selectionActive )
    {
        CRect rect = rectGraph;
        rect.top = rectXY.bottom;

        if ( rect.PtInRect( startp ) )
        {
            // Pan by keeping scale and moving it left/right
            //
            short z = short( point.x - pan_point.x ) * 40;

            OnMouseWheel( MK_SHIFT, z, point );

            pan_point = point;
            }
        }

    fft.mutex.Unlock ();
    }

void CFFTWnd::OnLButtonDown(UINT nFlags, CPoint point) 
{
    fft.mutex.Lock ();

    CRect rect;
    GetClientRect( &rect );
    OnClientRectChanged( rect );

    if ( rectGraph.PtInRect( point ) )
    {
        fMoving = TRUE;
        SetCapture ();
        startp = point;
        pan_point = point;

        OnSetCursorPosition( point );

        if ( rectXY.PtInRect( point ) )
        {
            if ( displayFreq == DISPLAY_FREQ_LOG || displayFreq == DISPLAY_FREQ_LIN )
            {
                SetSelectionStart ();
                }
            else if ( displayFreq == DISPLAY_CHROMATIC )
            {
                fft.SetRefNote( xChrom_to_Note( point.x ) );

                if ( waveOut.GetPlayingFreq () != fft.refFreq )
                    waveOut.SetGenerator( fft.refFreq, MAG_minus3dB );
                else
                    waveOut.SetGenerator ();
                }
            }

        OnUI_Changed ();
        OnData_Changed ();
        }

    fft.mutex.Unlock ();
    }

void CFFTWnd::OnLButtonUp(UINT nFlags, CPoint point) 
{
    fft.mutex.Lock ();

    if ( ! fMoving )
    {
        fft.mutex.Unlock ();
        return;
        }

    CRect rect;
    GetClientRect( &rect );
    OnClientRectChanged( rect );

    fMoving = FALSE;
    ReleaseCapture ();

    if ( rectXY.PtInRect( startp ) )
    {
        if ( displayFreq == DISPLAY_FREQ_LOG || displayFreq == DISPLAY_FREQ_LIN )
        {
            if ( point.x != startp.x && point.y != startp.y )
            {
                OnSetCursorPosition( point );
                SetZoomWindow ();
                }
            }
        }

    selectionActive = false;

    OnUI_Changed ();
    OnData_Changed ();

    fft.mutex.Unlock ();
    }

void CFFTWnd::OnCaptureChanged(CWnd *pWnd) 
{
    fft.mutex.Lock ();

    fMoving = FALSE;
    ReleaseCapture ();

    fft.mutex.Unlock ();

	CWnd::OnCaptureChanged(pWnd);
}

void CFFTWnd::OnRButtonDown(UINT nFlags, CPoint point) 
{
    fft.mutex.Lock ();

    if ( rectGraph.PtInRect( point ) )
    {
        fMoving = TRUE;
        SetCapture ();
        startp = point;

        OnSetCursorPosition( point );

        if ( rectXY.PtInRect( point ) )
        {
            if ( displayFreq == DISPLAY_FREQ_LOG || displayFreq == DISPLAY_FREQ_LIN )
            {
                SetSelectionStart ();
                OnUI_Changed ();
                OnData_Changed ();
                }
            }
        }

    fft.mutex.Unlock ();
    }

void CFFTWnd::OnRButtonUp(UINT nFlags, CPoint point) 
{
    fft.mutex.Lock ();

    if ( ! fMoving )
    {
        fft.mutex.Unlock ();
        return;
        }

    CRect rect;
    GetClientRect( &rect );
    OnClientRectChanged( rect );

    ReleaseCapture ();
    fMoving = FALSE;

    OnSetCursorPosition( point );

    if ( rectXY.PtInRect( startp ) )
    {
        if ( point.x != startp.x && point.y != startp.y )
        {
            if ( displayFreq == DISPLAY_FREQ_LOG || displayFreq == DISPLAY_FREQ_LIN  )
            {
                SetPkWindow ();
                }
            }
        else
        {
            selectionActive = false;
            SetCutOffMagAbs ();
            }
        }

    OnUI_Changed ();
    OnData_Changed ();

    fft.mutex.Unlock ();
    }

void CFFTWnd::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
    fft.mutex.Lock ();

    OnSetCursorPosition( point );

    if ( fft.pkwin.active 
        && fft.pkwin.f1 <= curFreq && curFreq <= fft.pkwin.f2 
        && fft.pkwin.m1 <= curMagDb && curMagDb <= fft.pkwin.m2 
        )
    {
        minFreqK = fft.F2K( fft.pkwin.f1 );
        maxFreqK = fft.F2K( fft.pkwin.f2 ) - 1;

        if ( maxFreqK - minFreqK < 3 )
        {
            minFreqK = fft.F2K( 10.0  );
            maxFreqK = fft.F2K( 20.0e3 );
            }

        kLen = maxFreqK - minFreqK + 1;
        logMinFreq = minFreqK == 0 ? 0.0 : log( fft.K2F( minFreqK ) );
        logFreqRange = log( fft.K2F( maxFreqK ) ) - logMinFreq;
/*
        maxMagLin = Db2Lin( fft.pkwin.m2 );
        maxMagDb = fft.pkwin.m2;
        minMagDb = fft.pkwin.m1;
*/
        }
    else
    {
        OnViewZoomOut ();
        }

    fft.mutex.Unlock ();
    }

BOOL CFFTWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint point) 
{
    if ( displayFreq == DISPLAY_CHROMATIC )
        return TRUE;

    fft.mutex.Lock ();

    ScreenToClient( &point );

    double z = double(zDelta) / 120.0;

    OnSetCursorPosition( point );

    double f1 = fft.K2F( minFreqK );
    double f2 = fft.K2F( maxFreqK );

    if ( displayFreq == DISPLAY_FREQ_LOG )
    {
        if ( nFlags & MK_SHIFT ) // Pan
        {
            double delta = - z * 0.01;

            double f1n = f1 * pow( f2 / f1, delta );
            double f2n = f1 * pow( f2 / f1, 1.0 + delta );

            f1 = f1n;
            f2 = f2n;
            }
        else // Zoom
        {
            double delta = 1.0 / ( 1.0 + z * 0.1 );

            f1 = curFreq / pow( curFreq / f1, delta );
            f2 = curFreq / pow( curFreq / f2, delta );
            }
        }
    else if ( displayFreq == DISPLAY_FREQ_LIN )
    {
        if ( nFlags & MK_SHIFT ) // Pan
        {
            double delta = - z * 0.01 * ( f2 - f1 );

            if ( f1 + delta < 10.0 )
                delta = 10.0 - f1;
            if ( f2 + delta > 20.0e3 )
                delta = 20.0e3 - f2;

            f1 += delta;
            f2 += delta;
            }
        else // Zoom
        {
            double delta = 1.0 / ( 1.0 + z * 0.1 );

            f1 = curFreq - delta * ( curFreq - f1 );
            f2 = curFreq - delta * ( curFreq - f2 );
            }
        }

    if ( f1 < 10.0 || f1 > 20.0e3 )
        f1 = 10.0;
    if ( f2 < 10.0 || f2 > 20.0e3 )
        f2 = 20.0e3;

    if ( fft.F2K( f2 - f1 ) < 10 )
    {
        fft.mutex.Unlock ();
        return TRUE;
        }

    minFreqK = fft.F2K( f1 );
    maxFreqK = fft.F2K( f2 );

    if ( maxFreqK - minFreqK < 3 )
    {
        minFreqK = fft.F2K( 10.0 );
        maxFreqK = fft.F2K( 20.0e3 );
        }

    kLen = maxFreqK - minFreqK + 1;
    logMinFreq = minFreqK == 0 ? 0.0 : log( fft.K2F( minFreqK ) );
    logFreqRange = log( fft.K2F( maxFreqK ) ) - logMinFreq;

    Trace( "Wheel: %+5.1lf: F %7.1lf %7.1lf -> K %5d %5d\n", z, f1, f2, minFreqK, maxFreqK );

    OnSetCursorPosition( point );

    fft.mutex.Unlock ();

    return TRUE;
    }

void CFFTWnd::OnDestroy() 
{
    fft.mutex.Lock ();

    DestroyFFT ();

    fft.mutex.Unlock ();

	CWnd::OnDestroy();
    }

BOOL CFFTWnd::PreTranslateMessage(MSG* pMsg) 
{
    if ( m_hAccelTable ) 
    {
       if ( ::TranslateAccelerator( m_hWnd, m_hAccelTable, pMsg ) ) 
       {
           return TRUE;
           }
        }
	
	return CWnd::PreTranslateMessage(pMsg);
}

void CFFTWnd::OnPlayTone() 
{
    fft.mutex.Lock ();

    CRect rect;
    GetClientRect( &rect );
    OnClientRectChanged( rect );

    CPoint point;
    GetCursorPos( &point );
    ScreenToClient( &point );
    OnSetCursorPosition( point );

    if ( ! rect.PtInRect( point ) )
    {
        waveOut.SetGenerator ();
        OnUI_Changed ();
        }

    else if ( rectGraph.PtInRect( point ) )
    {
        double playing_freq = waveOut.GetPlayingFreq ();

        if ( playing_freq != curFreq )
            waveOut.SetGenerator( curFreq, MAG_minus3dB );
        else
            waveOut.SetGenerator ();

        OnUI_Changed ();
        }

    fft.mutex.Unlock ();
    }

void CFFTWnd::OnTuneViolinG() 
{
    fft.mutex.Lock ();

	fft.SetRefNote( "G", 3 );
    if ( waveOut.GetPlayingFreq () != fft.refFreq )
        waveOut.SetGenerator( fft.refFreq, MAG_minus3dB );
    else
        waveOut.SetGenerator ();

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnTuneViolinD() 
{
    fft.mutex.Lock ();

    fft.SetRefNote( "D", 4 );
    if ( waveOut.GetPlayingFreq () != fft.refFreq )
        waveOut.SetGenerator( fft.refFreq, MAG_minus3dB );
    else
        waveOut.SetGenerator ();

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnTuneViolinA() 
{
    fft.mutex.Lock ();

	fft.SetRefNote( "A", 4 );
    if ( waveOut.GetPlayingFreq () != fft.refFreq )
        waveOut.SetGenerator( fft.refFreq, MAG_minus3dB );
    else
        waveOut.SetGenerator ();

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnTuneViolinE() 
{
    fft.mutex.Lock ();

    fft.SetRefNote( "E", 5 );
    if ( waveOut.GetPlayingFreq () != fft.refFreq )
        waveOut.SetGenerator( fft.refFreq, MAG_minus3dB );
    else
        waveOut.SetGenerator ();

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnTuneAuto() 
{
    fft.mutex.Lock ();

    fft.SetRefNote ();
    waveOut.SetGenerator ();   	

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnStopPlaying() 
{
    fft.mutex.Lock ();

    waveOut.SetGenerator ();   	

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnOctaveUp() 
{
    fft.mutex.Lock ();

    double playing_freq = waveOut.GetPlayingFreq ();
    if ( playing_freq > 0.0 )
    {
        if ( playing_freq * 2.0 <= 20e3 )
            playing_freq *= 2.0;
        waveOut.SetGenerator( playing_freq, MAG_minus3dB );
        }

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnOctaveDown() 
{
    fft.mutex.Lock ();

    double playing_freq = waveOut.GetPlayingFreq ();
    if ( playing_freq > 0.0 )
    {
        if ( playing_freq / 2.0 >= 15.0 )
            playing_freq /= 2.0;
        waveOut.SetGenerator( playing_freq, MAG_minus3dB );
        }

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnMeasureWaveIn() 
{
    fft.mutex.Lock ();

    waveOut.TurnOffMeasureFFT ();
    waveIn.TurnOnMeasureFFT ();

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnMeasureWaveOut() 
{
    fft.mutex.Lock ();

    waveIn.TurnOffMeasureFFT();
    waveOut.TurnOnMeasureFFT ();

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnStartLearningNoise() 
{
    fft.mutex.Lock ();

    waveOut.SetGenerator ();

    if ( fft.noiseFilter == FFT::NOISEF_LEARNING )
    {
        for ( int i = 0; i < fft.len; i++ )
            fft.noiseLevelMagLin[ i ] = PCM_RANGE * fft.avgMagLin[ i ];

        fft.noiseFilter = FFT::NOISEF_ENABLED;
        fft.noiseTau = 1.0;

        fft.cutOffMagDb = -70.0;

        if ( displayMagLog )
            maxMagDb = 0.0;
        else
            maxMagDb = fft.cutOffMagDb;
        maxMagLin = Db2Lin( maxMagDb );

        if ( ! fft.pkwin.active )
        {
            fft.pkwin.m1 = fft.cutOffMagDb;
            fft.pkwin.m2 = 0.0;
            }
        }
    else
    {
        fft.noiseFilter = FFT::NOISEF_LEARNING;
        fft.noiseTau = 1.0;

        for ( int i = 0; i < fft.len; i++ )
            fft.noiseLevelMagLin[ i ] = 0.0;
        }

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnClearLearnedNoise() 
{
    fft.mutex.Lock ();
/*
    for ( int i = 0; i < fft.len; i++ ) 
        fft.noiseLevelMagLin[ i ] = 0;
*/
    if ( fft.noiseFilter == FFT::NOISEF_DISABLED )
    {
        fft.noiseFilter = FFT::NOISEF_ENABLED;
        fft.noiseTau = 1.0;
        }
    else
    {
        fft.noiseFilter = FFT::NOISEF_DISABLED;
        fft.noiseTau = 1.0;
        }

    waveOut.SetGenerator ();

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnViewChromatic() 
{
    fft.mutex.Lock ();

    maxMagDb = fft.cutOffMagDb;
    maxMagLin = Db2Lin( maxMagDb );

    displayMagLog = false;
    displayFreq = DISPLAY_CHROMATIC;

    CPoint point;
    GetCursorPos( &point );
    ScreenToClient( &point );
    OnSetCursorPosition( point );

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnViewFreqLog() 
{
    fft.mutex.Lock ();

    maxMagDb = 0.0;
    maxMagLin = Db2Lin( maxMagDb );

    displayMagLog = true;
    displayFreq = DISPLAY_FREQ_LOG;

    CPoint point;
    GetCursorPos( &point );
    ScreenToClient( &point );
    OnSetCursorPosition( point );

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnViewFreqLin() 
{
    fft.mutex.Lock ();

    maxMagDb = fft.cutOffMagDb;
    maxMagLin = Db2Lin( maxMagDb );

    displayMagLog = true;
    displayFreq = DISPLAY_FREQ_LIN;

    CPoint point;
    GetCursorPos( &point );
    ScreenToClient( &point );
    OnSetCursorPosition( point );

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnViewFreqToggle() 
{
    fft.mutex.Lock ();

    if ( displayFreq == DISPLAY_CHROMATIC )
    {
        OnViewFreqLog ();
        }
    else if ( displayFreq == DISPLAY_FREQ_LOG || displayFreq == DISPLAY_FREQ_LIN  )
    {
        OnViewChromatic ();
        }

    OnUI_Changed ();

    fft.mutex.Unlock ();
    }

void CFFTWnd::OnViewLogToggle() 
{
    fft.mutex.Lock ();

    displayMagLog = ! displayMagLog;

    if ( displayMagLog )
        maxMagDb = 0.0;
    else
        maxMagDb = fft.cutOffMagDb;
    maxMagLin = Db2Lin( maxMagDb );

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnViewZoomOut() 
{
    fft.mutex.Lock ();

    minFreqK = fft.F2K( 16.0 );
    maxFreqK = fft.F2K( 8.0e3 );

    kLen = maxFreqK - minFreqK + 1;
    logMinFreq = minFreqK == 0 ? 0.0 : log( fft.K2F( minFreqK ) );
    logFreqRange = log( fft.K2F( maxFreqK ) ) - logMinFreq;

    minMagDb = -91.0;

    if ( displayMagLog )
        maxMagDb = 0.0;
    else
        maxMagDb = fft.cutOffMagDb;

    maxMagLin = Db2Lin( maxMagDb );

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnTogglePkWin() 
{
    fft.mutex.Lock ();

    fft.pkwin.active = ! fft.pkwin.active;

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnTogglePlaying() 
{
    fft.mutex.Lock ();

    if ( waveOut.GetPlayingFreq () > 0 )
        OnStopPlaying ();
    else
        waveOut.SetGenerator( fft.avgFreq, MAG_minus3dB );

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnToggleTopMost() 
{
    fft.mutex.Lock ();

    if ( GetExStyle () & WS_EX_TOPMOST )
    {
		SetWindowPos( CWnd::FromHandle(HWND_NOTOPMOST), 0,0,0,0, 
            SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOSIZE );
        }
    else
    {
        SetWindowPos( CWnd::FromHandle(HWND_TOPMOST), 0,0,0,0, 
            SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOSIZE );
        }

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
        if ( GetExStyle () & WS_EX_TOPMOST )
		    pSysMenu->CheckMenuItem( IDM_TOGGLE_TOP_MOST, MFS_CHECKED | MF_BYCOMMAND );
        else
		    pSysMenu->CheckMenuItem( IDM_TOGGLE_TOP_MOST, MFS_UNCHECKED | MF_BYCOMMAND );
	    }

    fft.mutex.Unlock ();
}

void CFFTWnd::OnSysCommand(UINT nID, LPARAM lParam) 
{
	if ((nID & 0xFFF0) == IDM_TOGGLE_TOP_MOST)
	{
		OnToggleTopMost ();
	}
	else
	{
	    CWnd::OnSysCommand(nID, lParam);
	}
}

void CFFTWnd::OnViewWaterfallToggle() 
{
    fft.mutex.Lock ();

    displayWaterfall = ! displayWaterfall;

    if ( displayWaterfall )
    {
        displayFFT = false;
        displayPeakInfo = false;
        }
    else
    {
        displayFFT = true;
        displayPeakInfo = true;
        }

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnToggleTraceWindow() 
{
    fft.mutex.Lock ();

    if ( IsTraceStarted () )
        TraceStop ();
    else
        TraceStart ();

    SetForegroundWindow ();
    SetFocus ();

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnFreezeWaterfall() 
{
    fft.mutex.Lock ();

    if ( ! displayWaterfall )
        displayWaterfall = true;
    else
        freezeWaterfall = ! freezeWaterfall;

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnWaterfallDecSpeed() 
{
    fft.mutex.Lock ();

    if ( ! displayWaterfall )
        displayWaterfall = true;
    else if ( specDeltaY > 1 )
        specDeltaY--;

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnWaterfallIncSpeed() 
{
    fft.mutex.Lock ();

    if ( ! displayWaterfall )
        displayWaterfall = true;
    else if ( specDeltaY < 10 )
        specDeltaY++;

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnDisplayFft() 
{
    fft.mutex.Lock ();

    displayFFT = ! displayFFT;
    displayPeakInfo = displayFFT;

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnDisplayPeakInfo() 
{
    fft.mutex.Lock ();

    displayPeakInfo = ! displayPeakInfo;

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnFftwinRectangular() 
{
    fft.mutex.Lock ();

    fft.SetWindow( FFT::FFTWIN_RECTANGULAR );

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnFftwinHanning() 
{
    fft.mutex.Lock ();

    fft.SetWindow( FFT::FFTWIN_HANNING );

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnFftwinBlackmanHarris() 
{
    fft.mutex.Lock ();

    fft.SetWindow( FFT::FFTWIN_BLACKMAN_HARRIS );

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnFftwinKaiserBessel() 
{
    fft.mutex.Lock ();

    fft.SetWindow( FFT::FFTWIN_KAISER_BESSEL );

    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnCursorUp() 
{
    CPoint point;
	GetCursorPos( &point );
    SetCursorPos( point.x, point.y - 1 );
}

void CFFTWnd::OnCurosrDown() 
{
    CPoint point;
	GetCursorPos( &point );
    SetCursorPos( point.x, point.y + 1 );
}

void CFFTWnd::OnCursorLeft() 
{
    CPoint point;
	GetCursorPos( &point );
    SetCursorPos( point.x - 1, point.y );
}

void CFFTWnd::OnCursorRight() 
{
    CPoint point;
	GetCursorPos( &point );
    SetCursorPos( point.x + 1, point.y );
}


void CFFTWnd::OnCursorUp10() 
{
    CPoint point;
	GetCursorPos( &point );
    SetCursorPos( point.x, point.y - 15 );
}

void CFFTWnd::OnCurosrDown10() 
{
    CPoint point;
	GetCursorPos( &point );
    SetCursorPos( point.x, point.y + 15 );
}

void CFFTWnd::OnCursorLeft10() 
{
    CPoint point;
	GetCursorPos( &point );
    SetCursorPos( point.x - 15, point.y );
}

void CFFTWnd::OnCursorRight10() 
{
    CPoint point;
	GetCursorPos( &point );
    SetCursorPos( point.x + 15, point.y );
}

void CFFTWnd::OnTestButton() 
{
    fft.mutex.Lock ();

    if ( fft.N == 16384 && fft.stepN == 4096 )
    {
        fft.SetNewRate( fft.sampleRate, 8192, 1024 );
        }
    else if ( fft.N == 8192 )
    {
        fft.SetNewRate( fft.sampleRate, 4096, 1024 );
        }
    else if ( fft.N == 4096 )
    {
        fft.SetNewRate( fft.sampleRate, 16384, 1048 );
        }
    else
    {
        fft.SetNewRate( fft.sampleRate, 16384, 4096 );
        }

    minFreqK = fft.F2K( 10.0  );
    maxFreqK = fft.F2K( 20.0e3 );
    logMinFreq = minFreqK == 0 ? 0.0 : log( fft.K2F( minFreqK ) );
    logFreqRange = log( fft.K2F( maxFreqK ) ) - logMinFreq;

    minMagDb = -91.0;
    if ( displayMagLog )
        maxMagDb = 0.0;
    else
        maxMagDb = fft.cutOffMagDb;
    maxMagLin = Db2Lin( maxMagDb );

    CPoint point;
    GetCursorPos( &point );
    ScreenToClient( &point );
    OnSetCursorPosition( point );

    OnData_Changed ();
    OnUI_Changed ();
    fft.mutex.Unlock ();
}

void CFFTWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    if ( nChar == 0x09 )
        fft.longExpAvgTau = true;

    //Trace( "OnKeyDown %02x, %d, %04x\n", nChar, nRepCnt, nFlags );	
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFFTWnd::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    if ( nChar == 0x09 )
        fft.longExpAvgTau = false;

    //Trace( "OnKeyUp %02x, %d, %04x\n", nChar, nRepCnt, nFlags );	
	CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}
