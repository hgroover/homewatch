#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QDebug>
#include <QTimer>
//#include <QVBoxLayout>



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
  ,   m_snd(this)
  ,   m_dpmsTimer()
  #if (TESTMODE == 0)
  ,   m_serial()
  #endif
{
    ui->setupUi(this);

    m_dpmsOn = true;
    m_dpmsTimer.setSingleShot(true);
    m_dpmsTimer.setInterval(3 * 60000);
    connect( &m_dpmsTimer, SIGNAL(timeout()), this, SLOT(screenDelayElapsed()) );
    m_dpmsTimer.start();

    // Initialize fault map (FIXME do this from persistent data)
    QCheckBox *chk;
#define SetFaultMap(mask,abbr,full) \
        m_faultMap[mask] = Fault(abbr, mask); \
        chk = new QCheckBox(full, ui->centralWidget); \
        ui->vlCheckboxes->addWidget(chk); \
        m_faultMap[mask].m_chk = chk

    SetFaultMap(0x01, "Gar.dr", "Garage dr");
    SetFaultMap(0x02, "LoftDr", "Loft dr");
    SetFaultMap(0x04, "Motion", "Motion det");
    m_faultMap[0x04].m_perimeter = false;
    SetFaultMap(0x08, "BackDr", "Back dr");
    SetFaultMap(0x10, "FrontDr", "Front dr");
    m_faultMap[0x20] = Fault("unk20", 0x20, false);
    m_faultMap[0x40] = Fault("unk40", 0x40, false);
    m_faultMap[0x80] = Fault("Power", 0x80, false);

    // Pins on I2c: pin 0 is msb
    SetFaultMap(0x100, "Brkfst", "Breakfast" );
    SetFaultMap(0x200, "Ofc", "Office" );
    SetFaultMap(0x400, "BR4", "BR 4" );
    SetFaultMap(0x800, "Study/Din", "Study" );
    SetFaultMap(0x1000, "Fam 2,3", "Family rm 2,3" );
    SetFaultMap(0x2000, "Fam 1", "Family rm 1" );
    m_faultMap[0x4000] = Fault("i2c1", 0x4000 );
    m_faultMap[0x8000] = Fault("i2c0", 0x8000 );

    m_watchMode = -1;
    m_faultMask = 0;
    m_previousFault = 0;
    m_previousRawFault = 0;

    m_rawLog.setFileName("monitor-raw.log");
    m_rawLog.open(QIODevice::Append);
    m_rawLog.write(QString("\r\nStarted ").append(QDateTime::currentDateTime().toString()).append("\r\n").toLocal8Bit());

#if (TESTMODE==0)
    // Set up serial communication
    m_serial.setBaudRate(QSerialPort::Baud19200);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);
    m_serial.setPortName("/dev/ttyS0");

    connect( &m_serial, SIGNAL(readyRead()), this, SLOT(handleReadyRead()) );
    connect( &m_serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)) );
#else
    m_inputTest.setFileName("/tmp/ttyS0");
    QTimer::singleShot(3000, this, SLOT(handleTestReady()) );
    if (m_inputTest.open(QIODevice::ReadOnly))
    {
        ui->txtLog->appendPlainText("Reading from test input");
        connect( &m_inputTest, SIGNAL(readyRead()), this, SLOT(handleTestReady()) );
    }
    else
    {
        ui->txtLog->appendPlainText("Failed to open pipe");
    }
#endif

    connect( this, SIGNAL(processChunk(QString)), this, SLOT(handleChunk(QString)) );

#if (TESTMODE == 0)
    if (m_serial.open(QIODevice::ReadWrite))
    {
        ui->txtLog->appendPlainText("Opened serial port");
    }
    else
    {
        ui->txtLog->appendPlainText("Failed to open serial port");
    }
#endif

    m_log.setFileName("monitor.log");
    m_log.open(QIODevice::Append);

    connect( this, SIGNAL(playSound(QString)), &m_snd, SLOT(play(QString)), Qt::QueuedConnection );

    m_snd.start();

    logLine("PiMonitor v" MONITOR_VER);
    showMaximized();
}

MainWindow::~MainWindow()
{
    m_snd.stop();
    delete ui;
}

