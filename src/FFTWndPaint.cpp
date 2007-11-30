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

///////////////////////////////////////////////////////////////////////////////

void CFFTWnd::OnPaintHeader( CDC& dc )
{
    CString str;

    dc.SelectObject( fontText );
    CSize textExt = dc.GetTextExtent( "X" );
    textExt.cy += 2;

    dc.SelectObject( fontScale );
    CSize scaleExt = dc.GetTextExtent( "+9" );
    scaleExt.cy += 1;

    /////////////////////////////////////////////////////////////////////////// 

    dc.SelectObject( &penGrid );
    dc.MoveTo( rectClient.left, rectHeader.top );
    dc.LineTo( rectClient.right, rectHeader.top );

    /////////////////////////////////////////////////////////////////////////// 
    // CPU Usage
    //
    dc.SetBkColor( colorBackground );
    dc.SetBkMode( TRANSPARENT );

    dc.SetTextAlign( TA_LEFT );
    dc.SetTextColor( colorTextDarkYellow );

    str.Format( "CPU: %4.1lf%% (%.1lf, %.1lf, %.1lf, %.1lf)", 
        double( cpuUsage_process ), double( cpuUsage_display ),
        double(  fft.cpuUsage ), double( waveIn.cpuUsage ), double( waveOut.cpuUsage )
        );
    dc.ExtTextOut( rectHeader.left, rectHeader.top + 15 + textExt.cy, ETO_OPAQUE, NULL, str, NULL );

    /////////////////////////////////////////////////////////////////////////// 
    // Tuning Mode
    //
    dc.SelectObject( fontText );
    dc.SetTextColor( colorTextHighlighted );
    dc.SetTextAlign( TA_LEFT );

    if ( ! fft.refNoteLocked )
    {
        str.Format( "Tune: Auto" );
        }
    else
    {
        str.Format( "Tune: %s", fft.noteName[fft.refNote] );
        }

    dc.ExtTextOut( rectHeader.left + 2, rectHeader.top + 15, ETO_OPAQUE, NULL, str, NULL );

    /////////////////////////////////////////////////////////////////////////// 
    // Cursor info
    //
    dc.SelectObject( fontText );
    dc.SetTextColor( colorTextDarkYellow );
    dc.SetTextAlign( TA_RIGHT );

    str = "Hz";
    dc.ExtTextOut( rectHeader.right, rectHeader.top + 10 + 1 * textExt.cy, ETO_OPAQUE, NULL, str, NULL );

    str = "dB";
    dc.ExtTextOut( rectHeader.right, rectHeader.top + 10 + 2 * textExt.cy, ETO_OPAQUE, NULL, str, NULL );

    textExt.cx = dc.GetTextExtent( "dB" ).cx + 10;

    dc.SetTextColor( colorTextCursor );

    str.Format( selectionActive ? "End" : "Cursor" );
    dc.ExtTextOut( rectHeader.right - textExt.cx, rectHeader.top + 8, ETO_OPAQUE, NULL, str, NULL );

    if ( curFreq < 10e3 )
        str.Format( "%6.1lf", curFreq );
    else
        str.Format( "%6.lf", curFreq );
    dc.ExtTextOut( rectHeader.right - textExt.cx, rectHeader.top + 10 + 1 * textExt.cy, ETO_OPAQUE, NULL, str, NULL );

    str.Format( "%+6.1lf", curMagDb );
    dc.ExtTextOut( rectHeader.right - textExt.cx, rectHeader.top + 10 + 2 * textExt.cy, ETO_OPAQUE, NULL, str, NULL );

    textExt.cx += dc.GetTextExtent( "000000" ).cx + 20;

    if ( selectionActive )
    {
        str.Format( "Begin" );
        dc.ExtTextOut( rectHeader.right - textExt.cx, rectHeader.top + 8, ETO_OPAQUE, NULL, str, NULL );

        if ( startFreq < 10e3 )
            str.Format( "%6.1lf", startFreq );
        else
            str.Format( "%6.lf", startFreq );
        dc.ExtTextOut( rectHeader.right - textExt.cx, rectHeader.top + 10 + 1 * textExt.cy, ETO_OPAQUE, NULL, str, NULL );

        str.Format( "%+6.1lf", startMagDb );
        dc.ExtTextOut( rectHeader.right - textExt.cx, rectHeader.top + 10 + 2 * textExt.cy, ETO_OPAQUE, NULL, str, NULL );
        }

    /////////////////////////////////////////////////////////////////////////// 

    static lastPlanStatus = FFT::PLAN_NOT_INITIALIZED;

    if ( fft.planStatus != FFT::PLAN_INITIALIZED )
    {
        dc.SelectObject( fontText );
        dc.SetTextAlign( TA_CENTER );
        dc.SetBkMode( TRANSPARENT );

        dc.SetTextColor( colorTextRed );

        if ( fft.planStatus == FFT::PLAN_INIT_1 )
            str.Format( "Initializing FFT plan: 1 of 3" );
        else if ( fft.planStatus == FFT::PLAN_INIT_2 )
            str.Format( "Initializing FFT plan: 2 of 3" );
        else if ( fft.planStatus == FFT::PLAN_INIT_3 )
            str.Format( "Initializing FFT plan: 3 of 3" );
        else if ( fft.planStatus == FFT::PLAN_NOT_INITIALIZED )
            str.Format( "Intitializing FFT plan..." );

        dc.ExtTextOut( rectHeader.left + rectHeader.Width()/2, rectHeader.top + rectHeader.Height()/2 - textExt.cy/2, ETO_OPAQUE, NULL, str, NULL );

        return;
        }

    /////////////////////////////////////////////////////////////////////////// 
    // Information about the the tone -- if any
    //
    if ( fft.avgFreq <= 0.0 )
        return;

    /////////////////////////////////////////////////////////////////////////// 
    // Spectrum peak info
    //
    dc.SetTextColor( colorTextDarkYellow );
    dc.SetTextAlign( TA_RIGHT );

    if ( ! selectionActive )
    {
        str.Format( "Peak" );
        dc.ExtTextOut( rectHeader.right - textExt.cx, rectHeader.top + 8, ETO_OPAQUE, NULL, str, NULL );

        if ( fft.avgFreq < 10e3 )
            str.Format( "%6.1lf", fft.avgFreq );
        else
            str.Format( "%6.lf", fft.avgFreq );
        dc.ExtTextOut( rectHeader.right - textExt.cx, rectHeader.top + 10 + 1 * textExt.cy, ETO_OPAQUE, NULL, str, NULL );

        str.Format( "%+6.1lf", fft.avgMagDb );
        dc.ExtTextOut( rectHeader.right - textExt.cx, rectHeader.top + 10 + 2 * textExt.cy, ETO_OPAQUE, NULL, str, NULL );
        }

    textExt.cx += dc.GetTextExtent( "000000" ).cx + 20;

    /////////////////////////////////////////////////////////////////////////// 
    // Reference note and distance in cents from detected note
    // (Cents gage with needle)
    //
    double cents = fft.avgCents > 50.0 ? 50.0 : fft.avgCents < -50.0 ? -50.0 : fft.avgCents;

    int xCent = ( rectHeader.left + 200 + rectHeader.right - textExt.cx ) / 2;
    int wCent = 3;

    CRect outCents( CPoint( xCent - 50 * wCent - 1, rectHeader.top + 16 ), CSize( 100 * wCent + 3, 20 ) );
/*
    if ( cents < -5.0 )
    {
        CBitmap* oldbmp;
        CDC memdc;
        memdc.CreateCompatibleDC( &dc );
        oldbmp = memdc.SelectObject( &bmpFlat );
        dc.BitBlt( outCents.left - 24, outCents.top - 4, 24, 24, &memdc, 0, 0, SRCCOPY );
        memdc.SelectObject( oldbmp );
        }
    if ( cents > 5.0 )
    {
        CBitmap* oldbmp;
        CDC memdc;
        memdc.CreateCompatibleDC( &dc );
        oldbmp = memdc.SelectObject( &bmpSharp );
        dc.BitBlt( outCents.right, outCents.top - 4, 24, 24, &memdc, 0, 0, SRCCOPY );
        memdc.SelectObject( oldbmp );
        }
*/
    dc.SelectObject( &fontMusic );

    dc.SetTextColor( colorGraphCents );
    if ( cents < -5.0 )
    {
        dc.SetTextAlign( TA_RIGHT );
        dc.ExtTextOut( outCents.left - 8, 
            outCents.top + outCents.Height()/2 - 3 - dc.GetTextExtent( strFlat ).cy/2, 
            ETO_OPAQUE, NULL, strFlat, NULL );
        }
    if ( cents > 5.0 )
    {
        dc.SetTextAlign( TA_LEFT );
        dc.ExtTextOut( outCents.right + 8, 
            outCents.top + outCents.Height()/2 - 3 - dc.GetTextExtent( strSharp ).cy/2, 
            ETO_OPAQUE, NULL, strSharp, NULL );
        }

    dc.SelectObject( fontText );

    dc.SetTextColor( colorTextHighlighted );
    dc.SetTextAlign( TA_CENTER );

    char sNote[ 32 ], sFreq[ 32 ];
    sprintf( sNote, "%s%d", fft.noteName[fft.refNote], fft.octave );

    if ( fft.centerFreq < 1e4 )
        sprintf( sFreq, "%.1lf", fft.centerFreq );
    else
        sprintf( sFreq, "%.lf", fft.centerFreq );

    if ( -5.0 <= cents && cents <= 5.0 )
    {
        str.Format( "%-3s %6s Hz %+4.1lf ¢", sNote, sFreq, fft.avgCents );
        dc.ExtTextOut( xCent, outCents.bottom + 6, ETO_OPAQUE, NULL, str, NULL );
        }
    else
    {
        str.Format( "%-3s %6s Hz %+4.lf ¢", sNote, sFreq, fft.avgCents );
        dc.ExtTextOut( xCent, outCents.bottom + 6, ETO_OPAQUE, NULL, str, NULL );
        }

    if ( -1.0 < cents && cents < 1.0 )
        dc.SetTextColor( colorGraphCents );
    else
        dc.SetTextColor( colorGraphCentsErr );

    dc.FrameRect( &outCents, &CBrush( colorGraphCentsFrame ) );

    for ( int j = -50; j <= 50; j++ )
    {
        dc.FillSolidRect( xCent + j * wCent, outCents.top, 
            1, j % 10 == 0 ? -5 : j % 5 == 0 ? -3 : -2,
            colorGraphCentsFrame );
        }

    outCents.DeflateRect( 1, 1 );
    dc.FillSolidRect( &outCents, colorGraphBackground );

    outCents.DeflateRect( 0, 1 );

    int x = xCent + int( cents * wCent );

    CRect inCents = outCents;

    for ( int i = 24; i > 0; i-=2 )
    {
        inCents.left = x - i < outCents.left ? outCents.left : x - i;
        inCents.right = x + i + 1 > outCents.right ? outCents.right : x + i + 1;
        if ( whiteBackground )
            dc.FillSolidRect( &inCents, RGB( ( i ) * 10, ( i ) * 10, ( i ) * 10 ) );
        else
            dc.FillSolidRect( &inCents, RGB( ( 25 - i ) * 10, ( 25 - i ) * 10, ( 25 - i ) * 10 ) );
        }

    inCents.left = xCent;
    inCents.right = inCents.left + 1;
    dc.FillSolidRect( inCents, fabs(cents) > 5.0 ? colorGraphCentsErr : RGB(0,0,0) );

    if ( cents >= -1.2 )
    {
        int pos = outCents.right - 2;
        dc.SetTextAlign( TA_RIGHT );
        dc.ExtTextOut( pos, outCents.top - 1, 0, NULL, "<<", 2, NULL );
        dc.SetTextAlign( TA_LEFT );
        }
    if ( cents <= 1.2 )
    {
        int pos = outCents.left + 2;
        dc.SetTextAlign( TA_LEFT );
        dc.ExtTextOut( pos, outCents.top - 1, 0, NULL, ">>", 2, NULL );
        }
    }

