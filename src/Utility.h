#ifndef _UTILITY_H_INCLUDED
#define _UTILITY_H_INCLUDED

#include <ctime>
#include <cstdio>

#include <mmsystem.h>

///////////////////////////////////////////////////////////////////////////////

class aTimestamp // Used to convert time to YYYY-MM-DD HH:MM:SS string
{
    TCHAR str[ 40 ];

public:

    aTimestamp( const time_t t )
    {
        tm* tm = ::localtime( &t );

        _stprintf( str,
            _T("%04hd:%02hd:%02hd %02hd:%02hd:%02hd"),
            tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec
            );
        };

    aTimestamp( const SYSTEMTIME& st )
    {
        _stprintf( str,
            _T("%04hd:%02hd:%02hd %02hd:%02hd:%02hd"),
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond
            );
        };

    operator TCHAR* () { return str; }
    };

///////////////////////////////////////////////////////////////////////////////

class aElapsed // Used to convert elapsed time to HH:MM:SS string
{
    TCHAR str[ 40 ];

public:

    aElapsed( long t )
    {
        if ( t < 0 )
        {
            t = -t;
            _stprintf( str,
                _T("-%02ld:%02ld:%02ld"),
                t / 3600, ( t / 60 ) % 60, t % 60
                );
            }
        else
        {
            _stprintf( str,
                _T("%02ld:%02ld:%02ld"),
                t / 3600, ( t / 60 ) % 60, t % 60
                );
            }
        };

    operator TCHAR* () { return str; }
    };

///////////////////////////////////////////////////////////////////////////////

static inline COLORREF TranslateColor( OLE_COLOR oleColor )
{
    COLORREF rgb;
    ::OleTranslateColor( oleColor, NULL, &rgb );
    return rgb;
    }

///////////////////////////////////////////////////////////////////////////////
// Trace & Log support
extern void TraceStart( const TCHAR* filename = NULL );
extern void Trace( const TCHAR* format ... );
extern void TraceNOTS( const TCHAR* format ... );
extern void DebugRC( DWORD retCode );
extern void DebugMMError( MMRESULT mmr );
extern void TraceStop( void );
extern BOOL IsTraceStarted( void );

#endif // _UTILITY_H_INCLUDED