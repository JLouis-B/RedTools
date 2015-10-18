#ifndef LOG_H
#define LOG_H

#include <irrlicht.h>

//#define USE_FLUSH_PATCH

using namespace irr;

class Log
{
public:

    //Log(irr::scene::ISceneManager* smgr = 0);
    Log(irr::scene::ISceneManager* smgr = 0, core::stringc filename = "");
    ~Log();

    void add(core::stringc addition);
    void push();
    void addAndPush(core::stringc addition);

    void enable(bool enabled);
    bool works();

private:
    irr::scene::ISceneManager* Smgr;
    core::stringc Filename;
    core::stringc Content;
    bool Enabled;

    #ifdef USE_FLUSH_PATCH
    irr::io::IWriteFile* LogFile;
    #endif
};

#endif // LOG_H
