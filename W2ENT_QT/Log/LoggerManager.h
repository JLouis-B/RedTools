#ifndef LOGGER_MANAGER_H
#define LOGGER_MANAGER_H

#include <vector>
#include <sstream>

#include "IAppLogger.h"

template <typename T>
irr::core::stringc toStr(const T& t) {
   std::ostringstream os;
   os << t;
   return irr::core::stringc(os.str().c_str());
}
irr::core::stringc formatString(irr::core::stringc baseString, ...);

enum LoggerType
{
    Logger_Dev,
    Logger_User
};


class LoggerManager
{
public:
    LoggerManager() {}
    ~LoggerManager() {}

    void registerLogger(IAppLogger* logger, LoggerType loggerType);
    void unregisterLogger(IAppLogger* logger);

    void add(irr::core::stringc addition, bool logToUser = false);
    void addLine(irr::core::stringc addition, bool logToUser = false);
    void addAndFlush(irr::core::stringc addition, bool logToUser = false);
    void addLineAndFlush(irr::core::stringc addition, bool logToUser = false);
    void flush();

    bool hasActiveLoggers() { return m_devLoggers.size() + m_userLoggers.size();}

    static LoggerManager* Instance();

private:
    std::vector<IAppLogger*> m_devLoggers;
    std::vector<IAppLogger*> m_userLoggers;

    static LoggerManager _instance;
};

#endif
