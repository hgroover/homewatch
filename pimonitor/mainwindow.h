#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioOutput>
#include <QByteArray>
#include <QIODevice>
#include <QTimer>
#include <QMap>

#include <QSerialPort>
#include <QFile>

#include "fault.h"
#include "pulsesimple.h"
#include "generator.h"

#define TESTMODE    0

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void processChunk( QString s );

private:
    Ui::MainWindow *ui;

    QTimer *m_pullTimer;

    //QAudioDeviceInfo m_device;
    //Generator *m_generator;
    //QAudioOutput *m_audioOutput;
    //QIODevice *m_output; // not owned
    QAudioFormat m_format;
    QMap<QString,Generator*> m_sounds;

    PulseSimple m_pulse;

    bool m_pullMode;
    QByteArray m_buffer;
    int m_watchMode; // -1 = unset, 0 = off, 1 = perimeter, 2 = armed
    unsigned int m_faultMask; // In perimeter watch, excludes non-perimeter faults
    unsigned int m_previousFault; // Last fault with filter applied

#if TESTMODE
    QFile m_inputTest;
#else
    QSerialPort m_serial;
#endif
    QFile m_log;
    QString m_partialLine;
    unsigned long m_logLines; // Lines sent to ui log since last reset

    QMap<unsigned int,Fault> m_faultMap;

    void resetUILog();
private slots:
    //void pullTimerExpired();
    void handleReadyRead();
    void handleTestReady();
    void handleTimeout();
    void handleError(QSerialPort::SerialPortError error);

    void handleChunk( QString s );
    void logLine( QString s );
    QString updateFault( unsigned int faultMap );

    void on_btnStartStop_clicked();
    void on_btnArm_clicked();
    void on_btnPerimeter_clicked();
    void on_btnDisarm_clicked();
    void attemptAudioOpen();

    void rebuildFaultMask();
};

#endif // MAINWINDOW_H
