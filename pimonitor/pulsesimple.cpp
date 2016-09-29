#include "pulsesimple.h"

const pa_sample_spec PulseSimple::s_ss = {
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = 2
};

PulseSimple::PulseSimple()
{
    m_s = NULL;
}

bool PulseSimple::open()
{
    if (m_s) close();
    m_s = pa_simple_new(NULL, "monitor", PA_STREAM_PLAYBACK, NULL, "playback", &s_ss, NULL, NULL, &m_error);
    return m_s != NULL;
}

int PulseSimple::write( unsigned char *b, int len )
{
    if (NULL == m_s) return -1;
    return pa_simple_write(m_s, b, (size_t) len, &m_error);
}

void PulseSimple::flush()
{
    if (m_s) pa_simple_drain( m_s, &m_error );
}

void PulseSimple::close()
{
    if (m_s != NULL)
    {
        pa_simple_drain( m_s, &m_error );
        pa_simple_free( m_s );
        m_s = NULL;
    }
}