///////////////////////////////////////////////////////////////////////////////

void CFFTWnd::OnPaintFooter( CDC& dc )
{
    CString str;

    dc.SelectObject( fontText );
    CSize textExt = dc.GetTextExtent( "X" );
    textExt.cy += 2;

    dc.SelectObject( fontScale );
    CSize scaleExt = dc.GetTextExtent( "+9" );
    scaleExt.cy += 1;

    /////////////////////////////////////////////////////////////////////////// 

    dc.FillSolidRect( rectClient.left, rectFooter.top, rectClient.Width (), 1, colorGraphGrid );

    /////////////////////////////////////////////////////////////////////////// 

    dc.SelectObject( fontScale );
    dc.SetTextAlign( TA_LEFT );
    dc.SetBkMode( TRANSPARENT );

    dc.SetTextColor( colorTextDarkYellow );

    double playing_freq = waveOut.GetPlayingFreq ();
    if ( playing_freq > 0.0 )
    {
        str.Format( "Playing: %.1lf Hz, %+.1lf dB", 
            playing_freq, Lin2Db( PCM_RANGE * waveOut.GetPlayingVolume () ) );
        }
    else if ( fft.noiseFilter == FFT::NOISEF_LEARNING )
    {
        str.Format( "Learning Noise; Timer: %.lf", fft.noiseTau );
        }
    else
    {
        char* winInfo = "?";
        switch( fft.winSel )
        {
            case FFT::FFTWIN_RECTANGULAR:     winInfo = "Rectangular"; break;
            case FFT::FFTWIN_HANNING:         winInfo = "Hanning"; break;
            case FFT::FFTWIN_BLACKMAN_HARRIS: winInfo = "Blackman-Harris"; break;
            case FFT::FFTWIN_KAISER_BESSEL:   winInfo = "Kaiser-Bessel"; break;
            }

        str.Format( "FFT: %d @ %.1lf kHz, Window: %s, Overlap: %d (%.lf ms)", 
            fft.N, fft.sampleRate / 1e3, 
            winInfo,
            fft.stepN, 1e3 * fft.stepN / fft.sampleRate
            );
        }

    dc.ExtTextOut( rectFooter.left, rectFooter.top + 10, ETO_OPAQUE, NULL, str, NULL );

    dc.SetTextAlign( TA_RIGHT );
    
    str.Format( "View: [%.1lf, %.1lf] Hz, [%+.1lf, %+.1lf] dB", 
        fft.K2F( minFreqK ), fft.K2F( maxFreqK ),
        minMagDb, maxMagDb
        );

    dc.ExtTextOut( rectFooter.right, rectFooter.top + 10, ETO_OPAQUE, NULL, str, NULL );
    }

