#include "log.h"


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

    io::IWriteFile* logFile = 0;
    if (Smgr)
        logFile = Smgr->getFileSystem()->createAndWriteFile(Filename, false);
    if (logFile)
        logFile->drop();
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

void Log::addAndPush(core::stringc addition)
{
    if (!isEnabled())
        return;

    add(addition);
    push();
}



bool Log::works()
{
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
