#ifndef PULSESIMPLE_H
#define PULSESIMPLE_H
// Pulse Audio Simple API class
// Since Qt's multimedia support for opensource Linux builds is a bit spotty,
// this class provides the simple API's blocking methods for rendering (short)
// sound samples.

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <pulse/simple.h>
#include <pulse/error.h>

class PulseSimple
{
public:
    PulseSimple();

    bool open();
    int write( unsigned char *b, int len );
    void flush();
    void close();

private:
    static const pa_sample_spec s_ss;
    pa_simple *m_s;
    int m_error;
};

#endif // PULSESIMPLE_H