//////////////////////////////////////////////////////////////////////////////

void CFFTWnd::OnPaintHorizontalGrid( CDC& dc )
{
    CString str;

    dc.SelectObject( &fontScale );
    CSize scaleExt = dc.GetTextExtent( "+00" );

    /////////////////////////////////////////////////////////////////////////// 
    // Bottom & Top horizontal magnitude reference lines and
    // cut-off magnitude horizontal line
    //
    dc.SetTextAlign( TA_RIGHT );
    dc.SetTextColor( colorTextDefault );

    dc.SelectObject( &penGrid );
    dc.MoveTo( rectXY.left, rectXY.bottom );
    dc.LineTo( rectXY.right, rectXY.bottom );
    dc.MoveTo( rectXY.left, rectXY.top );
    dc.LineTo( rectXY.right, rectXY.top );

    dc.SetBkColor( colorBackground );

    if ( displayMagLog )
    {
        dc.MoveTo( rectXY.left, rectXY.top );
        dc.LineTo( rectXY.right, rectXY.top );

        // Set up horLevel to be in tens of "absolute" dB 
        double horLevelDb = floor( ( maxMagDb ) / 10.0 ) * 10.0;

        int secondLevelY = rectXY.top;

        for ( int i = 0; i < 16 && horLevelDb > minMagDb; i++ )
        {
            int y = magDb_to_yLog( horLevelDb );

            if ( i == 0 )
                secondLevelY = y - secondLevelY;

            dc.MoveTo( rectScale.left, y );
            dc.LineTo( displayWaterfall ? rectXY.left : rectXY.right, y );

            str.Format( "%+.lf", horLevelDb );
            dc.ExtTextOut( rectScale.left - 2, y - scaleExt.cy / 2, ETO_OPAQUE, NULL, str, NULL );

            horLevelDb -= 10.0;
            }

        if ( secondLevelY >= scaleExt.cy )
        {
            str.Format( "%.lf", maxMagDb );
            dc.ExtTextOut( rectScale.left - 2, rectXY.top - scaleExt.cy / 2, ETO_OPAQUE, NULL, str, NULL );
            }

        if ( ! fft.pkwin.active && minMagDb <= fft.cutOffMagDb && fft.cutOffMagDb <= maxMagDb )
        {
            dc.SelectObject( &penGridCutOff );

            int y = magDb_to_Y( fft.cutOffMagDb );

            dc.MoveTo( rectXY.left, y );
            dc.LineTo( rectXY.right, y );

            dc.SetTextAlign( TA_LEFT );
            dc.SetTextColor( colorTextCutoff );
            str.Format( "%+.1lf dB", fft.cutOffMagDb );
            dc.ExtTextOut( rectXY.left + 2, y - scaleExt.cy, 0, NULL, str, NULL );
            }
        }
    else
    {
        double horLevel = maxMagLin;
        int last_yTxt = rectScale.top - 20;
        int last_yLine = rectScale.top - 20;

        for ( int i = 0; i < 20 && horLevel > 1e-10; i++ )
        {
            int y = magLin_to_yLin( horLevel );

            if ( y <= last_yLine + 1 )
                break;

            dc.MoveTo( rectScale.left, y );
            dc.LineTo( displayWaterfall ? rectXY.left : rectXY.right, y );

            if ( y - last_yTxt >= 15 )
            {
                double dB = Lin2Db( horLevel );
                str.Format( "%+.lf", dB );
                dc.ExtTextOut( rectScale.left - 2, y - scaleExt.cy / 2, ETO_OPAQUE, NULL, str, NULL );
                last_yTxt = y;
                }

            horLevel *= MAG_minus3dB; // horLevelDb -= 3 dB
            last_yLine = y;
            }

        if ( ! fft.pkwin.active && minMagDb <= fft.cutOffMagDb && fft.cutOffMagDb <= maxMagDb )
        {
            dc.SelectObject( &penGridCutOff );

            int y = magDb_to_Y( fft.cutOffMagDb );

            dc.MoveTo( rectXY.left, y );
            dc.LineTo( rectXY.right, y );

            dc.SetTextAlign( TA_LEFT );
            dc.SetTextColor( colorTextCutoff );
            str.Format( "%+.1lf dB", fft.cutOffMagDb );
            dc.ExtTextOut( rectXY.left + 2, y - scaleExt.cy, 0, NULL, str, NULL );
            }
        }
    }

