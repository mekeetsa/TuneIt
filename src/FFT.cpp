// CFFTWnd.cpp : implementation file
//

#include "stdafx.h"
#include "TuneIt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////

const char* FFT::noteName[ 12 ] 
    = { "C","C#","D","D#","E","F","F#", "G", "G#", "A","A#","H" };

///////////////////////////////////////////////////////////////////////////////

void FFT::Measure
( 
    int nSamples, 
    short int* inData
    )
{
    mutex.Lock ();

    if ( N <= 0 )
    {
        mutex.Unlock ();
        return;
        }

    for ( int i = 0; i < nSamples; i++ ) 
    {
        ++tClock;

        // Normalize sample to be in between [ -1.0, +1.0 ]
        //
        double sample = double( inData[ i ] ) / PCM_RANGE;

        // Keep VU meter peak and RMS running sum
        //
        vuRmsSum += sample * sample;
        if ( fabs( sample ) > vuPkMax )
            vuPkMax = fabs( sample );

        // Input gain (usually needed to compensate windowing function)
        //
        sample *= inputGain; 

        // High-Pass Filter
        //
        if ( HPF_enabled )
            sample = HPF_15Hz.filter( sample );

        // Low-Pass Filter
        //
        if ( LPF_enabled )
            sample = LPF_20kHz.filter( sample );

        // Store sample in FFT buffer
        //
        if ( planStatus == PLAN_INITIALIZED )
        {
            if ( sampleCount < N * 2 - 1 )
            {
                sampleBuffer[ sampleCount++ ] = sample;
                }
            }
        else
        {
            sampleBuffer[ sampleCount++ ] = sample;
            }
        }

    vuBar *= 0.99;
    if ( vuPkMax > vuBar )
        vuBar = vuPkMax;

    vuPk += ( vuPkMax - vuPk ) / 2.0;
    vuRms += ( sqrt( vuRmsSum / stepN ) - vuRms ) / 15.0;

    vuRmsSum = 0.0;
    vuPkMax = 0.0;

    if ( planStatus == PLAN_INITIALIZED )
    {
        PostThreadMessage( id, WM_USER, 0, 0 );
        }
    else
    {
        int overflow = sampleCount - N;
        if ( overflow > 0 )
        {
            sampleCount -= overflow;

            memmove( sampleBuffer, sampleBuffer + overflow,
                sampleCount * sizeof( sampleBuffer[0] )
                );
            }
        }

    mutex.Unlock ();
    }

///////////////////////////////////////////////////////////////////////////////

DWORD FFT::Main( void )
{
    Trace( "%s::Main\n", objID );

    SetThreadPriority( THREAD_PRIORITY_IDLE );

    InitializePlan ();

    if ( N < 0 )
        return 0;

    SetThreadPriority( THREAD_PRIORITY_NORMAL );

    SetTimer( NULL, 0, 15, NULL );

    MSG msg;
    while( ! fTerminating )
    {
        if ( 1 != GetMessage( &msg, NULL, 0, 0 ) )
        {
            break;
            }

        mutex.Lock ();

        if ( N <= 0 )
        {
            mutex.Unlock ();
            break;
            }

        if ( msg.message == WM_TIMER )
        {
            DoExpAverage ();
            }
        else if ( msg.message == WM_USER )
        {
            while ( sampleCount >= N )
            {
                DoFFT ();
                FindPeaks ();

                sampleCount -= stepN;

                memmove( sampleBuffer, sampleBuffer + stepN,
                    sampleCount * sizeof( sampleBuffer[0] )
                    );
                }
            }

        mutex.Unlock ();
        }

    Trace( "%s::Main COMPLETED\n", objID );

    return 0;
    }

///////////////////////////////////////////////////////////////////////////////

void FFT::DoFFT( void )
{
    for ( int k = 0; k < N; k++ ) 
    {
        in[k] = float( sampleBuffer[k] * window[ k ] );
        }

    if ( N == 16384 )
    {
        fftwf_execute( plan_16384 );
        }
    else if ( N == 8192  )
    {
        fftwf_execute( plan_8192 );
        }
    else if ( N == 4096 )
    {
        fftwf_execute( plan_4096 );
        }
    else
    {
        memset( out, 0, sizeof( float ) * ( MAX_FFT_SIZE + 2 ) );
        }
    }

///////////////////////////////////////////////////////////////////////////////

