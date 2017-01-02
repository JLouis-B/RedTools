#ifndef TW_UTILS_H
#define TW_UTILS_H

#include <irrlicht.h>

using namespace irr;

enum WitcherFileType
{
    WFT_WITCHER_2,
    WFT_WITCHER_3,
    WFT_NOT_WITCHER
};

WitcherFileType checkTWFileFormatVersion(io::IReadFile* file);
bool checkTWFileExtension(io::path filename);


#endif // TW_UTILS_H
