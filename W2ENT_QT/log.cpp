#include "log.h"
#include <cstdio>
#include <stdarg.h>



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

Log::Log(irr::scene::ISceneManager* smgr, core::stringc filename, LogOutput output) : Smgr(smgr), Filename(filename), Output(output)
{
    if (!hasOutput(LOG_FILE))
        return;

#ifdef USE_FLUSH_PATCH
    LogFile = 0;
    if (Smgr)
        LogFile = Smgr->getFileSystem()->createAndWriteFile(Filename, false);
#else
    Content = "";
    if (Smgr)
        if (Smgr->getFileSystem()->existFile(Filename))
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

void Log::push()
{
    if (!hasOutput(LOG_FILE))
        return;

#ifdef USE_FLUSH_PATCH
    if (!LogFile)
        return;

    LogFile->flush();
#else
    irr::io::IWriteFile* file = 0;
    if (Smgr)
        file = Smgr->getFileSystem()->createAndWriteFile(Filename, true);
    if (file)
    {
        file->write(Content.c_str(), Content.size());
        Content = "";
        file->drop();
    }
#endif
}

void Log::addLineAndPush(core::stringc addition)
{
    addAndPush(addition.append('\n'));
}

void Log::addAndPush(core::stringc addition)
{
    add(addition);
    push();
}



bool Log::works()
{
    if (!hasOutput(LOG_FILE))
        return true;

#ifdef USE_FLUSH_PATCH
    return LogFile != 0;
#else
    irr::io::IWriteFile* file = 0;
    if (Smgr)
        file = Smgr->getFileSystem()->createAndWriteFile(Filename, true);
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

bool Log::hasOutput(LogOutput output)
{
    return (Output & output) != 0;
}

bool Log::isEnabled()
{
    return !hasOutput(LOG_NONE);
}