void FFT::FindPeaks( void )
{
    double phaseDifference = 2.0 * M_PI * double(stepN) / double(N);

    memset( maxMagLinPerCent, 0, sizeof( maxMagLinPerCent ) );

    for ( int i = 0; i < MAX_PEAKS; i++ )
    {
	    peaks[i].magDb = -300.0;
	    peaks[i].freq = 0.0;
        }

    peakFreq = 0.0;

    for ( int k = 0; k <= len; k++ )
    {
	    double real = out[ k ][ 0 ];
	    double imag = out[ k ][ 1 ];

        // Compute phase difference 
        //
	    double phase = atan2( imag, real );
        double tmp = phase - lastPhase[ k ];
        lastPhase[ k ] = phase;

        // Subtract expected phase difference 
        //
        tmp -= double( k ) * phaseDifference;

        // Map delta phase into +/- Pi interval 
        //
        long qpd = long( tmp / M_PI );
        if ( qpd >= 0 ) 
            qpd += ( qpd & 1 );
        else 
            qpd -= ( qpd & 1 );

        tmp -= M_PI * double( qpd );

        // Get deviation from bin frequency from the +/- Pi interval 
        //
        tmp /= phaseDifference;

        // Compute the k-th partials' true frequency and magnitude
        //
        double freq = double(k) * freqPerBin + tmp * freqPerBin;
        double magLin = 2.0 * sqrt( real * real + imag * imag ) / N;

        // Remove noise
        //
        if ( noiseFilter == NOISEF_ENABLED && noiseLevelMagLin[ k ] > 1.0 )
            magLin /= noiseLevelMagLin[ k ];

        // Compute magnitude in dB
        //
	    double magDb = Lin2Db( magLin );

        lastMagLin[ k ] = magLin;
        lastFreq[ k ] = freq;

        // Map frequency to cents and keep max magnitude found per cent
        //
        if ( freq > 16.0 && freq < 8.0e3 )
        {
            int cents = int( 1200.0 * log( freq / noteFreq[0] ) / LOG_2 + 0.5 ) % 1200;
            if ( cents < 0 ) 
                cents += 1200;

            if ( magLin > maxMagLinPerCent[ cents ] )
                maxMagLinPerCent[ cents ] = magLin;
            }

        // Keep updated magnitude peaks
        //
        if ( magDb > peaks[0].magDb )
        {
            if ( pkwin.active )
            {
                if ( pkwin.f1 < freq && freq < pkwin.f2
                    && pkwin.m1 < magDb /*&& magDb < pkwin.m2*/
                    ) 
                {
                    memmove( &peaks[1], &peaks[0], sizeof(peaks) - sizeof(peaks[0]) );
	                peaks[0].freq = freq;
	                peaks[0].magDb = magDb;
                    peaks[0].k = k;
                    peaks[0].magLin = magLin;
                    }
                }
            else if ( freq > 16.0 && freq < 8.0e3 && magDb > cutOffMagDb ) 
            {
                memmove( &peaks[1], &peaks[0], sizeof(peaks) - sizeof(peaks[0]) );
	            peaks[0].freq = freq;
	            peaks[0].magDb = magDb;
                peaks[0].k = k;
                peaks[0].magLin = magLin;
	            }
            }
        }

    // Find base harmonic of the peak
    //
    peakK = 0;

    for ( int j = 1, maxHarm = 1; j < MAX_PEAKS && peaks[j].freq > 0.0; j++ )
    {
        int harmonic = int( peaks[0].freq / peaks[j].freq + 0.5 );
        double delta = peaks[0].freq / peaks[j].freq - harmonic;

        if ( maxHarm < harmonic && harmonic < 14 && -0.02 <= delta && delta <= 0.02 )
        {
            maxHarm = harmonic;
            peakK = j;
            }
        }

    peakFreq = peaks[ peakK ].freq;
    if ( peakFreq < 1e-15 )
        peakFreq = 1e-15;

    if ( ( pkwin.active 
          && pkwin.f1 < peakFreq && peakFreq < pkwin.f2
          && pkwin.m1 < peaks[ peakK ].magDb /*&& peaks[ peakK ].magDb < pkwin.m2*/ )
          || ( peakFreq > 15.0 && peaks[ peakK ].magDb > cutOffMagDb ) 
        )
    {
        // Normalize frequency to be in a +/- 500 cents to reference note
        //
        while ( centerFreq / peakFreq > CENTS_500 )
            centerFreq /= 2.0;
        while ( peakFreq / centerFreq > CENTS_500 )
            centerFreq *= 2.0;
        octave = int( refOctave + log( centerFreq / noteFreq[0] ) / LOG_2 );

        // Find new reference note
        //
        if ( ! refNoteLocked )
        {
            double lfreq = log(peakFreq);
            while (lfreq < noteFreqLog[0]-LOG_CENTS_100/2.)
                lfreq += LOG_2;
            while (lfreq >= noteFreqLog[0]+LOG_2-LOG_CENTS_100/2.)
                lfreq -= LOG_2;

            int newRefNote = refNote;

            double mldf = LOG_CENTS_100;
            for ( int i = 0; i < 12; i++ ) 
            {
                double ldf = fabs( lfreq-noteFreqLog[ i ] );
                if ( ldf < mldf ) 
                {
                    mldf = ldf;
                    newRefNote = i;
                    }
                }

            if ( newRefNote != refNote )
            {
                refNote = newRefNote;
                refFreq = noteFreq[ refNote ];

                centerFreq = refFreq;
                while ( centerFreq / peakFreq > CENTS_50 )
                    centerFreq /= 2.0;
                while ( peakFreq / centerFreq > CENTS_50 )
                    centerFreq *= 2.0;

                octave = int( refOctave + log( centerFreq / noteFreq[0] ) / LOG_2 );

                char* line = "";
#if 0
                Trace( "      |   |   |   |   |    \n" );

                switch( refNote )
                {
                    case  0: line = "  O   |   |   | O |   |    "; break;
                    case  1: line = " #O   |   |   |#O |   |    "; break;
                    case  2: line = "    O |   |   |   O   |    "; break;
                    case  3: line = "   #O |   |   |  #O   |    "; break;
                    case  4: line = "      O   |   |   | O |    "; break;
                    case  5: line = "      | O |   |   |   O    "; break;
                    case  6: line = "      |#O |   |   |  #O    "; break;
                    case  7: line = "      |   O   |   |   | O  "; break;
                    case  8: line = "      |  #O   |   |   |#O  "; break;
                    case  9: line = "      |   | O |   |   |    "; break;
                    case 10: line = "      |   |#O |   |   |    "; break;
                    case 11: line = "      |   |   O   |   |    "; break;
                    }                            
#endif
                Trace( "%s  %s%*d %7.1lf Hz\n", 
                    line, noteName[refNote], strlen(noteName[refNote]) - 3, 
                    octave, centerFreq );
                }
            }
        }
    }

