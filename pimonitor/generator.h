#ifndef _GENERATOR_H_INCLUDED_
#define _GENERATOR_H_INCLUDED_

#include <QAudioOutput>
#include <QByteArray>
#include <QIODevice>

class Generator : public QIODevice
{
    Q_OBJECT

public:
    Generator(QObject *parent);
    Generator(const QAudioFormat &format, qint64 durationUs, int sampleRate, QObject *parent, int startSlope = 1, int endSlope = 1 );
    ~Generator();

    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qint64 bytesAvailable() const;
    qint64 bufferSize() const { return m_buffer.length(); }
    void rewind() { m_pos = 0; }

signals:
    void msg( QString s );

public:
    void generateData(const QAudioFormat &format, qint64 durationUs, int sampleRate, int startSlope = 1, int endSlope = 1 );

private:
    qint64 m_pos;
    QByteArray m_buffer;
};


#endif
