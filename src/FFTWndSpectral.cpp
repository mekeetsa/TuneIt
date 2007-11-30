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

void CFFTWnd::OnPaintSpectral( CDC& dc )
{
    if ( fft.N < 0 )
        return;

    ///////////////////////////////////////////////////////////////////////////////

    dc.FillSolidRect( &rectXY, colorGraphBackground );

    if ( fft.pkwin.active )
    {
        int x1 = Freq_to_xSpec( fft.pkwin.f1 );
        int x2 = Freq_to_xSpec( fft.pkwin.f2 );

        int y1 = magDb_to_Y( fft.pkwin.m1 );
        int y2 = magDb_to_Y( fft.pkwin.m2 );

        CRect pkRect( x1, y1, x2, y2 );
        pkRect.NormalizeRect ();

        pkRect.right++;
        pkRect.bottom++;

        pkRect.IntersectRect( &rectXY, &pkRect ); 

        dc.FillSolidRect( &pkRect, colorGraphGridNote );
        }

    if ( displayWaterfall )
    {
        if ( HBITMAP( bmpSpecWaterfall ) )
        {
            BITMAP bmpb;
            bmpSpecWaterfall.GetBitmap( &bmpb );

            if ( bmpb.bmWidth != rectXY.Width() || bmpb.bmHeight != rectXY.Height() + 100 )
                bmpSpecWaterfall.DeleteObject ();
            }

        if ( ! HBITMAP( bmpSpecWaterfall ) )
        {
            bmpSpecWaterfall.CreateCompatibleBitmap( &dc, rectXY.Width(), rectXY.Height() + 100 );
            }

        CDC dc3;
        if ( HBITMAP( bmpSpecWaterfall ) && dc3.CreateCompatibleDC( &dc ) )
        {
            CBitmap* old = dc3.SelectObject( &bmpSpecWaterfall );

            if ( ! freezeWaterfall )
            {
		        dc3.BitBlt( 0, specDeltaY, rectXY.Width(), rectXY.Height() + specStartY - specDeltaY, &dc3, 0, 0, SRCCOPY );
                }

            double magLin = 0.0;
            double avgMagLin = 0.0;
            int xlast = 0;

            dc.SelectObject( &penSpectrAvg );

            for ( int k = minFreqK; k <= maxFreqK + 1; k++ )
            {
                if ( fft.lastMagLin[ k ] > magLin )
                    magLin = fft.lastMagLin[ k ];
                if ( fft.avgMagLin[ k ] > avgMagLin )
                    avgMagLin = fft.avgMagLin[ k ];

                int x = kFreq_to_xSpec( k ) - rectXY.left;

                if ( x <= xlast )
                    continue;

                if ( magLin >= maxMagLin )
                {
                    maxMagLin = magLin;
                    maxMagDb = Lin2Db( maxMagLin );
                    }

                COLORREF RGB = magDb_to_RGB( Lin2Db( avgMagLin ) );

                dc3.FillSolidRect( xlast, 0, x - xlast, specStartY - 1, RGB );

                xlast = x;
                magLin = 0.0;
                avgMagLin = 0.0;
                }

		    dc.BitBlt( rectXY.left, rectXY.top + 1 - specStartY, rectXY.Width(), rectXY.Height() + specStartY, &dc3, 0, 0, SRCCOPY );

            dc3.SelectObject( old );
            }

        dc.SelectObject( &penGrid );
        dc.MoveTo( rectXY.left, rectXY.top + 1 );
        dc.LineTo( rectXY.left, rectScale.top );
        dc.LineTo( rectXY.right, rectScale.top );
        dc.LineTo( rectXY.right, rectXY.top + 1 );

        for ( int y = rectXY.top; y < rectXY.bottom; y++ )
        {
            int RGB = magDb_to_RGB( Y_to_magDb( y ) );
            dc.FillSolidRect( rectScale.left, y, specStartX, 1, RGB );
            }
        }

    ///////////////////////////////////////////////////////////////////////////////

    OnPaintSpectralVerticalGrid( dc );

    OnPaintHorizontalGrid( dc );

    dc.IntersectClipRect( &rectXY ); // Clip to graph area

    OnPaintSpectralSelection( dc );

    ///////////////////////////////////////////////////////////////////////////

    CString str;

    dc.SelectObject( &fontText );

    if ( displayFFT )
    {
        /////////////////////////////////////////////////////////////////////////// 
        // FFT Data Display

        double magLin = 0.0;
        double avgMagLin = 0.0;
        int xlast = rectXY.left;

        dc.SetTextAlign( TA_LEFT );
        dc.SetTextColor( colorTextDefault );

        dc.SelectObject( &penSpectrAvg );

        for ( int k = minFreqK; k <= maxFreqK + 1; k++ )
        {
            if ( fft.lastMagLin[ k ] > magLin )
                magLin = fft.lastMagLin[ k ];

            if ( fft.avgMagLin[ k ] > avgMagLin )
                avgMagLin = fft.avgMagLin[ k ];

            int x = kFreq_to_xSpec( k );

            if ( k == minFreqK || x > xlast )
            {
                if ( magLin >= maxMagLin )
                {
                    maxMagLin = magLin;
                    maxMagDb = Lin2Db( maxMagLin );
                    }

                int y = magLin_to_Y( avgMagLin );
                int yP = magLin_to_Y( magLin );
            
                if ( magLin > avgMagLin )
                    dc.FillSolidRect( x, yP, 3, ( y - yP ) / 3, colorGraphSpectrTop );

                if ( k == minFreqK )
                    dc.MoveTo( x, y );
                else
                    dc.LineTo( x, y );

                xlast = x;
                magLin = 0.0;
                avgMagLin = 0.0;
                }
            }
        }

    if ( displayPeakInfo )
    {
        /////////////////////////////////////////////////////////////////////////// 
        // Harmonics to peak line

        if ( fft.peaks[fft.peakK].freq > 0.0 )
        {
            dc.SelectObject( &penGridCutOff );

            for ( int i = 1; i < 15; i++ )
            {
                int k = fft.peaks[fft.peakK].k * i; 

                if ( k < minFreqK || k > maxFreqK )
                    continue;

                int xpk = kFreq_to_xSpec( k );
                int ypk = magLin_to_Y( fft.avgMagLin[ k ] );
    
                dc.MoveTo( xpk, rectXY.bottom );
                dc.LineTo( xpk, ypk );
                }
            }

        /////////////////////////////////////////////////////////////////////////// 
        // Peaks' square marks

        for ( int k = 0; k < FFT::MAX_PEAKS; k ++ )
        {
            if ( fft.peaks[ k ].freq > 0.0 && minFreqK <= fft.peaks[ k ].k &&fft.peaks[ k ].k <= maxFreqK )
            {
                int x = Freq_to_xSpec( fft.peaks[ k ].freq );

                int y = displayMagLog 
                    ? magDb_to_yLog( fft.peaks[ k ].magDb )
                    : magLin_to_yLin( fft.peaks[ k ].magLin );

                dc.FrameRect( &CRect( CPoint( x - 1, y - 1 ), CSize( 3, 3 ) ), &CBrush( colorGraphPkMarks ) );
                }
            }

        /////////////////////////////////////////////////////////////////////////// 
        // Peak line

        if ( fft.peaks[fft.peakK].freq > 0.0 && minFreqK <= fft.peaks[fft.peakK].k && fft.peaks[fft.peakK].k <= maxFreqK )
        {
            dc.SelectObject( &penPeakLine );

            int xpk = Freq_to_xSpec( fft.peaks[fft.peakK].freq );

            int ypk;

            if ( fft.peaks[fft.peakK].freq == 0 )
            {
                ypk = magDb_to_Y( fft.cutOffMagDb );
                }
            else 
            {
                ypk = displayMagLog
                    ? magDb_to_yLog( fft.peaks[fft.peakK].magDb )
                    : magLin_to_yLin( fft.peaks[fft.peakK].magLin );
                }

            dc.MoveTo( xpk, rectXY.top );
            dc.LineTo( xpk, ypk );

            /////////////////////////////////////////////////////////////////////////// 
            // Peak line text info

            dc.SetTextColor( colorTextHighlighted );

            if ( fft.peaks[fft.peakK].freq > 0 )
            {
                dc.SetTextAlign( TA_RIGHT );
                str.Format( "%s%d", fft.noteName[fft.refNote], fft.octave );

                dc.SetTextColor( colorGraphBackground );

                for ( int x = -2; x < 3; x++ )
                    for ( int y = -2; y < 3; y++ )
                        dc.ExtTextOut( xpk - 5 + x, rectXY.top + 3 + y, 0, NULL, str, NULL );

                dc.SetTextColor( colorTextHighlighted );
                dc.ExtTextOut( xpk - 5, rectXY.top + 3, 0, NULL, str, NULL );
                dc.SetTextAlign( TA_LEFT );

                if ( -5.0 < fft.avgCents && fft.avgCents < 5.0 )
                    str.Format( "%+6.1lf ¢", fft.avgCents );
                else
                    str.Format( "%+6.lf ¢", fft.avgCents );
                dc.ExtTextOut( xpk + 5, rectXY.top + 3, 0, NULL, str, NULL );

                dc.SetTextColor( colorTextGray );

                if ( fft.avgFreq < 10e3 )
                    str.Format( "%6.1lf Hz", fft.avgFreq );
                else
                    str.Format( "%6.lf Hz", fft.avgFreq );
                dc.ExtTextOut( xpk + 5, rectXY.top + 20, 0, NULL, str, NULL );

                str.Format( "%6.1lf dB", fft.avgMagDb );
                dc.ExtTextOut( xpk + 5, rectXY.top + 37, 0, NULL, str, NULL );
                }
            else
            {
                str.Format( "%.1lf Hz", fft.avgFreq );
                dc.ExtTextOut( xpk + 5, rectXY.top + 3, 0, NULL, str, NULL );
                }
            }
        }
    }

