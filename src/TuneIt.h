//
//  TuneIt - Spectrum Analyzer and Musical Instrument Tuner
//
//  Copyright (C) 2007 by Mikica B Kocic
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
//  MA  02110-1301  USA
//
//
#if !defined(AFX_TUNEIT_H__3C9748BF_F74E_4A34_8287_0C3DF519F9B2__INCLUDED_)
#define AFX_TUNEIT_H__3C9748BF_F74E_4A34_8287_0C3DF519F9B2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

class CFFTWnd;
class CTuineItApp;

///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fftw3.h"

#include "FlatButton.h"

///////////////////////////////////////////////////////////////////////////////

class CPU_Usage
{
    DWORD last_ms;
    LONGLONG last_T;
    double cpuUsage;

    HANDLE hThread;
    HANDLE hProcess;

public:

    void Initialize( void )
    {
        last_ms = GetTickCount ();
        last_T = 0;
        cpuUsage = 0.0;
        hThread = NULL;
        }

    CPU_Usage( void )
    {
        Initialize ();
        }

    void Update( void )
    {
        DWORD msSinceBoot = GetTickCount ();
        DWORD ms = msSinceBoot > last_ms
            ? msSinceBoot - last_ms
            : last_ms - msSinceBoot;

        if ( ms < 1000.0 )
            return;

        last_ms = msSinceBoot;

        FILETIME creationTime, exitTime;
        union { FILETIME Time; LONGLONG T; } k;
        union { FILETIME Time; LONGLONG T; } u;

        if ( hProcess != NULL )
        {
            if ( ! GetProcessTimes( hProcess, &creationTime, &exitTime, &k.Time, &u.Time ) )
                return;
            }
        else if ( hThread != NULL )
        {
            if ( ! GetThreadTimes( hThread, &creationTime, &exitTime, &k.Time, &u.Time ) )
                return;
            }
        else
        {
            return;
            }

        LONGLONG T = ( u.T + k.T ) / 10000u; // convert from 100ns to 1ms granularity
        cpuUsage = double( T - last_T ) * 100.0 / ms; // in %
        last_T = T;
        }

    void SetThread( HANDLE hT )
    {
        Initialize ();
        hThread = hT;
        }

    void SetProcess( HANDLE hP )
    {
        Initialize ();
        hProcess = hP;
        }

    operator double( void )
    {
        Update ();
        return cpuUsage;
        }
    };


///////////////////////////////////////////////////////////////////////////////

class Thread
{
private:

    HANDLE handle;
    volatile bool isRunning;

    virtual DWORD Main( void ) = 0;

    static DWORD _stdcall ThreadWrapper( LPVOID lpContext )
    {
        if ( ! lpContext )
            return 0;

        Thread* thisP = (Thread*)lpContext;
        thisP->isRunning = true;
        DWORD rc = thisP->Main();
        thisP->isRunning = false;

        return rc;
        }

protected:

    DWORD id;
    TCHAR* objID;
    volatile bool fTerminating;

public:

    CPU_Usage cpuUsage;

    Thread( TCHAR* id )
    {
        objID = id;
        Trace( "%s::Thread.Construct\n", objID );

        isRunning = false;
        fTerminating = false;
        handle = NULL;
        id = 0;
        }

    ~Thread( void )
    {
        Trace( "%s::Thread.Destruct\n", objID );

        StopThread ();
        cpuUsage.SetThread( NULL );
        }

    bool StartThread( void )
    {
        Trace( "%s::Thread.StartThread\n", objID );

        isRunning = true;
        fTerminating = false;
        handle = CreateThread( NULL, 0, ThreadWrapper, this, 0, &id );
        cpuUsage.SetThread( handle );
        return handle != NULL;
        }

    bool StopThread( long dwTimeout = 2000 )
    {
        Trace( "%s:Thread.StopThread\n", objID );

        if ( handle != NULL )
        {
            Trace( "%s:Thread.WaitForThread to stop\n", objID );
            fTerminating = true;
            PostThreadMessage( id, WM_QUIT, 0, 0 );

            while ( isRunning && dwTimeout > 0 )
            {
                Sleep( 10 );
                dwTimeout -= 10;
                }
            }

        if ( ! isRunning )
        {
            Trace( "%s::Thread.StopThread completed\n", objID );
            }
        else
        {
            Trace( "%s::Thread is still running\n", objID );
            }

        cpuUsage.SetThread( handle );

        return ! isRunning;
        }

    void KillThread( void )
    {
        if ( handle != NULL )
        {
            TerminateThread( handle, 0 );
            CloseHandle( handle );
            handle = NULL;
            }

        cpuUsage.SetThread( handle );
        }

    bool SetThreadPriority( int priority )
    {
        Trace( "%s::Thread.SetThreadPriority\n", objID );

        if ( ! handle )
            return false;

        return ::SetThreadPriority( handle, priority ) != FALSE;
        }

    bool WaitThread( long dwTimeout = 2000 )
    {
        return WaitForSingleObject( handle, dwTimeout ) == WAIT_OBJECT_0;
        }

    HANDLE GetHandle( void ) const
    {
        return handle;
        }
    };

///////////////////////////////////////////////////////////////////////////////

class Mutex
{
    CRITICAL_SECTION cs;
public:

    Mutex() { ::InitializeCriticalSection( &cs ); }

    ~Mutex() { ::DeleteCriticalSection( &cs ); }

    void Lock() { ::EnterCriticalSection( &cs ); }

    void Unlock() { ::LeaveCriticalSection( &cs ); }
    };

///////////////////////////////////////////////////////////////////////////////


