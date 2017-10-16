#ifndef LOG_H
#define LOG_H

#include <irrlicht.h>

#include <iostream>
#include <sstream>

//#define USE_FLUSH_PATCH

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
    Log(irr::scene::ISceneManager* smgr = 0, core::stringc filename = "", LogOutput output = LOG_NONE);
    ~Log();

    void add(core::stringc addition);
    void addLine(core::stringc addition);
    void addAndPush(core::stringc addition);
    void addLineAndPush(core::stringc addition);
    void push();

    bool isEnabled();
    bool works();

    void setOutput(LogOutput output);

private:

    irr::scene::ISceneManager* Smgr;
    core::stringc Filename;
    core::stringc Content;

    LogOutput Output;
    bool hasOutput(LogOutput output);

    #ifdef USE_FLUSH_PATCH
    irr::io::IWriteFile* LogFile;
    #endif
};

#endif // LOG_H