///////////////////////////////////////////////////////////////////////////////

void FFT::DoExpAverage( void )
{
    // Calculate peak averages
    //
    if ( ( pkwin.active 
          && pkwin.f1 < peakFreq && peakFreq < pkwin.f2
          && pkwin.m1 < peaks[ peakK ].magDb /*&& peaks[ peakK ].magDb < pkwin.m2*/ )
          || ( peakFreq > 15.0 && peaks[ peakK ].magDb > cutOffMagDb ) 
        )
    {
        double cents = 1200.0 * log( peakFreq / centerFreq ) / LOG_2;

        if ( lastCenterFreq != centerFreq )
        {
            avgCents = cents;
            avgFreq = peakFreq;
            avgMagDb = peaks[ peakK ].magDb;
            lastCenterFreq = centerFreq;
            }

        avgCents += ( cents - avgCents ) / ( longExpAvgTau ? 25.0 : 5.0 );
        avgFreq += ( peakFreq - avgFreq ) / ( longExpAvgTau ? 25.0 : 5.0 );
        avgMagDb += ( peaks[ peakK ].magDb - avgMagDb ) / ( longExpAvgTau ? 25.0 : 5.0 );
#if 0
        Trace( "pkF=%6.1lf ctF=%6.1lf avF=%6.1lf Hz, C=%+5.1lf avC=%+5.1lf, %5.1lf dB\n", 
            peakFreq, centerFreq, avgFreq, cents, avgCents, avgMagDb );
#endif
        }

    // Compute exponential average of the magnitude
    //
    for ( int k = 0; k < len; k++ )
    {
        if ( noiseFilter == NOISEF_LEARNING )
            avgMagLin[ k ] += ( lastMagLin[ k ] - avgMagLin[ k ] ) / noiseTau;
        else
            avgMagLin[ k ] += ( lastMagLin[ k ] - avgMagLin[ k ] ) / ( longExpAvgTau ? 25.0 : 5.0 );
        }

    for ( int i = 0; i < 1200; i++ )
    {
        avgMagLinPerCent[ i ] += ( maxMagLinPerCent[ i ] - avgMagLinPerCent[ i ] ) / ( longExpAvgTau ? 30.0 : 8.0 );
        }

    if ( noiseFilter == NOISEF_LEARNING )
    {
        noiseTau += 1.0;

        if ( noiseTau > 200.0 )
        {
            for ( int i = 0; i < len; i++ )
                noiseLevelMagLin[ i ] = PCM_RANGE * avgMagLin[ i ];

            noiseFilter = NOISEF_ENABLED;
            noiseTau = 1.0;
            }

        cutOffMagDb = -70.0;
        }
    }

///////////////////////////////////////////////////////////////////////////////

void FFT::SetRefNote( int note, int oct )
{
    if ( oct < 0 )
        oct = refOctave;

    pkwin.active = false; 

    if ( note < 0 || note >= 12 )
    {
        refNoteLocked = false;
        }
    else
    {
        refNoteLocked = true;
        refNote = note;
        refFreq = noteFreq[ refNote ];
        for( ; oct < refOctave; oct++ )
            refFreq /= 2.0;
        for( ; oct > refOctave; oct-- )
            refFreq *= 2.0;
        centerFreq = refFreq;
        octave = int( refOctave + log( centerFreq / noteFreq[0] ) / LOG_2 );
        }

    Trace( "RefNote: %d, Locked: %s\n", refNote, refNoteLocked ? "true" : "false" );
    }

///////////////////////////////////////////////////////////////////////////////

void FFT::SetRefNote( const char* name, int oct )
{
    pkwin.active = false; 

    int note = -1;

    for ( int i = 0; i < 12 && name; i++ )
    {
        if ( stricmp( name, noteName[ i ] ) == 0 )
        {
            note = i;
            break;
            }
        }

    SetRefNote( note, oct );
    }