enum CMxType
{
    CMX_TYPE_CONTROL,
    CMX_TYPE_SRC_LINE,
    CMX_TYPE_DST_LINE,
    CMX_TYPE_DEVICE
    };

struct CMxReport
{
    CMxType type;
    CMxReport( CMxType t ) { type = t; }
    virtual void Report( CString& str ) = 0;
    };

struct CMxControl : public CMxReport
{
    UINT uDevID;
    HMIXEROBJ hmxobj;
    LPMIXERLINE pmxldest;
    MIXERLINE mxl;
    MIXERCONTROL mxc;
    MIXERCONTROLDETAILS mxcd;
    MIXERCONTROLDETAILS_LISTTEXT* mxcd_lt;

    int cChannels;
    BOOL fDisabled;
    BOOL fMultiple;
    BOOL fUniform;
    int len;
    DWORD cSteps;
    int cMultipleItems;
    int cItems;

    CMxControl( HMIXER hmx, DWORD dwControlID, LPMIXERLINE p );
    ~CMxControl ();
    void InsertIntoTree( CTreeCtrl& m_MxT, CListCtrl& m_MxL, HTREEITEM ht );
    void Report( CString& str );
    void Update ();
    void Retrieve ();
    };

struct CMxSrcLine : public CMxReport
{
    HMIXEROBJ hmxobj;
    MIXERLINE mxl;
    CMxControl** pCtrl;
    int cCtrl;

    BOOL fSource;
    BOOL fActive;
    BOOL fDisconnected;

    CMxSrcLine( HMIXER hmx, DWORD dwLineID, LPMIXERLINE p );
    ~CMxSrcLine ();

    void InsertIntoTree( CTreeCtrl& m_MxT, CListCtrl& m_MxL, HTREEITEM ht );
    void Report( CString& str );
    };

struct CMxDstLine : public CMxReport
{
    HMIXEROBJ hmxobj;
    MIXERLINE mxl;
    CMxControl** pCtrl;
    int cCtrl;

    CMxSrcLine** pLine;
    int cLine;

    BOOL fSource;
    BOOL fActive;
    BOOL fDisconnected;

    CMxDstLine( HMIXER hmx, DWORD dwLineID );
    ~CMxDstLine ();

    void InsertIntoTree( CTreeCtrl& m_MxT, CListCtrl& m_MxL, HTREEITEM ht );
    void Report( CString& str );
    };

struct CMxDevice : public CMxReport
{
    UINT uDevID;
    HMIXEROBJ hmxobj;
    MIXERCAPS mxcaps;

    CMxDstLine** pLine;
    int cLine;

    CMxDevice( UINT uDevID, CWnd* wnd );
    ~CMxDevice ();

    void InsertIntoTree( CTreeCtrl& m_MxT, CListCtrl& m_MxL );
    void Report( CString& str );
    };



///////////////////////////////////////////////////////////////////////////////
// Floating-point constants 
//
const double CENTS_500      = pow( 2.0, 6.0/12.0 ); // 500 cents = 1 half octave
const double CENTS_100      = pow( 2.0, 1.0/12.0 ); // 100 cents = 1 half-note
const double CENTS_50       = pow( 2.0, 1.0/24.0 ); // 50 cents
const double LOG_CENTS_100  = log( pow( 2.0, 1.0/12.0 ) );
const double LOG_2          = log( 2 );
const double PCM_RANGE      = 32767.0; // sample range
const double DB_RANGE       = 20.0 * log10( PCM_RANGE ); // sample range in dB
const double M_PI           = 3.1415926535897932384626433833;
const double MAG_minus3dB   = pow( 10.0, -3.0/20.0 ); // -3dB

//////////////////////////////////////////////////////////////////////////////
// Audio interface

class WaveInBuf
    : public Thread
{
    CFFTWnd* fft;
    UINT deviceID;
    volatile bool measureFFT;

    virtual DWORD Main( void );

public:

    WaveInBuf::WaveInBuf( void )
        : Thread( "WaveInBuf" )
    {
        Trace( "%s::WaveInBuf\n", objID );

        deviceID = WAVE_MAPPER;
        measureFFT = true;
        fft = NULL;
        }

    void SetParent( CFFTWnd* parent = NULL )
    {
        fft = parent;
        }

    WaveInBuf::~WaveInBuf( void )
    {
        Trace( "%s::~WaveInBuf\n", objID );
        }

    void TurnOffMeasureFFT ()
    {
        measureFFT = false;
        }

    void TurnOnMeasureFFT ()
    {
        measureFFT = true;
        }
    };

