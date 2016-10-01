#include "soundoutput.h"

#include <QTimer>
#include <QAudioDeviceInfo>
#include <QAudioOutput>

const int DurationSeconds = 1;
const int ToneSampleRateHz = 600;
const int DataSampleRateHz = 44100;
const int BufferSize      = 32768;

SoundOutput::SoundOutput(QObject *parent) :
    QThread(parent)
 ,   m_pulse()
{
    m_format.setSampleRate(DataSampleRateHz);
    m_format.setChannelCount(2);
    m_format.setSampleSize(16);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);

    Generator *g;
    g = new Generator(this);
    //connect( g, SIGNAL(msg(QString)), this, SLOT(logLine(QString)) );
    g->generateData(m_format, /*DurationSeconds* */ 1000000/2, ToneSampleRateHz, 1, 1 );
    m_sounds["beep1s"] = g;
    g = new Generator( this );
    //connect( g, SIGNAL(msg(QString)), this, SLOT(logLine(QString)) );
    g->generateData(m_format, 1000000/4, 1280, 1, 1);
    m_sounds["peep250ms"] = g;

    if (m_pulse.open())
    {
        emit msg("Opened simple pulse API");
    }
    else
    {
        emit msg("Failed to open simple pulse API, retry in 11s");
        QTimer::singleShot(11000, this, SLOT(attemptAudioOpen()) );
    }

}

SoundOutput::~SoundOutput()
{
    m_pulse.close();
}

void SoundOutput::play(QString sampleName)
{
    qint64 bytesRead;
    static unsigned char buf[0x4000];
    if (!m_sounds.contains(sampleName)) return;
    //if (!m_lock.tryLock(5)) return;
    m_sounds[sampleName]->rewind();
    if ((bytesRead = m_sounds[sampleName]->readData((char *)buf, sizeof(buf))) >= 256)
    {
        //emit msg(QString().sprintf("Got %lld bytes of sound data", bytesRead ));
        m_pulse.write(buf, bytesRead);
        m_pulse.flush();
    }
    else
    {
        emit msg("Failed to get sound data from generator");
    }
    //m_lock.unlock();
}

void SoundOutput::stop()
{
}

void SoundOutput::attemptAudioOpen()
{
    if (m_pulse.open())
    {
        emit msg("Second attempt to open pulse succeeded");
    }
    else
    {
        emit msg("Giving up on audio");
    }
}