///////////////////////////////////////////////////////////////////////////////

void CFFTWnd::OnPaintSpectralVerticalGrid( CDC& dc )
{
    CString str;

    dc.SelectObject( &fontScale );

    CSize scaleExt = dc.GetTextExtent( "00000" );

    dc.SetBkMode( TRANSPARENT );
    dc.SetTextAlign( TA_CENTER );
    dc.SetTextColor( colorTextDefault );

    int N = 40;
    
    /////////////////////////////////////////////////////////////////////////// 
    // Vertical frequency lines

    dc.SelectObject( &penGrid );

    dc.MoveTo( rectXY.left, rectXY.bottom );
    dc.LineTo( rectXY.left, rectXY.top );
    dc.MoveTo( rectXY.right, rectXY.bottom );
    dc.LineTo( rectXY.right, rectXY.top );

    double baseHarmonic = fft.centerFreq;
    double baseHarmMulti = 2.0;

    if ( displayFreq == DISPLAY_FREQ_LIN )
    {
        double f = fft.K2F( kLen );

        baseHarmMulti = pow( 10.0, ceil( log10( f * ( scaleExt.cx + 2 ) / rectXY.Width ( ) ) ) );
        if ( baseHarmMulti < 1.0 )
            baseHarmMulti = 1.0;

        baseHarmonic = floor( fft.K2F( minFreqK ) / baseHarmMulti ) * baseHarmMulti;

        N = 1 + int( f / baseHarmMulti + 0.5 );
        }
    else if ( fft.refNoteLocked )
    {
        while ( baseHarmonic > fft.K2F( minFreqK ) ) 
            baseHarmonic /= 2.0;
        }
    else // LOG and not locked
    {
        baseHarmonic = pow( 10.0, floor( log10( fft.K2F( minFreqK ) ) ) );
        if ( baseHarmonic < 10.0 )
            baseHarmonic = 10.0;
        }

    for ( int i = 0; i < N && baseHarmonic < 20000.0; i++ )
    {
        int x = Freq_to_xSpec( baseHarmonic );

        if ( x > rectXY.right )
            break;

        if ( displayFreq == DISPLAY_FREQ_LIN )
        {
            if ( x >= rectXY.left )
            {
                dc.SelectObject( &penGrid );
                dc.MoveTo( x, rectXY.bottom );
                dc.LineTo( x, rectXY.top );

                if ( baseHarmMulti < 100.0 )
                    str.Format( "%.lf", baseHarmonic );
                else
                    str.Format( "%.1lfk", baseHarmonic / 1e3 );

                dc.ExtTextOut( x, rectXY.bottom + 4, ETO_OPAQUE, NULL, str, NULL );
                }
            }
        else if ( fft.refNoteLocked )
        {
            if ( x >= rectXY.left )
            {
                dc.SelectObject( &penGrid );
                dc.MoveTo( x, rectXY.bottom );
                dc.LineTo( x, rectXY.top );

                if ( baseHarmonic < 1000.0 )
                    str.Format( "%.1lf", baseHarmonic );
                else
                    str.Format( "%.lf", baseHarmonic );

                dc.ExtTextOut( x, rectXY.bottom + 4, ETO_OPAQUE, NULL, str, NULL );
                }

            for ( int j = 1; j < 12; j++ )
            {
                int x = Freq_to_xSpec( baseHarmonic * fft.noteFreq[j] / fft.noteFreq[0] );

                if ( rectXY.left <= x && x <= rectXY.right )
                {
                    dc.SelectObject( &penGridNote );
                    dc.MoveTo( x, rectXY.bottom );
                    dc.LineTo( x, rectXY.top );
                    }
                }
            }
        else // LOG and not locked
        {
            if ( x >= rectXY.left )
            {
                if ( baseHarmonic == 1e1 || baseHarmonic == 1e2 
                  || baseHarmonic == 1e3 || baseHarmonic == 1e4 )
                {
                    dc.SelectObject( &penGrid );
                    }
                else
                {
                    dc.SelectObject( &penGridSub );
                    }
                dc.MoveTo( x, rectXY.bottom );
                dc.LineTo( x, rectXY.top );

                int t;
                if ( baseHarmonic < 100.0 )
                    t = int( baseHarmonic / 10.0 );
                else if ( baseHarmonic < 1000.0 )
                    t = int( baseHarmonic / 100.0 );
                else if ( baseHarmonic < 10000.0 )
                    t = int( baseHarmonic / 1000.0 );
                else
                    t = int( baseHarmonic / 10000.0 );

                if ( ( t % 10 ) >= 0 && ( t % 10 ) <= 5 && baseHarmonic > 1.0 )
                {
                    if ( baseHarmonic < 1000.0 )
                        str.Format( "%.lf", baseHarmonic );
                    else
                        str.Format( "%.1lfk", baseHarmonic / 1e3 );

                    dc.ExtTextOut( x, rectXY.bottom + 4, ETO_OPAQUE, NULL, str, NULL );
                    }
                }
            }

        if ( displayFreq == DISPLAY_FREQ_LIN )
        {
            baseHarmonic += baseHarmMulti;
            }
        else if ( fft.refNoteLocked )
        {
            baseHarmonic *= baseHarmMulti;
            }
        else // LOG and not locked
        {
            if ( baseHarmonic < 100.0 )
                baseHarmonic += 10.0;
            else if ( baseHarmonic < 1000.0 )
                baseHarmonic += 100.0;
            else if ( baseHarmonic < 10000.0 )
                baseHarmonic += 1000.0;
            else
                baseHarmonic += 10000.0;
            }
        }
    }

