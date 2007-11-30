#ifndef _FFT_H_INCLUDED
#define _FFT_H_INCLUDED

class RealFFT
{
    short* SinTable;
    int Points;

public:
    int* BitReversed;
    RealFFT( int fftlen );
    ~RealFFT( void );
    void Transform( short* buffer );

    double A2( short* fftdata, int i )
    {
        double Re = fftdata[ BitReversed[ i ] ];
        double Im = fftdata[ BitReversed[ i ] + 1 ];
        return double( Re * Re + Im * Im );
        }
    };

class WIND
{
    int Points;
    short *wind; // Array storing windowing function 

public:
    WIND( int fftlen, int windfunc, double alpha = 0.5 ); // gaussian window parameter
    ~WIND ();
    void Transform( short* sample16, short* fftdata );
    void TransformLR( short* sample16, short* fftdata );
    };

#endif