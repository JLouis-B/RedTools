#ifndef IAPPLOGGER_H
#define IAPPLOGGER_H

#include <irrString.h>

class IAppLogger
{
public:
    IAppLogger() {}
    virtual ~IAppLogger() {}

    virtual void log(irr::core::stringc content) = 0;
    virtual void flush() {}
};

#endif // IAPPLOGGER_H
