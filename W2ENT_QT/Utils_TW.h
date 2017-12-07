#ifndef UTILS_TW_H
#define UTILS_TW_H

#include <irrlicht.h>
#include "Utils_Loaders.h"
#include "Log.h"

using namespace irr;

enum WitcherFileType
{
    WFT_WITCHER_2,
    WFT_WITCHER_3,
    WFT_NOT_WITCHER
};

WitcherFileType checkTWFileFormatVersion(io::IReadFile* file);
bool checkTWFileExtension(io::path filename);
WitcherFileType checkIsTWFile(io::IReadFile* file, io::path filename);
void loadTWStringsAndFiles(io::IReadFile* file, WitcherFileType fileType, core::array<core::stringc>& strings, core::array<core::stringc>& files, bool withTypes = false);
void loadTW2StringsAndFiles(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files, bool withTypes = false);

#endif // UTILS_TW_H
