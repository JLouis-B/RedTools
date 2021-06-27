#ifndef CALLBACKLOGGER_H
#define CALLBACKLOGGER_H

#include "IAppLogger.h"

#include <functional>

class CallbackLogger : public IAppLogger
{
public:
    CallbackLogger();

    virtual void log(irr::core::stringc content) override;

    void registerLogToUserCallback(std::function<void(irr::core::stringc)>);
    void unregisterLogToUserCallback();

private:
    std::function<void(irr::core::stringc)> LogToUserCallback;
};

#endif // CALLBACKLOGGER_H