void MainWindow::on_btnStartStop_clicked()
{
    QString whichSound("beep1s");
    static int oddEven = 0;
    if (oddEven++ % 2) whichSound="peep250ms";
    emit playSound(whichSound);
}

void MainWindow::handleReadyRead()
{
#if (TESTMODE == 0)
    QByteArray data(m_serial.readAll());
    m_rawLog.write(data);
    m_rawLog.flush();
    QString chunk(m_partialLine);
    m_partialLine.clear();
    chunk += data;
    emit processChunk(chunk);
#endif
}

void MainWindow::handleTestReady()
{
#if TESTMODE
    if (!m_inputTest.isOpen()) return;
    QByteArray data(m_inputTest.readAll());
    if (!data.isEmpty())
    {
        QString chunk(m_partialLine);
        m_partialLine.clear();
        chunk += data;
        emit processChunk(chunk);
    }
    QTimer::singleShot(3000, this, SLOT(handleTestReady()) );
#endif
}

void MainWindow::handleChunk(QString s)
{
    bool discardLast = !s.endsWith('\n');
    QStringList a(s.split('\n', QString::SkipEmptyParts));
    int limit = a.length();
    if (discardLast)
    {
        limit--;
        if (!a.isEmpty())
        {
            m_partialLine += a.last();
        }
    }
    if (limit < 1) return;
    QString sPrefix(QDateTime::currentDateTime().toString("[dd-MMM-yy hh:mm:ss] "));
    int n;
    bool gotFault = false;
    unsigned int lastFault = 0;
    QRegExp reFault("^F\\s+(\\d+)\\s+t\\s+\\d+\\s*$");
    for (n = 0; n < limit; n++)
    {
        // Process fault lines
        // F %d t %ld
        QString sLine(a[n].trimmed());
        if (sLine.isEmpty()) continue;
        if (reFault.exactMatch(sLine))
        {
            gotFault = true;
            lastFault = reFault.cap(1).toUInt();
        }
        else if (sLine == "PERIM")
        {
            if (m_watchMode < 1)
            {
                m_watchMode = 1;
                logLine( sPrefix + "Perimeter watch detected" );
                ui->btnDisarm->setEnabled(true);
                ui->btnPerimeter->setEnabled(false);
                rebuildFaultMask();
            }
        }
        else
        {
            logLine( sPrefix + sLine );
        }
    }
    if (gotFault && lastFault != m_previousRawFault)
    {
        QString faultMsg(updateFault(lastFault));
        if (m_watchMode >= 1 && !faultMsg.isEmpty() && lastFault != m_previousFault)
        {
            logLine( sPrefix + faultMsg );
            emit playSound("beep1s");
        }
        m_previousRawFault = lastFault;
        m_previousFault = lastFault & m_faultMask;
    }
    m_log.flush();
}

QString MainWindow::updateFault( unsigned int faultMap )
{
    QString s;
    // Set indicators that have changed
    int n;
    unsigned int filteredFault = (faultMap & m_faultMask);
    unsigned int mask = 0x01;
    for (n = 0; n < 32; n++, mask <<= 1)
    {
        if (!m_faultMap.contains(mask)) continue;
        if (((mask & m_previousRawFault) ^ (mask & faultMap)) != 0 && m_faultMap[mask].m_chk)
        {
            m_faultMap[mask].m_chk->setChecked((mask & faultMap) != 0);
        }
    }
    // If nothing to log/beep, we're done
    if (filteredFault == 0)
    {
        if (m_previousFault) return "clear";
        return s;
    }
    // If monitoring perimeter, ignore motion
    mask = 0x01;
    for (n = 0; n < 32; n++, mask <<= 1)
    {
        if (m_faultMap.contains(mask))
        {
            // Is this fault triggered?
            if (0 == (filteredFault & mask)) continue;
            s += ' ';
            s += m_faultMap[mask].m_name;
        }
    }
    if (s.isEmpty()) return s;
    return s.prepend("FAULT:");
}

void MainWindow::logLine(QString s)
{
    m_log.write(s.toLocal8Bit().append('\n'));
    if (m_logLines > 10240)
    {
        resetUILog();
    }
    ui->txtLog->appendPlainText(s);
    if (!m_dpmsOn) forceDpms(true);
    else m_dpmsTimer.start();
}

