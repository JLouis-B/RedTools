#include "log.h"
#include <cstdio>
#include <stdarg.h>

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
                    float floatValue = va_arg(va, double);
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

Log::Log() : FileSystem(nullptr), Filename(core::stringc()), Output(LOG_NONE)
{

}

void Log::create(io::IFileSystem* fileSystem, core::stringc filename)
{
    FileSystem = fileSystem;
    Filename = filename;

    if (!hasOutput(LOG_FILE))
        return;

#ifdef USE_FLUSH_PATCH
    LogFile = nullptr;
    if (FileSystem)
        LogFile = FileSystem->createAndWriteFile(Filename, false);
#else
    Content = "";
    if (FileSystem && FileSystem->existFile(Filename))
            remove(Filename.c_str());
#endif

}

Log::~Log()
{
    #ifdef USE_FLUSH_PATCH
        if (LogFile)
            LogFile->drop();
    #else
    #endif
}

void Log::add(core::stringc addition)
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
}

void Log::addLine(core::stringc addition)
{
    add(addition.append('\n'));
}

void Log::flush()
{
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

void Log::addLineAndFlush(core::stringc addition)
{
    addAndFlush(addition.append('\n'));
}

void Log::addAndFlush(core::stringc addition)
{
    add(addition);
    flush();
}



bool Log::works()
{
    if (!hasOutput(LOG_FILE))
        return true;

#ifdef USE_FLUSH_PATCH
    return LogFile != 0;
#else
    irr::io::IWriteFile* file = 0;
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

Log* Log::Instance()
{
    return &_instance;
}
