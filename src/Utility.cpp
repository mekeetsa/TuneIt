#include "StdAfx.h"
#include "Utility.h"

// Utility functions implementation

static volatile BOOL fTraceStarted = FALSE;

void
DebugMMError
(
    MMRESULT mmr
    )
{
    PSTR psz = "Unknown ERROR";

    switch (mmr)
    {
        case MMSYSERR_NOERROR:
            psz = "MMSYSERR_NOERROR";
            break;

        case MMSYSERR_ERROR:
            psz = "MMSYSERR_ERROR";
            break;

        case MMSYSERR_BADDEVICEID:
            psz = "MMSYSERR_BADDEVICEID";
            break;

        case MMSYSERR_NOTENABLED:
            psz = "MMSYSERR_NOTENABLED";
            break;

        case MMSYSERR_ALLOCATED:
            psz = "MMSYSERR_ALLOCATED";
            break;

        case MMSYSERR_INVALHANDLE:
            psz = "MMSYSERR_INVALHANDLE";
            break;

        case MMSYSERR_NODRIVER:
            psz = "MMSYSERR_NODRIVER";
            break;

        case MMSYSERR_NOMEM:
            psz = "MMSYSERR_NOMEM";
            break;

        case MMSYSERR_NOTSUPPORTED:
            psz = "MMSYSERR_NOTSUPPORTED";
            break;

        case MMSYSERR_BADERRNUM:
            psz = "MMSYSERR_BADERRNUM";
            break;

        case MMSYSERR_INVALFLAG:
            psz = "MMSYSERR_INVALFLAG";
            break;

        case MMSYSERR_INVALPARAM:
            psz = "MMSYSERR_INVALPARAM";
            break;

        case WAVERR_BADFORMAT:
            psz = "WAVERR_BADFORMAT";
            break;

        case WAVERR_STILLPLAYING:
            psz = "WAVERR_STILLPLAYING";
            break;

        case WAVERR_UNPREPARED:
            psz = "WAVERR_UNPREPARED";
            break;

        case WAVERR_SYNC:
            psz = "WAVERR_SYNC";
            break;
/*
        case ACMERR_NOTPOSSIBLE:
            psz = "ACMERR_NOTPOSSIBLE";
            break;

        case ACMERR_BUSY:
            psz = "ACMERR_BUSY";
            break;

        case ACMERR_UNPREPARED:
            psz = "ACMERR_UNPREPARED";
            break;

        case ACMERR_CANCELED:
            psz = "ACMERR_CANCELED";
            break;
*/
        }

    Trace( "%s\n", psz );
    }

void DebugRC( DWORD retCode )
{
    CHAR lpMsgBuf[ 512 ] = { 0 };

    DWORD len = ::FormatMessage
    (
        FORMAT_MESSAGE_FROM_SYSTEM,    // source and processing options
        NULL,                          // pointer to  message source
        retCode,                       // requested message identifier
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // language identifier for requested message
        lpMsgBuf,                      // pointer to message buffer
        sizeof( lpMsgBuf ) - 1,        // maximum size of message buffer
        NULL                           // address of array of message inserts
        );

    if ( len > 0 )
    {
        if ( lpMsgBuf[ len - 1 ] == '\n' )
            Trace( "%s", lpMsgBuf );
        else
            Trace( "%s\n", lpMsgBuf );
        }
    else
    {
        Trace( "Return code = %lu (%08lx)\n", retCode, retCode );
        }
    }

void TraceStop( void )
{
#ifndef _DEBUG
    // return;
#endif
    const TCHAR* filename = _T("NUL:");
    const TCHAR* flags = _T("w");
    _tfreopen( filename, flags, stdout );
    _tfreopen( filename, flags, stderr );

    fTraceStarted = FALSE;

    ::FreeConsole ();
}

BOOL IsTraceStarted( void )
{
    return fTraceStarted;
    }

void TraceStart( const TCHAR* filename )
{
#ifndef _DEBUG
    // return;
#endif
    const TCHAR* flags = _T("a+");

    if ( filename == NULL ) // no filename means writing to console
    {
        ::AllocConsole ();
        filename = _T("CON:");
        flags = _T("w");
        }

    _tfreopen( filename, flags, stdout );
    _tfreopen( filename, flags, stderr );

    fTraceStarted = TRUE;
}

void Trace( const TCHAR* format ... )
{
#ifndef _DEBUG
    //return;
#endif
    if ( ! fTraceStarted )
        return;

    va_list argptr;
    va_start( argptr, format );

    SYSTEMTIME st;
    ::GetLocalTime( &st );

    _tprintf( _T("%s: "), (TCHAR*)aTimestamp( st ) );

    _vtprintf( format, argptr );
}

void TraceNOTS( const TCHAR* format ... )
{
#ifndef _DEBUG
    //return;
#endif

    if ( ! fTraceStarted )
        return;

    va_list argptr;
    va_start( argptr, format );

    _vtprintf( format, argptr );
}

