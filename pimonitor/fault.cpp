#include "fault.h"

Fault::Fault()
    : m_name()
{
    m_mask = 0;
    m_perimeter = true;
}

Fault::Fault(QString name, unsigned int mask, bool isPerimeter )
    : m_name(name)
{
    m_mask = mask;
    m_perimeter = isPerimeter;
}

Fault::Fault(const Fault& src)
{
    *this = src;
}

void Fault::operator =(const Fault& src)
{
    m_name = src.m_name;
    m_mask = src.m_mask;
    m_perimeter = src.m_perimeter;
}
