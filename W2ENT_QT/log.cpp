#include "log.h"



Log::Log(irr::scene::ISceneManager* smgr, core::stringc filename) : Smgr(smgr), Filename(filename)
{
    Enabled = true;

#ifdef USE_FLUSH_PATCH
    LogFile = 0;
    if (Smgr)
        LogFile = Smgr->getFileSystem()->createAndWriteFile(Filename, false);
#else
    Content = "";
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
        file = Smgr->getFileSystem()->createAndWriteFile(Filename);
    if (file)
    {
        file->write(Content.c_str(), Content.size());
        file->drop();
    }
#endif
}

void Log::addAndPush(core::stringc addition)
{
    if (!Enabled)
        return;

    add(addition);

#ifdef USE_FLUSH_PATCH
    push();
#else
    irr::io::IWriteFile* file = 0;
    if (Smgr)
        file = Smgr->getFileSystem()->createAndWriteFile(Filename, true);
    if (file)
    {
        file->write(addition.c_str(), addition.size());
        file->drop();
    }
#endif

}

void Log::enable(bool enabled)
{
    Enabled = enabled;
}

bool Log::works()
{
#ifdef USE_FLUSH_PATCH
    return LogFile != 0;
#else
    return true;
#endif
}
