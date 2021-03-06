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
#include "soundoutput.h"

#define TESTMODE    0
#define MONITOR_VER "1.0.004"

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
    void playSound( QString sampleName );

private:
    Ui::MainWindow *ui;

    int m_watchMode; // -1 = unset, 0 = off, 1 = perimeter, 2 = armed
    unsigned int m_faultMask; // In perimeter watch, excludes non-perimeter faults
    unsigned int m_previousFault; // Last fault with filter applied
    unsigned int m_previousRawFault; // Last fault without filter

#if TESTMODE
    QFile m_inputTest;
#else
    QSerialPort m_serial;
#endif
    // "Cooked" log with timestamp prefixes
    QFile m_log;
    // Raw log as received from Arduino
    QFile m_rawLog;
    QString m_partialLine;
    unsigned long m_logLines; // Lines sent to ui log since last reset

    SoundOutput m_snd;
    QMap<unsigned int,Fault> m_faultMap;

    bool m_dpmsOn;
    QTimer m_dpmsTimer;

    void resetUILog();
private slots:
    void handleReadyRead();
    void handleTestReady();
    void handleTimeout();
    void handleError(QSerialPort::SerialPortError error);

    void handleChunk( QString s );
    void logLine( QString s );
    QString updateFault( unsigned int faultMap );

    void on_btnArm_clicked();
    void on_btnPerimeter_clicked();
    void on_btnDisarm_clicked();

    // Reconstruct m_faultMask as a bitmask of all faults which are perimeter faults
    void rebuildFaultMask();

    // Force monitor off or on via DPMS
    void forceDpms(bool onOff);

    // Blank elapsed
    void screenDelayElapsed() { forceDpms(false); }

    // Force repaint after resuming screen
    void forceRepaint() { repaint(); }
};

#endif // MAINWINDOW_H
