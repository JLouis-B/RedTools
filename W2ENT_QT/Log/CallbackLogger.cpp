#include "CallbackLogger.h"

CallbackLogger::CallbackLogger() :
    LogToUserCallback(nullptr)
{
}

void CallbackLogger::log(irr::core::stringc content)
{
    if (LogToUserCallback != nullptr)
    {
        LogToUserCallback(content);
    }
}

void CallbackLogger::registerLogToUserCallback(std::function<void(irr::core::stringc)> callback)
{
    LogToUserCallback = callback;
}

void CallbackLogger::unregisterLogToUserCallback()
{
    LogToUserCallback = nullptr;
}
