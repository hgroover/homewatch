#include "mainwindow.h"
#include "generator.h"

#include <QDebug>
#include <qmath.h>
#include <qendian.h>
Generator::Generator(QObject *parent)
    : QIODevice(parent)
    , m_pos(0)
{
}

Generator::Generator(const QAudioFormat &format,
                     qint64 durationUs,
                     int sampleRate,
                     QObject *parent,
                     int startSlope,
                     int endSlope)
    :   QIODevice(parent)
    ,   m_pos(0)
{
    if (format.isValid())
    {
        qDebug() << "Generating data for" << durationUs;
        generateData(format, durationUs, sampleRate, startSlope, endSlope);
    }
    else qDebug() << "Not generating data";
}

Generator::~Generator()
{

}

void Generator::start()
{
    if (open(QIODevice::ReadOnly))
    {
        emit msg("Generator opened successfully");
    }
    else
    {
        emit msg("Generator failed to open");
    }
}

void Generator::stop()
{
    m_pos = 0;
    close();
}

// Fill buffer for this instance with data in specified format. "sampleRate" is hz (frequency)
void Generator::generateData(const QAudioFormat &format, qint64 durationUs, int sampleRate, int startSlope, int endSlope )
{
    // Convert sample size in bits to per-channel bytes
    const int channelBytes = format.sampleSize() / 8;
    // Per-sample bytes to cover multiple channels
    const int sampleBytes = format.channelCount() * channelBytes;

    // Project number of samples at specified datarate, bits per sample and number of channels
    // Note that format.sampleRate() is actual sample rate (e.g. 44100) whereas the argument
    // sampleRate is frequency (e.g. 440, 880, 1760, etc)
    // startSlope and endSlope can provide a linear frequency adjustment
    qint64 length = (format.sampleRate() * format.channelCount() * (format.sampleSize() / 8))
                        * durationUs / 1000000;

    Q_ASSERT(length % sampleBytes == 0);
    Q_UNUSED(sampleBytes) // suppress warning in release builds

    m_buffer.resize(length);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(m_buffer.data());
    int sampleIndex = 0;
    int totalSamples = (int)(length / sampleBytes);
    qreal delta = ((qreal)(endSlope - startSlope)) / totalSamples;
    qreal frequency = sampleRate * startSlope;
    emit msg(QString().sprintf("Length %lld sampleBytes %d count %d delta %.f", length, sampleBytes, totalSamples, delta));

    while (length) {
        const qreal x = qSin(2 * M_PI * frequency * qreal(sampleIndex % format.sampleRate()) / format.sampleRate());
        frequency += delta;
        for (int i=0; i<format.channelCount(); ++i) {
            if (format.sampleSize() == 8 && format.sampleType() == QAudioFormat::UnSignedInt) {
                const quint8 value = static_cast<quint8>((1.0 + x) / 2 * 255);
                *reinterpret_cast<quint8*>(ptr) = value;
            } else if (format.sampleSize() == 8 && format.sampleType() == QAudioFormat::SignedInt) {
                const qint8 value = static_cast<qint8>(x * 127);
                *reinterpret_cast<quint8*>(ptr) = value;
            } else if (format.sampleSize() == 16 && format.sampleType() == QAudioFormat::UnSignedInt) {
                quint16 value = static_cast<quint16>((1.0 + x) / 2 * 65535);
                if (format.byteOrder() == QAudioFormat::LittleEndian)
                    qToLittleEndian<quint16>(value, ptr);
                else
                    qToBigEndian<quint16>(value, ptr);
            } else if (format.sampleSize() == 16 && format.sampleType() == QAudioFormat::SignedInt) {
                qint16 value = static_cast<qint16>(x * 32767);
                if (format.byteOrder() == QAudioFormat::LittleEndian)
                    qToLittleEndian<qint16>(value, ptr);
                else
                    qToBigEndian<qint16>(value, ptr);
            }

            ptr += channelBytes;
            length -= channelBytes;
        }
        ++sampleIndex;
    }
}

qint64 Generator::readData(char *data, qint64 len)
{
    qint64 total = 0;
    if (!m_buffer.isEmpty()) {
        while (len - total > 0) {
            const qint64 chunk = qMin((m_buffer.size() - m_pos), len - total);
            memcpy(data + total, m_buffer.constData() + m_pos, chunk);
            m_pos = (m_pos + chunk) % m_buffer.size();
            total += chunk;
        }
    }
    return total;
}

qint64 Generator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 Generator::bytesAvailable() const
{
    return m_buffer.size() + QIODevice::bytesAvailable();
}