class WaveOutBuf
    : public Thread
{
    struct WaveGen
    {
        volatile bool fadeOut;
        volatile double freq;
        volatile double volume;
        double targetVolume;
        double targetFreq;
        double T;
        double t;
        double sample;

        void Initialize( unsigned int sampleRate )
        {
            T = 1.0 / sampleRate;
            t = 0.0;
            freq = 0.0;
            volume = 0.0;
            targetFreq = 0.0;
            targetVolume = 0.0;
            fadeOut = false;
            sample = 0.0;
            }

        void OnEverySample( void )
        {
            t += T;

            const double volInc = 1.0 / 48.0 / 100.0; // fade-out in 100 ms

            if ( fadeOut )
            {
                // Fade out be 1/48000/volInc seconds long
                //
                volume -= volInc;

                if ( volume < volInc )
                {
                    fadeOut = false;
                    volume = 0.0;
                    freq = targetFreq;
                    }
                }
            else if ( freq != targetFreq )
            {
                fadeOut = true;
                }
            else
            {
                if ( volume < targetVolume - volInc )
                    volume += volInc;
                else if ( volume > targetVolume - volInc )
                    volume -= volInc;
                else
                    volume = targetVolume;
                }

            sample = volume * PCM_RANGE * sin( t * 2.0 * M_PI * freq );

            // Saturate sample
            //
            if ( sample > PCM_RANGE ) 
                sample = int( PCM_RANGE );
            else if ( sample < -PCM_RANGE ) 
                sample = -int( PCM_RANGE );
            }

        double GetSample( void )
        {
            if ( freq <= 0.0 )
                return 0.0;
            
            return sample;
            }
        };

    CFFTWnd* fft;
    UINT deviceID;
    volatile bool measureFFT;

    WaveGen gen1;
    WaveGen gen2;

    virtual DWORD Main( void );

public:

    double GetPlayingFreq( void ) const
    {
        if ( gen1.targetFreq > 0.0 )
            return gen1.targetFreq;

        return gen2.targetFreq;
        }

    double GetPlayingVolume( void ) const
    {
        if ( gen1.targetFreq > 0.0 )
            return gen1.volume;

        return gen2.volume;
        }

    void SetGenerator( double freq = 0.0, double volume = 0.0 )
    {
        Trace( "Playing: %.1lf Hz, vol = %.1lf %%\n", freq, volume * 100.0 );
        if ( ! gen1.fadeOut && gen1.freq == 0.0 ) // GEN1 is free
        {
            gen1.targetFreq = freq;
            gen1.targetVolume = volume;
            gen2.targetFreq = 0.0;
            gen2.targetVolume = 0.0;
            }
        else
        {
            gen1.targetFreq = 0.0;
            gen1.targetVolume = 0.0;
            gen2.targetFreq = freq;
            gen2.targetVolume = volume;
            }
        }

    WaveOutBuf::WaveOutBuf( void )
        : Thread( "WaveOutBuf" )

    {
        Trace( "%s::WaveOutBuf\n", objID );

        deviceID = WAVE_MAPPER;
        measureFFT = false;
        fft = NULL;
        }

    void SetParent( CFFTWnd* parent = NULL )
    {
        fft = parent;
        }

    WaveOutBuf::~WaveOutBuf( void )
    {
        Trace( "%s::~WaveOutBuf\n", objID );
        }

    void TurnOffMeasureFFT ()
    {
        measureFFT = false;
        }

    void TurnOnMeasureFFT ()
    {
        measureFFT = true;
        }
    };

///////////////////////////////////////////////////////////////////////////////
// 
//  Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
//  Command line: mkfilter -Be -Hp -o 1 -a 3.1250000000e-04 0.0000000000e+00 -l 
//  URL: http://www-users.cs.york.ac.uk/~fisher/mkfilter/trad.html
//
//  filtertype  =  Bessel  
//  passtype    =  Highpass  
//  order       =  2  
//  sampleRate  =  48000 Hz 
//  corner1     =  15 Hz
//

class HPF_15Hz
{
    enum { NZEROS = 2 };
    enum { NPOLES = 2 };

    double xv[ NZEROS + 1 ];
    double yv[ NPOLES + 1 ];

public:

    HPF_15Hz( void )
    {
        for ( int i = 0; i < NZEROS + 1; i++ )
            xv[ i ] = 0.0;
        for ( i = 0; i < NPOLES + 1; i++ )
            yv[ i ] = 0.0;
        }

    double filter( double input_value )
    {
        xv[0] = xv[1]; xv[1] = xv[2]; 
        xv[2] = input_value  / 1.001337397e+00;

        yv[0] = yv[1]; yv[1] = yv[2]; 
        yv[2] = ( xv[0] + xv[2] ) - 2.0 * xv[1]
              + ( -0.9973299684 * yv[0] ) + ( 1.9973275889 * yv[1] );

        return yv[2];
        }
    };

///////////////////////////////////////////////////////////////////////////////
// 
//  Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
//  Command line: mkfilter -Be -Lp -o 2 -a 4.1666666667e-01 0.0000000000e+00 -l
//  URL: http://www-users.cs.york.ac.uk/~fisher/mkfilter/trad.html
//
//  filtertype  =  Bessel  
//  passtype    =  Lowpass   
//  order       =  2  
//  sampleRate  =  48000 Hz 
//  corner1     =  20000 Hz
//

class LPF_20kHz
{
    enum { NZEROS = 2 };
    enum { NPOLES = 2 };

    double xv[ NZEROS + 1 ];
    double yv[ NPOLES + 1 ];

public:

    LPF_20kHz( void )
    {
        for ( int i = 0; i < NZEROS + 1; i++ )
            xv[ i ] = 0.0;
        for ( i = 0; i < NPOLES + 1; i++ )
            yv[ i ] = 0.0;
        }

    double filter( double input_value )
    {
        xv[0] = xv[1]; xv[1] = xv[2]; 
        xv[2] = input_value / 1.409226968e+00;

        yv[0] = yv[1]; yv[1] = yv[2]; 
        yv[2] = ( xv[0] + xv[2] ) + 2.0 * xv[1]
              + ( -0.4821925319 * yv[0] ) + ( -1.3562430718 * yv[1] );

        return yv[2];
        }
    };

///////////////////////////////////////////////////////////////////////////
// FFT

struct FreqMagFrame
{
    bool active;
    double m1;
    double m2;
    double f1;
    double f2;

    FreqMagFrame ()
    {
        active = false;
        m1 = m2 = 0.0;
        f1 = f1 = 0.0;
        }
    };

///////////////////////////////////////////////////////////////////////////

inline double Lin2Db( double magLin )
{
    return magLin < 1e-15 ? -300.0 : 20.0 * log10( magLin );
    }
 