void CFFTWnd::OnPaintVUMeter( CDC& dc )
{
    // VU RMS left/right
    int L1 = rectXY.right + 3;
    int R1 = rectScale.right - 1;
    // VU Peak left/right
    int L2 = L1 + 3;
    int R2 = R1 - 3;

    CRect rect;
    rect.top = rectXY.top;
    rect.bottom = rectXY.bottom + 1;
    rect.left = L1 - 1;
    rect.right = R1 + 1;

    dc.SelectObject( &penGrid );
    dc.FrameRect( &rect, &CBrush( colorGraphGrid ) );

    rect.DeflateRect( 1, 1 );

    for ( int y = rect.top; y < rect.bottom; y++ )
    {
        int RGB = magDb_to_RGB( Y_to_magDb( y ) );
        dc.FillSolidRect( rect.left, y, rect.Width (), 1, RGB );
        }

    ////////////////////////////////////////////////////////////////////////
    // Peak
    //
    int yP = magLin_to_Yabs( fft.vuPk );

    if ( yP < rect.top )
        yP = rect.top;
    if ( yP > rect.bottom - 1 )
        yP = rect.bottom - 1;

    ////////////////////////////////////////////////////////////////////////
    // RMS
    //
    int yA = magLin_to_Yabs( fft.vuRms );
    if ( yA < rect.top )
        yA = rect.top;
    if ( yA > rect.bottom - 1 )
        yA = rect.bottom - 1;

    ////////////////////////////////////////////////////////////////////////
    // Peak inertion bar
    //
    int yB = magLin_to_Yabs( fft.vuBar );
    if ( yB < rect.top )
        yB = rect.top;
    if ( yB > rect.bottom - 1 )
        yB = rect.bottom - 1;

    ////////////////////////////////////////////////////////////////////////
    // Draw peak and avg
    //
    if ( fft.vuPk < fft.vuRms )
        yP = yA;

    rect.bottom = yP;

    dc.FillSolidRect( &rect, colorGraphBackground );

    rect.bottom = yA;

    rect.left = L1;
    rect.right = L2;

    dc.FillSolidRect( &rect, colorGraphBackground );

    rect.right = R2;
    rect.left = R1;

    dc.FillSolidRect( &rect, colorGraphBackground );

    ////////////////////////////////////////////////////////////////////////
    // Saturation warning (if peak is above -3dB)
    //
    if ( fft.vuPk >= MAG_minus3dB )
    {
        rect.top = yP;
        rect.left = L2;
        rect.right = R2;
        dc.FillSolidRect( &rect, colorTextRed );
        }

    ////////////////////////////////////////////////////////////////////////
    // Draw peak intertion bar
    //
    rect.top = yB;
    rect.bottom = yB + 2;
    rect.left = L1;
    rect.right = R1;
    dc.FillSolidRect( &rect, 
        fft.vuBar >= MAG_minus3dB ? colorTextRed : magDb_to_RGB( Y_to_magDb( yB ) ) );
    }

