#ifndef CONSOLELOGGER_H
#define CONSOLELOGGER_H

#include "IAppLogger.h"

class ConsoleLogger : public IAppLogger
{
public:
    ConsoleLogger();
    virtual ~ConsoleLogger() override {}

    virtual void log(irr::core::stringc content) override;
    virtual void flush() override;
};

#endif // CONSOLELOGGER_H