inline double Db2Lin( double magDb )
{
    return pow( 10.0, magDb / 20.0 );
    }
 
class FFT
    : public Thread
{
    enum { MAX_FFT_SIZE = 32768 };

    virtual DWORD Main( void );

public:

    enum { MAX_PEAKS = 16 };

    struct Peak
    {
        int k;
        double freq;
        double magDb;
        double magLin;
        };

    enum PLAN_STATUS
    {
        PLAN_NOT_INITIALIZED,
        PLAN_INIT_1,
        PLAN_INIT_2,
        PLAN_INIT_3,
        PLAN_INITIALIZED
        };

    volatile PLAN_STATUS planStatus;

    ///////////////////////////////////////////////////////////////////////////
    // Filters

    LPF_20kHz LPF_20kHz;
    HPF_15Hz  HPF_15Hz;

    enum FFT_WINDOW
    {
        FFTWIN_RECTANGULAR = 0,
        FFTWIN_HANNING = 1,
        FFTWIN_BLACKMAN_HARRIS = 2,
        FFTWIN_KAISER_BESSEL = 3
        };

    unsigned int sampleRate;
    int N;
    int len;
    int stepN;
    double freqPerBin;

    ///////////////////////////////////////////////////////////////////////////

    unsigned long tClock;

    ///////////////////////////////////////////////////////////////////////////

    double inputGain;
    bool LPF_enabled;
    bool HPF_enabled;
    FFT_WINDOW winSel;

    ///////////////////////////////////////////////////////////////////////////

    fftwf_plan plan_4096;
    fftwf_plan plan_8192;
    fftwf_plan plan_16384;

    double* window;
    float* in;
    fftwf_complex* out;
    double* sampleBuffer;
    int sampleCount;

    ///////////////////////////////////////////////////////////////////////////

    double* lastMagLin;
    double* lastFreq;
    double* lastPhase;
    double* avgMagLin;

    ///////////////////////////////////////////////////////////////////////////

    enum NOISE_FILTER { NOISEF_DISABLED, NOISEF_LEARNING, NOISEF_ENABLED };
    NOISE_FILTER noiseFilter;
    double noiseTau;
    double* noiseLevelMagLin;

    ///////////////////////////////////////////////////////////////////////////

    double avgMagLinPerCent[ 1200 ];
    double maxMagLinPerCent[ 1200 ];

    ///////////////////////////////////////////////////////////////////////////

    double vuPk;
    double vuRms;
    double vuBar;
    double vuPkMax;
    double vuRmsSum;

    ///////////////////////////////////////////////////////////////////////////

    FFT::Peak peaks[ FFT::MAX_PEAKS ];
    int peakK;
    double peakFreq;

    bool longExpAvgTau;
    double avgFreq;
    double avgCents;
    double avgMagDb;

    ///////////////////////////////////////////////////////////////////////////
    // Note recognition

    static const char* noteName[ 12 ];
    double noteFreq[12];
    double noteFreqLog[12];
    bool refNoteLocked;
    int refNote;
    double refFreq;
    int refOctave;
    double lastCenterFreq;
    double centerFreq;
    int octave;

    FreqMagFrame pkwin;
    double cutOffMagDb;

    Mutex mutex;

    ///////////////////////////////////////////////////////////////////////////

    FFT( void )
        : Thread( "FFT" )
    {
        N = -1; // meaning fft structure not initialized
        plan_4096 = NULL;
        plan_8192 = NULL;
        plan_16384 = NULL;
        }

    void SetNewRate
    (
        unsigned int argSsamplesPerSec, 
        unsigned int argSamplesPerFFT, 
        int argStepN
        )
    {
        if ( N < 0 )
            return;

        sampleRate = argSsamplesPerSec;
        N = argSamplesPerFFT;
        len = N / 2;
        stepN = argStepN;
        freqPerBin = sampleRate / double(N);

        sampleCount = N - sampleRate / 100; //  - stepN;
        memset( sampleBuffer, 0, MAX_FFT_SIZE * sizeof(double) );

        memset( lastMagLin, 0, ( MAX_FFT_SIZE/2 + 1 ) * sizeof(double) );
        memset( lastFreq,   0, ( MAX_FFT_SIZE/2 + 1 ) * sizeof(double) );
        memset( lastPhase,  0, ( MAX_FFT_SIZE/2 + 1 ) * sizeof(double) );
        memset( avgMagLin,  0, ( MAX_FFT_SIZE/2 + 1 ) * sizeof(double) );

        memset( noiseLevelMagLin, 0, ( MAX_FFT_SIZE/2 + 1 ) * sizeof(double) );
        noiseTau = 1.0;
        noiseFilter = NOISEF_DISABLED;

        memset( peaks, 0, sizeof( peaks ) );
        peakK = 0;
        peakFreq = 0.0;

        SetWindow( winSel );
        }

    void SetWindow( FFT_WINDOW sel )
    {
        winSel = sel;

        if( winSel == FFTWIN_HANNING )
        {
            // Hanning Window
            //
            for ( int k = 0; k < N; k++ ) 
            {
                window[ k ] = 0.5 
                            - 0.5 * cos( 2.0 * M_PI * double(k)/double(N) );
                }

            inputGain = Db2Lin( 6.0 );
            }
        else if( winSel == FFTWIN_BLACKMAN_HARRIS )
        {
            // Blackman-Harris Window with 4 terms / -92dB side-lobe 
            //
            for ( int k = 0; k < N; k++ ) 
            {
                window[ k ] = 0.35875
                            - 0.48829 * cos( 2.0 * M_PI * double(k) / double(N) )
                            + 0.14128 * cos( 4.0 * M_PI * double(k) / double(N) )
                            - 0.01168 * cos( 6.0 * M_PI * double(k) / double(N) );
                }

            inputGain = Db2Lin( 8.9 );
            }
        else if( winSel == FFTWIN_KAISER_BESSEL )
        {
            // Kaiser Bessel Window
            //
            for ( int k = 0; k < N; k++ ) 
            {
                window[ k ] = 0.402
                            - 0.498 * cos( 2.0 * M_PI * double(k) / double(N) )
                            + 0.098 * cos( 4.0 * M_PI * double(k) / double(N) )
                            + 0.001 * cos( 6.0 * M_PI * double(k) / double(N) );
                }

            inputGain = Db2Lin( 7.9 );
            }
        else // default
        {
            // Rectangular Window
            //
            for ( int k = 0; k < N; k++ ) 
            {
                window[k] = 1.0;
                }

            inputGain = 1.0;
            }
        }

    void InitializePlan( void )
    {
        if ( fTerminating )
            return;

        planStatus = PLAN_INIT_1;
        Trace( "Initializing FFT plan for N=4096... ");
        plan_4096 = fftwf_plan_dft_r2c_1d( 4096, in, out, FFTW_MEASURE );
        TraceNOTS( "Done.\n" );

        if ( fTerminating )
            return;

        planStatus = PLAN_INIT_2;
        Trace( "Initializing FFT plan for N=8192... ");
        plan_8192 = fftwf_plan_dft_r2c_1d( 8192, in, out, FFTW_MEASURE );
        TraceNOTS( "Done.\n" );

        if ( fTerminating )
            return;

        planStatus = PLAN_INIT_3;
        Trace( "Initializing FFT plan for N=16384... ");
        plan_16384 = fftwf_plan_dft_r2c_1d( 16384, in, out, FFTW_MEASURE );
        TraceNOTS( "Done.\n" );

        if ( fTerminating )
            return;

        planStatus = PLAN_INITIALIZED;
        }

    void Initialize
    (
        unsigned int argSsamplesPerSec, 
        unsigned int argSamplesPerFFT, 
        int argStepN,
        const char* aName = "A",
        int aOctave = 4,
        double aFreq = 440.0
        )
    {
        tClock = 0;

        planStatus = PLAN_NOT_INITIALIZED;

        sampleRate = argSsamplesPerSec;
        N = argSamplesPerFFT;
        len = N / 2;
        stepN = argStepN;
        freqPerBin = sampleRate / double(N);

        in = new float[ MAX_FFT_SIZE + 2];
        out = (fftwf_complex*)in;

        window = new double[ MAX_FFT_SIZE + 2 ];
        inputGain = Db2Lin( 8.9 );

        SetWindow( FFTWIN_BLACKMAN_HARRIS );

        LPF_enabled = true;
        HPF_enabled = true;

        lastMagLin = new double[ MAX_FFT_SIZE/2 + 1 ];
        memset( lastMagLin, 0, ( MAX_FFT_SIZE/2 + 1 ) * sizeof(double) );
        lastFreq = new double[ MAX_FFT_SIZE/2 + 1 ];
        memset( lastFreq, 0, ( MAX_FFT_SIZE/2 + 1 ) * sizeof(double) );

        // NOTE: fft plan should be initialized by the WaveIn thread!

        // Force early DoFFT
        sampleCount = N - sampleRate / 100; //  - stepN;

        sampleBuffer = new double[ MAX_FFT_SIZE * 2 ];
        memset( sampleBuffer, 0, MAX_FFT_SIZE * 2 * sizeof(double) );

        lastPhase = new double[ MAX_FFT_SIZE/2 + 1 ];
        memset( lastPhase, 0, ( MAX_FFT_SIZE/2 + 1 ) * sizeof(double) );

        avgMagLin = new double [ MAX_FFT_SIZE/2 + 1 ];
        memset( avgMagLin, 0, ( MAX_FFT_SIZE/2 + 1 ) * sizeof(double) );

        noiseLevelMagLin = new double [ MAX_FFT_SIZE/2 + 1 ];
        memset( noiseLevelMagLin, 0, ( MAX_FFT_SIZE/2 + 1 ) * sizeof(double) );
        noiseTau = 1.0;
        noiseFilter = NOISEF_DISABLED;

        memset( peaks, 0, sizeof( peaks ) );
        peakK = 0;
        peakFreq = 0.0;

        longExpAvgTau = false;
        avgFreq = 0.0;  
        avgCents = 0.0; 
        avgMagDb = 0.0; 

        vuPk = 0.0;
        vuRms = 0.0;
        vuBar = 0.0;
        vuPkMax = 0.0;
        vuRmsSum = 0.0;

        // Initialize tuning / note recognition
        //
        refNoteLocked = false;

        // Defaults: A4 = 440.0 Hz
        refOctave = 4;
        refNote = 9;
        refFreq = 440.0;

        for ( int i = 0; i < 12; i++ )
        {
            if ( stricmp( aName, noteName[ i ] ) == 0 )
            {
                refNote = i;
                refFreq = aFreq;
                refOctave = aOctave;
                break;
                }
            }

        centerFreq = refFreq;
        octave = refOctave;
        lastCenterFreq = centerFreq;

        noteFreq[ refNote ] = refFreq;
        noteFreqLog[ refNote ] = log( refFreq );

        for ( i = refNote - 1; i >= 0; i-- ) 
        {
            noteFreq[ i ] = noteFreq[ i + 1] / CENTS_100;
            noteFreqLog[ i ] = noteFreqLog[ i + 1] - LOG_CENTS_100;
            }

        for ( i = refNote + 1; i < 12; i++ ) 
        {
            noteFreq[ i ] = noteFreq[ i - 1] * CENTS_100;
            noteFreqLog[ i ] = noteFreqLog[ i - 1] + LOG_CENTS_100;
            }

        for ( i = 0; i < 12; i++ )
            Trace( "%-2s = %.2lf Hz%s\n", noteName[ i ], noteFreq[ i ], refNote == i ? " (referent)" : "" );

        for ( i = 0; i < 1200; i++ )
            maxMagLinPerCent[ i ] = avgMagLinPerCent[ i ] = 0.0;

        cutOffMagDb = -60.0;

        pkwin.f1 = 16.0;
        pkwin.f2 = 8.0e3;
        pkwin.m1 = cutOffMagDb;
        pkwin.m2 = 0.0;
        }

    void Destroy( void )
    {
        mutex.Lock ();

        if ( N < 0 )
        {
            mutex.Unlock ();
            return;
            }

        StopThread ();

        if ( plan_16384 )
            fftwf_destroy_plan( plan_16384 );
        if ( plan_8192 )
            fftwf_destroy_plan( plan_8192 );
        if ( plan_4096 )
            fftwf_destroy_plan( plan_4096 );

        delete [] in;
        delete [] sampleBuffer;
        delete [] lastMagLin;
        delete [] lastFreq;
        delete [] lastPhase;
        delete [] avgMagLin;
        delete [] noiseLevelMagLin;

        N = -1;
        plan_16384 = NULL;
        plan_8192 = NULL;
        plan_4096 = NULL;

        mutex.Unlock ();
        }

    void Measure( int nSamples, short int* inData );

    void DoFFT( void );
    void FindPeaks( void );
    void DoExpAverage( void );

    void SetRefNote( int note, int octave = -1 );
    void SetRefNote( const char* name = NULL, int octave = -1 );

    ///////////////////////////////////////////////////////////////////////////

    int F2K( double freq )
    {
        return int( freq * N / sampleRate + 0.5 );
        }

    double K2F( int k )
    {
        return double( k ) * sampleRate / N;
        }

    ///////////////////////////////////////////////////////////////////////////

    ~FFT( void )
    {
        Destroy ();
        }
    };