///////////////////////////////////////////////////////////////////////////////

void CFFTWnd::OnPaintCursorInfo( CDC& dc )
{
/*
    CRgn rgn;
    rgn.CreateRectRgn( rectClient.left, rectClient.top, rectClient.right, rectClient.bottom );
    dc.SelectClipRgn( &rgn, RGN_OR );
*/
    CPoint pt;
    GetCursorPos( &pt );
    ScreenToClient( &pt );

    if ( ! rectXY.PtInRect( pt ) )
        return;

    int xpos = pt.x + 70 < rectXY.right ? pt.x + 70 : pt.x - 3;
    int ypos = pt.y - 34 > rectXY.top ? pt.y - 34 : pt.y + 3;

    if ( displayWaterfall && pt.y - 34 > rectXY.top )
        ypos += 17;

    dc.SelectObject( fontScale );
    dc.SetTextColor( colorTextCursor );

    CString str;
    if ( displayFreq == DISPLAY_CHROMATIC )
    {
        str.Format( "%s", fft.noteName[  curNote] );
        dc.SetTextAlign( TA_LEFT );
        dc.ExtTextOut( xpos - dc.GetTextExtent( "D#+00.0c" ).cx, ypos, 0, NULL, str, NULL );

        str.Format( "%+5.1lf¢", curCents );
        dc.SetTextAlign( TA_RIGHT );
        dc.ExtTextOut( xpos, ypos, 0, NULL, str, NULL );
        }
    else if ( displayFreq == DISPLAY_FREQ_LOG || displayFreq == DISPLAY_FREQ_LIN )
    {
        str.Format( "%6.1lf Hz", curFreq );
        dc.SetTextAlign( TA_RIGHT );
        dc.ExtTextOut( xpos, ypos, 0, NULL, str, NULL );
        }

    dc.SetTextAlign( TA_RIGHT );

    if ( ! displayWaterfall )
    {
        str.Format( "%+6.1lf dB", curMagDb );
        dc.ExtTextOut( xpos, ypos + 17, 0, NULL, str, NULL );
        }
    }