void MainWindow::resetUILog()
{
    QString sTail(ui->txtLog->toPlainText().right(4096));
    QStringList a(sTail.split('\n', QString::SkipEmptyParts));
    a.removeFirst();
    m_logLines = a.length();
    ui->txtLog->setPlainText(a.join('\n'));
}

void MainWindow::handleTimeout()
{
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    QString s(QString().sprintf("unk:%d", (int)error));
    switch (error)
    {
    case QSerialPort::NoError:
        //0	No error occurred.
        return;
    case QSerialPort::DeviceNotFoundError:
        //1	An error occurred while attempting to open an non-existing device.
        s = "Device not found";
        break;
    case QSerialPort::PermissionError:
        //2	An error occurred while attempting to open an already opened device by another process or a user not having enough permission and credentials to open.
        s = "Permission error";
        break;
    case QSerialPort::OpenError:
        //3	An error occurred while attempting to open an already opened device in this object.
        s = "Open error";
        break;
    case QSerialPort::NotOpenError:
        //13	This error occurs when an operation is executed that can only be successfully performed if the device is open. This value was introduced in QtSerialPort 5.2.
        s = "Device must be open (NotOpenError)";
        break;
    case QSerialPort::ParityError:
        //4	Parity error detected by the hardware while reading data.
        s = "Parity error";
        break;
    case QSerialPort::FramingError:
        //5	Framing error detected by the hardware while reading data.
        s = "Framing error";
        break;
    case QSerialPort::BreakConditionError:
        //6	Break condition detected by the hardware on the input line.
        s = "Break detected";
        break;
    case QSerialPort::WriteError:
        //7	An I/O error occurred while writing the data.
        s = "Write error";
        break;
    case QSerialPort::ReadError:
        //8	An I/O error occurred while reading the data.
        s = "Read error";
        break;
    case QSerialPort::ResourceError:
        //9	An I/O error occurred when a resource becomes unavailable, e.g. when the device is unexpectedly removed from the system.
        s = "Resource error";
        break;
    case QSerialPort::UnsupportedOperationError:
        //10	The requested device operation is not supported or prohibited by the running operating system.
        s = "Unsupported operation";
        break;
    case QSerialPort::TimeoutError:
        s = "Timeout occurred";
        break;
    }
    ui->txtLog->appendPlainText(s);
}

void MainWindow::on_btnArm_clicked()
{

}

void MainWindow::on_btnPerimeter_clicked()
{
    // Set perimeter watch
    m_watchMode = 1;
    ui->btnDisarm->setEnabled(true);
    ui->btnPerimeter->setEnabled(false);
    rebuildFaultMask();
#if (TESTMODE == 0)
    m_serial.write("^1");
#endif
}

void MainWindow::on_btnDisarm_clicked()
{
    // If armed, prompt for code
    if (m_watchMode >= 2)
    {
        logLine("Disarming");
    }
    // Turn off
    m_watchMode = 0;
    m_faultMask = 0;
    ui->btnDisarm->setEnabled(false);
    ui->btnPerimeter->setEnabled(true);
#if (TESTMODE == 0)
    m_serial.write("^0");
#endif
}

// Build bitmask with bits set for each perimeter fault
void MainWindow::rebuildFaultMask()
{
    m_faultMask = 0;
    int n;
    unsigned int mask = 0x01;
    for (n = 0; n < 32; n++, mask <<= 1)
    {
        if (!m_faultMap.contains(mask)) continue;
        if (!m_faultMap[mask].m_perimeter) continue;
        m_faultMask |= mask;
    }
}

// Force monitor off or on via DPMS
void MainWindow::forceDpms(bool onOff)
{
    QString sCmd("xset dpms force ");
    sCmd.append(onOff ? "on" : "off");
    system(sCmd.toLocal8Bit().constData());
    m_dpmsOn = onOff;
    if (onOff)
    {
        m_dpmsTimer.start();
        QTimer::singleShot(1000, this, SLOT(forceRepaint()));
    }
    else
    {
        m_dpmsTimer.stop();
    }
}