///////////////////////////////////////////////////////////////////////////////
// CFFTWnd window

class CFFTWnd : public CWnd
{
    BOOL whiteBackground;

    COLORREF colorBackground;

    COLORREF colorTextDefault;
    COLORREF colorTextHighlighted;
    COLORREF colorTextDarkYellow;
    COLORREF colorTextGray;
    COLORREF colorTextCursor;
    COLORREF colorTextRed;
    COLORREF colorTextCutoff;

    COLORREF colorGraphBackground;
    COLORREF colorGraphGrid;
    COLORREF colorGraphGridSub;
    COLORREF colorGraphGridNote;
    COLORREF colorGraphPkWindow;
    COLORREF colorGraphPkMarks;
    COLORREF colorGraphCents;
    COLORREF colorGraphCentsFrame;
    COLORREF colorGraphCentsErr;
    COLORREF colorGraphSelection;

    COLORREF colorGraphSpectrAvg;
    COLORREF colorGraphSpectrTop;
    COLORREF colorGraphSpectrBot;
    COLORREF colorGraphChromAvg;

    CPen penGrid;
    CPen penGridSub;
    CPen penGridNote;
    CPen penGridNoteC;
    CPen penGridCutOff;
    CPen penPeakLine;
    CPen penSpectrAvg;
    CPen penSpectrTop;
    CPen penSpectrBot;
    CPen penChromAvg;