///////////////////////////////////////////////////////////////////////////////

void CFFTWnd::OnSetCursorPosition( CPoint point )
{
    fft.mutex.Lock ();

    if ( point.x < rectXY.left )
        point.x = rectXY.left;
    if ( point.x > rectXY.right )
        point.x = rectXY.right;

    if ( point.y < rectXY.top )
        point.y = rectXY.top;
    if ( point.y > rectXY.bottom )
        point.y = rectXY.bottom;

    curMagDb = Y_to_magDb( point.y );

    SetCurFreq( point.x );

    fft.mutex.Unlock ();

#if 0
    TraceNOTS( "X %5d -- %5d %5d, Y %5d -- %5d %5d --> Freq %6.1lf, MagDb %6.1lf\n", 
        point.x, rectXY.left, rectXY.right, 
        point.y, rectXY.top, rectXY.bottom, 
        curFreq, curMagDb
        );
#endif

    }

///////////////////////////////////////////////////////////////////////////////

static inline void normalize( double& x, double& y )
{
    if ( x > y )
    {
        double t = x;
        x = y;
        y = t;
        }
    }

void CFFTWnd::SetSelectionStart( void )
{
    fft.mutex.Lock ();

    startFreq = curFreq;
    startMagDb = curMagDb;
    selectionActive = true;

    fft.mutex.Unlock ();
    }

