#ifndef SOUNDOUTPUT_H
#define SOUNDOUTPUT_H

#include <QString>
#include <QThread>
#include <QMap>

#include "pulsesimple.h"
#include "generator.h"

class SoundOutput : public QThread
{
    Q_OBJECT
public:
    explicit SoundOutput(QObject *parent = 0);
    ~SoundOutput();

private:
    QAudioFormat m_format;
    QMap<QString,Generator*> m_sounds;

    PulseSimple m_pulse;

signals:
    void msg(QString text);

public slots:
    void play(QString sampleName);
    void stop();

private slots:
    void attemptAudioOpen();

};

#endif // SOUNDOUTPUT_H