    ///////////////////////////////////////////////////////////////////////////
    // Members

    WaveInBuf waveIn;
    WaveOutBuf waveOut;
    CPU_Usage cpuUsage_process;
    CPU_Usage cpuUsage_display;

    ///////////////////////////////////////////////////////////////////////////

    FFT fft;

    double maxMagLin;
    double maxMagDb;
    double minMagDb; // Note: minMagLin == 0

    int minFreqK;
    int maxFreqK;
    int kLen;
    double logMinFreq;
    double logFreqRange;

    double vuBar;

    ///////////////////////////////////////////////////////////////////////////
    // Cursor

    double curFreq;
    double curMagDb;
    double curCents;
    int curNote;

    double selectionActive;
    double startFreq;
    double startMagDb;

    ///////////////////////////////////////////////////////////////////////////
    // Display

    CRect rectClient;
    CRect rectHeader;
    CRect rectFooter;
    CRect rectGraph;
    CRect rectScale;
    CRect rectXY;

    enum DISPLAY_FREQ { DISPLAY_FREQ_LOG, DISPLAY_FREQ_LIN, DISPLAY_CHROMATIC };

    volatile DISPLAY_FREQ displayFreq;
    bool displayMagLog;
    bool displayFFT;
    bool displayPeakInfo;
    bool displayWaterfall;
    bool freezeWaterfall;
    int specStartY;
    int specDeltaY;
    int specStartX;

    HACCEL m_hAccelTable;
    BOOL fMoving; // mouse is moving
    CPoint startp; // On left/right button start point
    CPoint pan_point; // reference

	CBitmap bmpDC; // device context bitmap (for smoothless upate);

    CBitmap bmpSharp;
    CBitmap bmpFlat;
    CBitmap bmpSpecWaterfall;
    CBitmap bmpChromWaterfall;

