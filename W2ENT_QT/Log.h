#ifndef LOG_H
#define LOG_H

#include <IFileSystem.h>

#include <sstream>
#include <functional>
#include <vector>

#define USE_FLUSH_PATCH

using namespace irr;


template <typename T>
core::stringc toStr(const T& t) {
   std::ostringstream os;
   os << t;
   return core::stringc(os.str().c_str());
}
core::stringc formatString(core::stringc baseString, ...);

enum LogOutput
{
    LOG_NONE    = 0,
    LOG_FILE    = 1,
    LOG_CONSOLE = 2
};

inline LogOutput& operator|=(LogOutput& a, LogOutput b)
{
    return reinterpret_cast<LogOutput&>(reinterpret_cast<int&>(a) |= static_cast<int>(b));
}

inline LogOutput operator&(LogOutput a, LogOutput b)
{
    return static_cast<LogOutput>(static_cast<int>(a) & static_cast<int>(b));
}

class Log
{
public:
    Log();
    ~Log();

    void create(io::IFileSystem* fileSystem, core::stringc filename);
    void close();

    void add(core::stringc addition, bool logToUser = false);
    void addLine(core::stringc addition, bool logToUser = false);
    void addAndFlush(core::stringc addition, bool logToUser = false);
    void addLineAndFlush(core::stringc addition, bool logToUser = false);
    void flush();

    bool isEnabled();
    bool works();

    void setOutput(LogOutput output);
    void addOutput(LogOutput output);

    void registerLogToUserCallback(std::function<void(core::stringc)>);
    void unregisterLogToUserCallback();

    static Log *Instance();

private:

    io::IFileSystem* FileSystem;
    core::stringc Filename;
    core::stringc Content;

    std::function<void(core::stringc)> LogToUserCallback;

    LogOutput Output;
    bool hasOutput(LogOutput output);

    #ifdef USE_FLUSH_PATCH
    irr::io::IWriteFile* LogFile;
    #endif

    void logToUserCallbacks(core::stringc text);

    static Log _instance;
};

#endif // LOG_H
