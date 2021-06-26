#include "Log.h"

#include <cstdarg>
#include <iostream>

#include <IWriteFile.h>

Log Log::_instance = Log();

core::stringc formatString(core::stringc baseString, ...)
{
    core::stringc newString = "";
    va_list va;
    va_start(va, baseString);

    char lastChar = 'a';
    for (u32 i = 0; i < baseString.size(); ++i)
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

Log::Log() :
    FileSystem(nullptr),
    Filename(core::stringc()),
    Output(LOG_NONE)
{
    #ifdef USE_FLUSH_PATCH
        LogFile = nullptr;
    #endif
}

Log::~Log()
{
    addLineAndFlush("Close the log file");
    close();
}

void Log::create(io::IFileSystem* fileSystem, core::stringc filename)
{
    FileSystem = fileSystem;
    Filename = filename;

    if (!hasOutput(LOG_FILE))
        return;

    close();

#ifdef USE_FLUSH_PATCH
    if (FileSystem)
        LogFile = FileSystem->createAndWriteFile(Filename, false);
#else
    if (FileSystem && FileSystem->existFile(Filename))
            remove(Filename.c_str());
#endif

}

void Log::close()
{
    #ifdef USE_FLUSH_PATCH
        if (LogFile)
            LogFile->drop();
        LogFile = nullptr;
    #else
        Content = "";
    #endif
}

void Log::add(core::stringc addition, bool logToUser)
{
    if (hasOutput(LOG_CONSOLE))
        std::cout << addition.c_str();
    if (!hasOutput(LOG_FILE))
        return;

#ifdef USE_FLUSH_PATCH
    if (!LogFile)
        return;

    LogFile->write(addition.c_str(), addition.size());
#else
    Content += addition;
#endif

    if (logToUser)
    {
        logToUserCallbacks(addition);
    }
}

void Log::addLine(core::stringc addition, bool logToUser)
{
    add(addition.append('\n'), logToUser);
}

void Log::flush()
{
    if (hasOutput(LOG_CONSOLE))
        std::cout.flush();

    if (!hasOutput(LOG_FILE))
        return;

#ifdef USE_FLUSH_PATCH
    if (!LogFile)
        return;

    LogFile->flush();
#else
    irr::io::IWriteFile* file = 0;
    if (FileSystem)
        file = FileSystem->createAndWriteFile(Filename, true);
    if (file)
    {
        file->write(Content.c_str(), Content.size());
        Content = "";
        file->drop();
    }
#endif
}

void Log::addLineAndFlush(core::stringc addition, bool _logToUser)
{
    addAndFlush(addition.append('\n'), _logToUser);
}

void Log::addAndFlush(core::stringc addition, bool _logToUser)
{
    add(addition);
    flush();

    if (_logToUser)
    {
        logToUserCallbacks(addition);
    }
}

void Log::logToUserCallbacks(core::stringc text)
{
    if (LogToUserCallback != nullptr)
    {
        LogToUserCallback(text);
    }
}

bool Log::works()
{
    if (!hasOutput(LOG_FILE))
        return true;

#ifdef USE_FLUSH_PATCH
    return LogFile != nullptr;
#else
    irr::io::IWriteFile* file = nullptr;
    if (FileSystem)
        file = FileSystem->createAndWriteFile(Filename, true);
    if (file)
    {
        file->drop();
        return true;
    }
    return false;
#endif
}

void Log::setOutput(LogOutput output)
{
    Output = output;
}

void Log::addOutput(LogOutput output)
{
    Output |= output;
}

bool Log::hasOutput(LogOutput output)
{
    return (Output & output) != 0;
}

bool Log::isEnabled()
{
    return Output != LOG_NONE;
}

void Log::registerLogToUserCallback(std::function<void(core::stringc)> callback)
{
    LogToUserCallback = callback;
}

void Log::unregisterLogToUserCallback()
{
    LogToUserCallback = nullptr;
}

Log* Log::Instance()
{
    return &_instance;
}
