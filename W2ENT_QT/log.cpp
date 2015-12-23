#include "log.h"

Log::Log(irr::scene::ISceneManager* smgr, core::stringc filename) : Smgr(smgr), Filename(filename)
{
    Enabled = true;
    ConsoleOutput = false;


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
#ifdef USE_FLUSH_PATCH
    if (!LogFile)
        return;

    LogFile->write(addition.c_str(), addition.size());
#else
    Content += addition;
#endif

    if (ConsoleOutput)
        std::cout << addition.c_str();
}

void Log::push()
{
    if (!Enabled)
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
    if (!Enabled)
        return;

    add(addition);
    push();
}

void Log::enable(bool enabled)
{
    Enabled = enabled;
}

bool Log::isEnabled()
{
    return Enabled;
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

void Log::setConsoleOutput(bool enabled)
{
    ConsoleOutput = enabled;
}
