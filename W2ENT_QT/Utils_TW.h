#ifndef UTILS_TW_H
#define UTILS_TW_H

#include <irrlicht.h>
#include "Utils_Irr_Loaders.h"
#include "Log.h"

using namespace irr;

enum WitcherFileType
{
    WFT_WITCHER_2,
    WFT_WITCHER_3,
    WFT_NOT_WITCHER
};

enum WitcherContentType
{
    WTC_ENTITY,
    WTC_MESH,
    WTC_RIG,
    WTC_ANIMATIONS,
    WTC_MATERIAL,
    WTC_OTHER
};

struct WitcherFileDesc
{
    WitcherFileType _type;
    WitcherContentType _contentType;
};

WitcherFileDesc getTWFileDescription(io::IReadFile* file, io::path filename);

WitcherContentType getTWFileContentType(io::path filename);
WitcherFileType getTWFileFormatVersion(io::IReadFile* file);
WitcherFileType checkIsTWFile(io::IReadFile* file, io::path filename);
void loadTWStringsAndFiles(io::IReadFile* file, WitcherFileType fileType, core::array<core::stringc>& strings, core::array<core::stringc>& files, bool withTypes = false);
void loadTW2StringsAndFiles(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files, bool withTypes = false);

#endif // UTILS_TW_H
