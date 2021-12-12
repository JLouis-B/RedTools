#include "IrrFileLogger.h"

#include <IWriteFile.h>

IrrFileLogger::IrrFileLogger(irr::io::IFileSystem *fileSystem, irr::core::stringc filename) :
    FileSystem(fileSystem),
    Filename(filename)
{
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

void IrrFileLogger::close()
{
    #ifdef USE_FLUSH_PATCH
        if (LogFile)
            LogFile->drop();
        LogFile = nullptr;
    #else
        Content = "";
    #endif
}

void IrrFileLogger::log(irr::core::stringc content)
{
#ifdef USE_FLUSH_PATCH
    if (!LogFile)
        return;

    LogFile->write(content.c_str(), content.size());
#else
    Content += content;
#endif
}

void IrrFileLogger::flush()
{
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

bool IrrFileLogger::works()
{
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
