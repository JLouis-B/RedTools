#ifndef IRRFILELOGGER_H
#define IRRFILELOGGER_H

#include "IAppLogger.h"

#include <IFileSystem.h>

#define USE_FLUSH_PATCH

class IrrFileLogger : public IAppLogger
{
public:
    IrrFileLogger(irr::io::IFileSystem* fileSystem, irr::core::stringc filename);
    virtual ~IrrFileLogger() override { close(); }

    virtual void log(irr::core::stringc content) override;
    virtual void flush() override;

    bool works();

private:
    irr::io::IFileSystem* FileSystem;
    irr::core::stringc Filename;
    irr::core::stringc Content;

#ifdef USE_FLUSH_PATCH
    irr::io::IWriteFile* LogFile;
#endif

    void close();
};

#endif // IRRFILELOGGER_H
