#ifndef FAULT_H
#define FAULT_H

#include <QString>
#include <QCheckBox>

// Describe a fault
class Fault
{
public:
    Fault();
    Fault(QString name, unsigned int mask, bool isPerimeter = true);
    Fault(const Fault& src);
    void operator=(const Fault& src);
    QString m_name;
    unsigned int m_mask;
    bool m_perimeter;
    QCheckBox *m_chk;
};

#endif // FAULT_H
