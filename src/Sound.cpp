// FFTWnd.cpp : implementation file
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

DWORD WaveInBuf::Main( void )
{
    Trace( "%s::Main\n", objID );

    if ( ! fft )
    {
        Trace( "%s::Cowardly refusing to work without CFFTWnd parent.\n", objID );
        return 0;
        }

    SetThreadPriority( THREAD_PRIORITY_TIME_CRITICAL );

    unsigned int sampleRate = fft->GetSampleRate ();

    size_t nSampleCount = sampleRate / 100; // gives 100 Hz freq of GetMessage () and 10ms delay
    int nBufferCount = 128;

    unsigned short localSeqNo = 0;

    WAVEHDR* waveBufHdrs = (WAVEHDR*)::GlobalAlloc( GPTR, nBufferCount * sizeof( WAVEHDR ) );
    // note GPTR means waveBufHdrs is zero init
    for ( int j = 0; j < nBufferCount; j ++ )
    {
        waveBufHdrs[ j ].lpData = (char*)::GlobalAlloc( GPTR, 2 * nSampleCount * sizeof( short ) );
        waveBufHdrs[ j ].dwBufferLength = nSampleCount * sizeof(short);
        waveBufHdrs[ j ].dwUser = j;
        }

    /////////////////////////
    // 16-bit PCM
    //
    WAVEFORMATEX wfx = { 0 };
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    ////////////////////////
    // Wave Input
    //
    HWAVEIN hwi;

    MMRESULT
    rc = waveInOpen( &hwi, deviceID, &wfx, 
                     GetCurrentThreadId (), 0, CALLBACK_THREAD /*| WAVE_FORMAT_DIRECT*/ );
    if ( rc )
    {
        DebugMMError( rc );
        Trace( "%s: waveInOpen failed\n", objID );
        return 1;
        }

    Trace( "%s: waveInOpen succeded device id %d\n", objID, deviceID );

    for ( j = 0; j < nBufferCount; j++ )
        rc = waveInPrepareHeader( hwi, &waveBufHdrs[ j ], sizeof( WAVEHDR ) );

    for ( j = 0; j < nBufferCount; j++ )
        rc = waveInAddBuffer( hwi, &waveBufHdrs[ j ], sizeof( WAVEHDR ) );

    rc = waveInStart( hwi );

    if ( rc )
    {
        Trace( "%s: waveInStart failed\n", objID );
        }

    MSG msg;
    while( ! fTerminating )
    {
        if ( 1 != GetMessage( &msg, NULL, 0, 0 ) )
        {
            break;
            }

        // Trace( "%s:: %d\n", objID, msg.message );

        if ( msg.message == WM_QUIT )
            break;

        if ( msg.message == MM_WIM_DATA )
        {
            short* pcmP = (short*)LPWAVEHDR( msg.lParam )->lpData;
            rc = waveInAddBuffer( hwi, LPWAVEHDR( msg.lParam ), sizeof( WAVEHDR ) );
            if ( measureFFT && fft )
                fft->Measure( nSampleCount, pcmP );
            }
        }

    Trace( "%s::Main COMPLETED\n", objID );

    waveInReset( hwi );
    Sleep( 200 );
    waveInClose( hwi );

    for ( j = 0; j < nBufferCount; j ++ )
        ::GlobalFree( waveBufHdrs[ j ].lpData );

    ::GlobalFree( waveBufHdrs );

    return 0;
    }

///////////////////////////////////////////////////////////////////////////////

DWORD WaveOutBuf::Main( void )
{
    Trace( "%s::Main\n", objID );

    SetThreadPriority( THREAD_PRIORITY_TIME_CRITICAL );

    unsigned int sampleRate = fft->GetSampleRate ();

    gen1.Initialize( sampleRate );
    gen2.Initialize( sampleRate );

    size_t nSampleCount = sampleRate / 100; // gives 1 Hz freq of GetMessage ()
    int nBufferCount = 15; // x 10 ms = 150 ms output latency

    WAVEHDR* waveBufHdrs = (WAVEHDR*)::GlobalAlloc( GPTR, nBufferCount * sizeof( WAVEHDR ) );
    // note GPTR means waveBufHdrs is zero init
    for ( int j = 0; j < nBufferCount; j ++ )
    {
        waveBufHdrs[ j ].lpData = (char*)::GlobalAlloc( GPTR, 2 * nSampleCount * sizeof( short ) );
        waveBufHdrs[ j ].dwBufferLength = nSampleCount * sizeof( short );
        waveBufHdrs[ j ].dwUser = j;
        }

    /////////////////////////
    // 16-bit PCM
    //
    WAVEFORMATEX wfx = { 0 };
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    ////////////////////////
    // Wave Output
    //
    HWAVEOUT hwo;

    MMRESULT
    rc = waveOutOpen( &hwo, deviceID, &wfx, GetCurrentThreadId (), 0, /*WAVE_FORMAT_DIRECT | */ CALLBACK_THREAD );
    if ( rc )
    {
        DebugMMError( rc );
        Trace( "%s: waveOutOpen failed\n", objID );
        return 1;
        }

    Trace( "%s: waveOutOpen succeded device id %d\n", objID, deviceID );

    for ( j = 0; j < nBufferCount; j++ )
        rc = waveOutPrepareHeader( hwo, &waveBufHdrs[ j ], sizeof( WAVEHDR ) );

    for ( j = 0; j < nBufferCount; j++ )
        rc = waveOutWrite( hwo, &waveBufHdrs[ j ], sizeof( WAVEHDR ) );

    MSG msg;
    while( ! fTerminating )
    {
        if ( 1 != GetMessage( &msg, NULL, 0, 0 ) )
        {
            break;
            }

        // Trace( "%s:: %d\n", objID, msg.message );

        if ( msg.message == WM_QUIT )
            break;

        if ( msg.message == MM_WOM_DONE )
        {
            LPWAVEHDR powh = LPWAVEHDR( msg.lParam );

            powh->dwBufferLength = nSampleCount * 2;
            short* pcmP = (short*)powh->lpData;

            for ( unsigned int i = 0; i < nSampleCount; i++ )
            {
                gen1.OnEverySample ();
                gen2.OnEverySample ();

                *pcmP++ = short( gen1.GetSample() + gen2.GetSample () );
                }

            if ( measureFFT && fft )
                fft->Measure( nSampleCount, (short*)powh->lpData );

            MMRESULT rc = waveOutWrite( hwo, powh, sizeof( WAVEHDR ) );
            }
        }

    Trace( "%s::Main COMPLETED\n", objID );

    waveOutReset( hwo );
    Sleep( 100 );
    waveOutClose( hwo );

    for ( j = 0; j < nBufferCount; j ++ )
        ::GlobalFree( waveBufHdrs[ j ].lpData );

    ::GlobalFree( waveBufHdrs );

    return 0;
    }