///////////////////////////////////////////////////////////////////////////////

void CFFTWnd::OnPaintSpectralSelection( CDC& dc )
{
    if ( fft.N < 0 )
        return;

    // Pk window 
    //
    if ( fft.pkwin.active )
    {
        int x1 = Freq_to_xSpec( fft.pkwin.f1 );
        int x2 = Freq_to_xSpec( fft.pkwin.f2 );

        int y1 = magDb_to_Y( fft.pkwin.m1 );
        int y2 = magDb_to_Y( fft.pkwin.m2 );

        CRect pkRect( x1, y1, x2, y2 );
        pkRect.NormalizeRect ();

        pkRect.right++;
        pkRect.bottom++;

        dc.FrameRect( &pkRect, &CBrush( colorGraphPkWindow ) );
        }

    // Selection window 
    //
    if ( selectionActive )
    {
        int x1 = Freq_to_xSpec( startFreq );
        int x2 = Freq_to_xSpec( curFreq );

        int y1 = magDb_to_Y( startMagDb );
        int y2 = magDb_to_Y( curMagDb );

        CRect selRect( x1, y1, x2, y2 );
        selRect.NormalizeRect ();

        selRect.right++;
        selRect.bottom++;

        dc.FrameRect( &selRect, &CBrush( colorGraphSelection ) );
        }
    }