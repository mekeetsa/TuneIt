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

void CFFTWnd::OnPaintChromatic( CDC& dc )
{
    if ( fft.N < 0 )
        return;

    ///////////////////////////////////////////////////////////////////////////////

    int deltax = 1 + int( rectXY.Width () / 1300.0 );

    dc.FillSolidRect( &rectXY, colorGraphBackground );

    if ( displayWaterfall )
    {
        if ( HBITMAP( bmpChromWaterfall ) )
        {
            BITMAP bmpb;
            bmpChromWaterfall.GetBitmap( &bmpb );

            if ( bmpb.bmWidth != rectXY.Width() || bmpb.bmHeight != rectXY.Height() + 100 )
                bmpChromWaterfall.DeleteObject ();
            }

        if ( ! HBITMAP( bmpChromWaterfall ) )
        {
            bmpChromWaterfall.CreateCompatibleBitmap( &dc, rectXY.Width(), rectXY.Height() + 100 );
            }

        CDC dc3;
        if ( HBITMAP( bmpChromWaterfall ) && dc3.CreateCompatibleDC( &dc ) )
        {
            CBitmap* old = dc3.SelectObject( &bmpChromWaterfall );

            if ( ! freezeWaterfall )
		        dc3.BitBlt( 0, specDeltaY, rectXY.Width(), rectXY.Height() + specStartY - specDeltaY, &dc3, 0, 0, SRCCOPY );

            for ( int i = 0; i < 1300; i++ )
            {
                int x = int( double(i) * rectXY.Width () / 1300.0 );

                int i2 = ( i < 100 ? 1100 + i : i - 100 ) % 1200;

                double magLin = fft.avgMagLinPerCent[ i2 ];
                double magDb = Lin2Db( magLin );

                if ( magLin >= maxMagLin )
                {
                    maxMagLin = magLin;
                    maxMagDb = magDb;
                    }

                int RGB = magDb_to_RGB( magDb ); 

                dc3.FillSolidRect( x, 0, deltax, specStartY - 1, RGB );
                }

            static last_refNote = -1;
            if ( last_refNote != fft.refNote )
            {
                //dc3.FillSolidRect( 0, specStartY, rectXY.Width (), 1, colorGraphGrid );

                int x = int( double(fft.refNote*100 + 50) * rectXY.Width () / 1300.0 + 0.5 );
                int xLen = int( double(100) * rectXY.Width () / 1300.0 + 0.5 );

                dc3.FillSolidRect( x, specStartY + 1, xLen, 1, colorTextHighlighted );

                last_refNote = fft.refNote;
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

    ///////////////////////////////////////////////////////////////////////////

    CString str;

    ///////////////////////////////////////////////////////////////////////////

    dc.SetBkMode( TRANSPARENT );
    dc.SetTextColor( colorTextDefault );
    dc.SelectObject( &penGrid );

    /////////////////////////////////////////////////////////////////////////// 
    // Vertical frequency lines

    dc.SetTextAlign( TA_CENTER );

    dc.SelectObject( &penGridNote );
    dc.MoveTo( rectXY.left, rectXY.bottom );
    dc.LineTo( rectXY.left, rectXY.top );
    dc.MoveTo( rectXY.right, rectXY.bottom );
    dc.LineTo( rectXY.right, rectXY.top );

    for ( int i = 0; i < 12; i++ )
    {
        int x = Cents_to_xChrom( i * 100 );
        int x1 = Cents_to_xChrom( i * 100 - 50 );
        int x2 = Cents_to_xChrom( i * 100 + 50 );

        if ( fft.refNote == i && ! displayWaterfall )
        {
            dc.FillSolidRect( &CRect( x1, rectXY.top, x2, rectXY.bottom ), colorGraphGridNote );
            }

        dc.SelectObject( &penGrid );
        dc.MoveTo( x1, rectXY.bottom );
        dc.LineTo( x1, rectXY.top );
        dc.MoveTo( x2, rectXY.bottom );
        dc.LineTo( x2, rectXY.top );

        if ( fft.refNote == i )
        {
            CRect r( x1, rectScale.bottom - 1, x2 + 1, rectGraph.bottom );
            dc.FillSolidRect( &r, colorGraphGridNote );
            dc.FrameRect( &r, &CBrush( colorGraphGrid ) );

            r.top = rectGraph.top;
            r.bottom = rectXY.top - 1;
            if ( displayWaterfall )
                r.bottom -= specStartY;

            dc.FillSolidRect( &r, colorGraphGridNote );
            dc.FrameRect( &r, &CBrush( colorGraphGrid ) );

            r.top = r.bottom;
            r.bottom = r.top + 1;
            r.left = x;
            r.right = x + 1;
            dc.FillSolidRect( &r, colorGraphCents ); 
            }

        if ( ! displayWaterfall )
        {
            if ( fft.refNote == i )
                dc.SelectObject( &penGrid );
            else
                dc.SelectObject( &penGridNote );

            dc.MoveTo( x, rectXY.bottom );
            dc.LineTo( x, rectXY.top );
            }

        CString str;
        str.Format( "%s", fft.noteName[ i % 12 ] );

        if ( fft.refNote == i )
        {
            dc.SetTextColor( colorTextHighlighted );
            dc.SelectObject( &fontTextBold );
            }
        else
        {
            dc.SetTextColor( colorTextDefault );
            dc.SelectObject( &fontText );
            }

        dc.ExtTextOut( x, rectXY.bottom + 2, ETO_OPAQUE, NULL, str, NULL );
        dc.ExtTextOut( x, rectGraph.top + 2, ETO_OPAQUE, NULL, str, NULL );
        dc.SelectObject( &fontText );
        }

    OnPaintHorizontalGrid( dc );

    dc.IntersectClipRect( &rectXY ); // Clip to graph area

    dc.SelectObject( &fontText );

    if ( displayFFT )
    {
        /////////////////////////////////////////////////////////////////////////// 
        // FFT Data Display

        dc.SetTextAlign( TA_LEFT );
        dc.SetTextColor( colorTextDefault );

        dc.SelectObject( &penChromAvg );

        for ( i = 0; i < 1300; i++ )
        {
            int x = Cents_to_xChrom( i - 100 );
            int i2 = ( i < 100 ? 1100 + i : i - 100 ) % 1200;

            ////////////////////////////////////////
            double magLin = fft.avgMagLinPerCent[ i2 ];
            double magDb = Lin2Db( magLin );

            if ( magLin >= maxMagLin )
            {
                maxMagLin = magLin;
                maxMagDb = magDb;
                }

            int y = displayMagLog
                ? magDb_to_yLog( magDb )
                : magLin_to_yLin( magLin );

            int yP = magLin_to_Y( fft.maxMagLinPerCent[ i2 ] );
        
            if ( yP < y )
            {
                dc.FillSolidRect( 
                    x - 1, yP, deltax + 2, ( y - yP ) / 3 , 
                    colorGraphSpectrTop 
                    );
                }

            if ( y < rectXY.bottom )
            {
                dc.FillSolidRect( 
                    x - 1, y, deltax + 2, rectXY.bottom - y, 
                    i < 50 || i > 1250 ? colorGraphChromAvg / 2 : colorGraphChromAvg 
                    );
                }

            }
        }

    if ( displayPeakInfo )
    {
        /////////////////////////////////////////////////////////////////////////// 
        // Peaks' square marks

        for ( int k = 0; k < FFT::MAX_PEAKS; k ++ )
        {
            if ( fft.peaks[ k ].freq > 0.0 )
            {
                double cents = Freq_to_Cents( fft.peaks[ k ].freq );

                int x = Cents_to_xChrom( cents );

                int y = displayMagLog
                    ? magDb_to_yLog( fft.peaks[ k ].magDb )
                    : magLin_to_yLin( fft.peaks[ k ].magLin );

                dc.FrameRect( &CRect( CPoint( x - 1, y - 1 ), CSize( 3, 3 ) ), &CBrush( colorGraphPkMarks ) );
                }
            }

        if ( fft.peaks[fft.peakK].freq > 0.0 )
        {
            dc.SelectObject( &penPeakLine );

            double cents = Freq_to_Cents( fft.peaks[ fft.peakK ].freq );

            int x = Cents_to_xChrom( cents );

            int y = displayMagLog
                ? magDb_to_yLog( fft.peaks[ fft.peakK ].magDb )
                : magLin_to_yLin( fft.peaks[ fft.peakK ].magLin );

            if ( y < rectXY.top + 16 )
                y = rectXY.top + 16;

            dc.MoveTo( x, rectXY.top + 1 );
            dc.LineTo( x, y );

            // cents guide

            x = Cents_to_xChrom( fft.refNote * 100 );

            dc.SetBkColor( colorGraphGridNote );

            if ( 1.0 <= fft.avgCents && fft.avgCents < 50.0 )
            {
                str.Format( "<%+2.lf", fft.avgCents );

                dc.SetTextColor( colorGraphBackground );

                for ( int a = -2; a < 3; a++ )
                    for ( int b = -2; b < 3; b++ )
                        dc.ExtTextOut( x + 1 + a, rectXY.top + 3 + b, ETO_OPAQUE, NULL, str, NULL );

                dc.SetTextColor( colorGraphCents );
                dc.ExtTextOut( x + 1, rectXY.top + 3, ETO_OPAQUE, NULL, str, NULL );
                }
            else if ( -50.0 < fft.avgCents && fft.avgCents <= -1.0 )
            {
                str.Format( "%+2.lf>", fft.avgCents );

                dc.SetTextAlign( TA_RIGHT );
                dc.SetTextColor( colorGraphBackground );

                for ( int a = -2; a < 3; a++ )
                    for ( int b = -1; b < 2; b++ )
                        dc.ExtTextOut( x - 1 + a, rectXY.top + 3 + b, ETO_OPAQUE, NULL, str, NULL );

                dc.SetTextColor( colorGraphCents );
                dc.ExtTextOut( x - 1, rectXY.top + 3, ETO_OPAQUE, NULL, str, NULL );

                dc.SetTextAlign( TA_LEFT );
                }
            else if ( fft.avgCents > -1.0 && fft.avgCents < 1.0 )
            {
                dc.ExtTextOut( x + 3, rectXY.top + 3, ETO_OPAQUE, NULL, "<", 1, NULL );
                dc.SetTextAlign( TA_RIGHT );
                dc.ExtTextOut( x - 1, rectXY.top + 3, ETO_OPAQUE, NULL, ">", 1, NULL );
                dc.SetTextAlign( TA_LEFT );
                }

            dc.SetTextColor( colorTextDefault );
            dc.SetBkMode( TRANSPARENT );
            }
        }
    }