    CFlatButton btnViewFreqLog;
    CFlatButton btnViewFreqLin;
    CFlatButton btnViewFreqChrom;
    CFlatButton btnViewLog;
    CFlatButton btnViewWaterfall;
    CFlatButton btnViewPkWin;
    CFlatButton btnTuneG;
    CFlatButton btnTuneD;
    CFlatButton btnTuneA;
    CFlatButton btnTuneE;
    CFlatButton btnTuneAuto;
    CFlatButton btnNoiseLearn;
    CFlatButton btnNoiseClear;
    CFlatButton btnPlayTone;
    CFlatButton btn8vaPlus;
    CFlatButton btn8vaMinus;
    CFlatButton btnDebug;

    CFont fontText;
    CFont fontTextBold;
    CFont fontScale;
    CFont fontMusic;

    CString strSharp;
    CString strFlat;

    ///////////////////////////////////////////////////////////////////////////

// Construction
public:
	CFFTWnd( void );

// Attributes
public:

// Operations
public:
    unsigned GetSampleRate( void ) const
    {
        return fft.sampleRate;
        }

    BOOL Initialize( void );
    void OnClientRectChanged( CRect& rect );

    void OnUI_Changed( void );
    void OnData_Changed( void );

    void OnPaintHeader( CDC& dc );
    void OnPaintFooter( CDC& dc );
    void OnPaintHorizontalGrid( CDC& dc );
    void OnPaintSpectral( CDC& dc );
    void OnPaintSpectralVerticalGrid( CDC& dc );
    void OnPaintSpectralSelection( CDC& dc );
    void OnPaintChromatic( CDC& dc );
    void OnPaintVUMeter( CDC& dc );
    void OnPaintCursorInfo( CDC& dc );

    void OnSetCursorPosition( CPoint pos );
    void SetSelectionStart( void );
    void SetPkWindow( void );
    void SetZoomWindow( void );
    void SetCutOffMagAbs( void );

    void InitializeFFTPlan( void );
    void DestroyFFT( void );

    void Measure( int nSamples, short int* inData )
    {
        fft.Measure( nSamples, inData );
        }

    void InitializePlan( void )
    {
        fft.InitializePlan ();
        }

// Units' conversion functions

    ///////////////////////////////////////////////////////////////////////////

    void SetCurFreq( int x )
    {
        if ( displayFreq == DISPLAY_FREQ_LOG )
        {
            if ( x < rectXY.left )
                curFreq = fft.K2F( minFreqK );
            else if ( x > rectXY.right )
                curFreq = fft.K2F( maxFreqK );
            else
                curFreq = exp( logMinFreq + double( x - rectXY.left ) / rectXY.Width () * logFreqRange  );
            }
        else if ( displayFreq == DISPLAY_FREQ_LIN )
        {
            if ( x < rectXY.left )
                curFreq = fft.K2F( minFreqK );
            else if ( x > rectXY.right )
                curFreq = fft.K2F( maxFreqK );
            else
                curFreq = ( minFreqK + kLen * double( x - rectXY.left ) / rectXY.Width () )
                        * fft.sampleRate / fft.N;
            }
        else if ( displayFreq == DISPLAY_CHROMATIC )
        {
            if ( x < rectXY.left )
                x = rectXY.left;
            else if ( x > rectXY.right )
                x = rectXY.right;

            curCents = double( x - rectXY.left ) / rectXY.Width () * 1300.0 - 50.0;
            if ( curCents < 0.0 )
                curCents += 1200.0;
            else if ( curCents > 1200.0 )
                curCents -= 1200.0;

            curNote = int( curCents / 100.0 );
            curCents -= curNote * 100.0 + 50.0;

            curFreq = fft.noteFreq[ curNote ] * pow( 2.0, curCents / 1200.0 );
            }
        }

    double X_to_Freq_nocheck( double x )
    {
        if ( displayFreq == DISPLAY_FREQ_LOG )
        {
            return exp( logMinFreq + double( x - rectXY.left ) / rectXY.Width () * logFreqRange  );
            }
        else if ( displayFreq == DISPLAY_FREQ_LIN )
        {
            return ( minFreqK + kLen * double( x - rectXY.left ) / rectXY.Width () )
                   * fft.sampleRate / fft.N;
            }
        }

    int kFreq_to_xSpec( int k )
    {
        if ( displayFreq == DISPLAY_FREQ_LOG )
        {
            return rectXY.left + int( ( log( fft.K2F( k ) ) - logMinFreq ) / logFreqRange * rectXY.Width() + 0.5 );
            }
        else
        {
            return rectXY.left + int( double( k - minFreqK ) / kLen * rectXY.Width() + 0.5 );
            }
        }

    int Freq_to_xSpec( double freq )
    {
        if ( displayFreq == DISPLAY_FREQ_LOG )
        {
            return rectXY.left + int( ( log( freq ) - logMinFreq ) / logFreqRange * rectXY.Width() + 0.5 );
            }
        else
        {
            return rectXY.left + int( ( freq * fft.N / fft.sampleRate - minFreqK ) / kLen * rectXY.Width() + 0.5 );
            }
        }

    double Freq_to_Cents( double freq )
    {
        double cents = 1200.0 * log( freq / fft.noteFreq[0] ) / LOG_2;

        cents = cents - floor( cents / 1200.0 ) * 1200.0;
        if ( cents < 0.0 )
            cents += 1200.0;

        if ( cents >= 1150.0 )
            cents -= 1200.0;

        return cents;
        }

    int Cents_to_xChrom( double cents )
    {
        return rectXY.left + int( ( cents + 100.0 ) * rectXY.Width() / 1300.0 + 0.5 );
        }
    
