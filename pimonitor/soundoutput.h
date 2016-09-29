#ifndef SOUNDOUTPUT_H
#define SOUNDOUTPUT_H

#include <QThread>

class SoundOutput : public QThread
{
    Q_OBJECT
public:
    explicit SoundOutput(QObject *parent = 0);

signals:
    void msg(QString text);

public slots:
    void play(QString sampleName);
    void stop();
};

#endif // SOUNDOUTPUT_H
