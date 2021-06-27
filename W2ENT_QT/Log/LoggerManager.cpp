#include "LoggerManager.h"

#include <cstdarg>

LoggerManager LoggerManager::_instance = LoggerManager();

irr::core::stringc formatString(irr::core::stringc baseString, ...)
{
    irr::core::stringc newString = "";
    va_list va;
    va_start(va, baseString);

    char lastChar = 'a';
    for (irr::u32 i = 0; i < baseString.size(); ++i)
    {
        char currentChar = baseString[i];
        if (lastChar == '%')
        {
            switch(currentChar)
            {
                case 'd':
                {
                    int intValue = va_arg(va, int);
                    newString += toStr(intValue);
                    break;
                }
                case 'f':
                {
                    float floatValue = static_cast<float>(va_arg(va, double));
                    newString += toStr(floatValue);
                    break;
                }
                case 's':
                {
                    char* strValue = va_arg(va, char*);
                    newString += toStr(strValue);
                    break;
                }
                default:
                    break;
            }
        }

        if (currentChar != '%' && lastChar != '%')
            newString.append(currentChar);

        lastChar = currentChar;
    }

    va_end(va);
    return newString;
}


void LoggerManager::registerLogger(IAppLogger* logger, LoggerType loggerType)
{
    switch (loggerType) {
    case Logger_Dev:
        m_devLoggers.push_back(logger);
        break;

    case Logger_User:
        m_userLoggers.push_back(logger);
        break;

    default:
        break;
    }
}

void LoggerManager::unregisterLogger(IAppLogger* logger)
{
    for (unsigned int i = 0; i < m_devLoggers.size(); ++i)
    {
        if (m_devLoggers[i] == logger)
        {
            m_devLoggers.erase(m_devLoggers.begin() + i);
        }
    }

    for (unsigned int i = 0; i < m_userLoggers.size(); ++i)
    {
        if (m_userLoggers[i] == logger)
        {
            m_userLoggers.erase(m_userLoggers.begin() + i);
        }
    }
}

void LoggerManager::add(irr::core::stringc addition, bool logToUser)
{
    for (unsigned int i = 0; i < m_devLoggers.size(); ++i)
    {
        m_devLoggers[i]->log(addition);
    }

    if (logToUser)
    {
        for (unsigned int i = 0; i < m_userLoggers.size(); ++i)
        {
            m_userLoggers[i]->log(addition);
        }
    }
}

void LoggerManager::addLine(irr::core::stringc addition, bool logToUser)
{
    add(addition.append('\n'), logToUser);
}

void LoggerManager::flush()
{
    for (unsigned int i = 0; i < m_devLoggers.size(); ++i)
    {
        m_devLoggers[i]->flush();
    }

    for (unsigned int i = 0; i < m_userLoggers.size(); ++i)
    {
        m_userLoggers[i]->flush();
    }
}

void LoggerManager::addLineAndFlush(irr::core::stringc addition, bool logToUser)
{
    addAndFlush(addition.append('\n'), logToUser);
}

void LoggerManager::addAndFlush(irr::core::stringc addition, bool logToUser)
{
    add(addition, logToUser);
    flush();
}

LoggerManager* LoggerManager::Instance()
{
    return &_instance;
}
