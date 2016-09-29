#ifndef FAULT_H
#define FAULT_H

#include <QString>

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
};

#endif // FAULT_H