void CFFTWnd::SetPkWindow( void )
{
    fft.mutex.Lock ();

    if ( startFreq < 1e-15 )
        startFreq = 1e-15;

    if ( curFreq < 1e-15 )
        curFreq = 1e-15;

    fft.pkwin.f1 = startFreq;
    fft.pkwin.f2 = curFreq;
    fft.pkwin.m1 = startMagDb;
    fft.pkwin.m2 = curMagDb;

    normalize( fft.pkwin.f1, fft.pkwin.f2 );
    normalize( fft.pkwin.m1, fft.pkwin.m2 );

    // make selection to be pkWindow
    //
    fft.pkwin.active = true;
    selectionActive = false;

    Trace( "PkWIN: %.1lf .. %.1lf Hz, %.1lf .. %.1lf dB\n", fft.pkwin.f1, fft.pkwin.f2, fft.pkwin.m1, fft.pkwin.m2 );

    fft.mutex.Unlock ();
    }

void CFFTWnd::SetZoomWindow( void )
{
    fft.mutex.Lock ();

    if ( startFreq < 1e-15 )
        startFreq = 1e-15;

    if ( curFreq < 1e-15 )
        curFreq = 1e-15;

    double f1 = startFreq;
    double f2 = curFreq;
    double m1 = startMagDb;
    double m2 = curMagDb;

    normalize( f1, f2 );
    normalize( m1, m2 );

    minFreqK = fft.F2K( f1 );
    maxFreqK = fft.F2K( f2 );

    if ( maxFreqK - minFreqK < 10 )
    {
        minFreqK = fft.F2K( 10.0 );
        maxFreqK = fft.F2K( 20.0e3 );
        }

    kLen = maxFreqK - minFreqK + 1;
    logMinFreq = minFreqK == 0 ? 0.0 : log( fft.K2F( minFreqK ) );
    logFreqRange = log( fft.K2F( maxFreqK ) ) - logMinFreq;

    maxMagLin = Db2Lin( m2 );
    maxMagDb = m2;
    minMagDb = m1;

    selectionActive = false;

    Trace( "ZOOM: %.1lf .. %.1lf Hz, %.1lf .. %.1lf dB\n", f1, f2, m1, m2 );

    fft.mutex.Unlock ();
    }

void CFFTWnd::SetCutOffMagAbs( void )
{
    fft.mutex.Lock ();

    fft.pkwin.active = false;

    fft.cutOffMagDb = curMagDb;

    Trace( "cutOffMagDb = %.2lf\n", fft.cutOffMagDb );

    fft.mutex.Unlock ();
    }