    int xChrom_to_Note( int x )
    {
        int note = int( double( x - rectXY.left ) / rectXY.Width () * 1300.0 - 50.0 + 0.5 );
        note += 1200;
        note %= 1200;
        return note / 100;
        }

    ///////////////////////////////////////////////////////////////////////////

    int magDb_to_yLog( double magDb )
    {
        return rectXY.bottom - 1 
               - int( ( magDb - minMagDb ) 
                      / ( maxMagDb - minMagDb ) * ( rectXY.Height () - 1 ) + 0.5 );
        }

    int magLin_to_yLin( double magLin )
    {
        return rectXY.bottom - 1 
               - int( magLin / maxMagLin * ( rectXY.Height () - 1 ) + 0.5 );
        }

    int magLin_to_Y ( double magLin )
    {
        if ( displayMagLog )
        {
            return rectXY.bottom - 1 
                   - int( ( Lin2Db( magLin ) - minMagDb ) 
                          / ( maxMagDb - minMagDb ) * ( rectXY.Height () - 1 ) + 0.5 );
            }
        else
        {
            return magLin_to_yLin( magLin );
            }
        }

    int magDb_to_Y ( double magDb )
    {
        if ( displayMagLog )
        {
            return magDb_to_yLog( magDb );
            }
        else
        {
            return rectXY.bottom - 1 
                   - int( Db2Lin( magDb ) / maxMagLin * ( rectXY.Height () - 1 ) + 0.5 );
            }
        }

    double Y_to_magDb( int y )
    {
        if ( displayMagLog ) // logarithmic scale
        {
            if ( y > rectXY.bottom - 1 )
                return minMagDb;
            if ( y < rectXY.top )
                return maxMagDb;

            return ( maxMagDb - minMagDb ) 
                 * double( rectXY.bottom - 1 - y ) / ( rectXY.Height () - 1 ) 
                 + minMagDb;
            }
        else // linear scale
        {
            if ( y >= rectXY.bottom - 1 )
                return minMagDb;
            if ( y <= rectXY.top )
                return maxMagDb;

            return Lin2Db( maxMagLin * double( rectXY.bottom - 1 - y ) / ( rectXY.Height () - 1 ) );
            }
        }

    int magLin_to_Yabs ( double magLin )
    {
        if ( displayMagLog )
        {
            return rectXY.bottom - 1 
                   - int( ( Lin2Db( magLin ) + DB_RANGE ) 
                          / DB_RANGE * ( rectXY.Height () - 1 ) + 0.5 );
            }
        else
        {
            return rectXY.bottom - 1
                   - int( magLin * ( rectXY.Height () - 1 ) + 0.5 );
            }
        }

    ///////////////////////////////////////////////////////////////////////////

    COLORREF magDb_to_RGB( double magDb )
    {
        double A = ( magDb - minMagDb ) / ( maxMagDb - minMagDb );

        int B = A > 0.0 ? int( ( A - 0.0 ) * 256.0 / 0.9 ) : 0;
        int R = A > 0.6 ? int( ( A - 0.6 ) * 256.0 / 0.3 ) : 0;
        int G = A > 0.8 ? int( ( A - 0.8 ) * 256.0 / 0.2 ) : 0;
        if ( R > 255 ) R = 255;
        if ( B > 255 ) B = 255;
        if ( G > 255 ) G = 255;
        return RGB( R, G, B );
        }

    ///////////////////////////////////////////////////////////////////////////

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFFTWnd)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFFTWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CFFTWnd)
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnDestroy();
	afx_msg void OnPlayTone();
	afx_msg void OnViewFreqToggle();
	afx_msg void OnTuneViolinG();
	afx_msg void OnTuneViolinD();
	afx_msg void OnTuneViolinA();
	afx_msg void OnTuneViolinE();
	afx_msg void OnStopPlaying();
	afx_msg void OnTuneAuto();
	afx_msg void OnOctaveUp();
	afx_msg void OnOctaveDown();
	afx_msg void OnMeasureWaveIn();
	afx_msg void OnMeasureWaveOut();
	afx_msg void OnClearLearnedNoise();
	afx_msg void OnStartLearningNoise();
	afx_msg void OnViewChromatic();
	afx_msg void OnViewFreqLog();
	afx_msg void OnViewFreqLin();
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg void OnViewLogToggle();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnViewZoomOut();
	afx_msg void OnTogglePkWin();
	afx_msg void OnTogglePlaying();
	afx_msg void OnToggleTopMost();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnViewWaterfallToggle();
	afx_msg void OnToggleTraceWindow();
	afx_msg void OnFreezeWaterfall();
	afx_msg void OnWaterfallDecSpeed();
	afx_msg void OnWaterfallIncSpeed();
	afx_msg void OnDisplayFft();
	afx_msg void OnDisplayPeakInfo();
	afx_msg void OnFftwinRectangular();
	afx_msg void OnFftwinHanning();
	afx_msg void OnFftwinBlackmanHarris();
	afx_msg void OnFftwinKaiserBessel();
	afx_msg void OnCursorUp();
	afx_msg void OnCurosrDown();
	afx_msg void OnCursorLeft();
	afx_msg void OnCursorRight();
	afx_msg void OnCursorUp10();
	afx_msg void OnCurosrDown10();
	afx_msg void OnCursorLeft10();
	afx_msg void OnCursorRight10();
	afx_msg void OnTestButton();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CTuneItApp:
// See TuneIt.cpp for the implementation of this class
//

class CTuneItApp : public CWinApp
{
    CFFTWnd mainWin;

public:
	CTuneItApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTuneItApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTuneItApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TUNEIT_H__3C9748BF_F74E_4A34_8287_0C3DF519F9B2__INCLUDED_)
