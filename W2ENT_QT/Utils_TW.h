#ifndef UTILS_TW_H
#define UTILS_TW_H

#include <irrlicht.h>
#include "Utils_Loaders_Irr.h"
#include "Log.h"

using namespace irr;

enum WitcherFileType
{
    WFT_WITCHER_2,
    WFT_WITCHER_3,
    WFT_UNKNOWN
};

enum WitcherContentType
{
    WCT_WITCHER_ENTITY,
    WCT_WITCHER_MESH,
    WCT_WITCHER_RIG,
    WCT_WITCHER_ANIMATIONS,
    WCT_WITCHER_MATERIAL,
    WCT_WITCHER_OTHER
};

// unused yet. remove ?
struct WitcherFileDesc
{
    WitcherFileType _version;
    WitcherContentType _contentType;
    bool _hasWitcherMagicCode;

    // test _hasWitcherMagicCode + _version
    WitcherFileType _fileType;
};

// unused yet. remove ?
WitcherFileDesc getFullTWFileDescription(io::IReadFile* file, io::path filename);

WitcherContentType getTWFileContentType(io::path filename);
WitcherFileType hasTWFileFormatVersion(io::IReadFile* file);
bool hasWitcherMagicCode(io::IReadFile* file);
WitcherFileType getTWFileType(io::IReadFile* file);

void loadTWStringsAndFiles(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files, bool withTypes = false);
void loadTW2StringsAndFiles(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files, bool withTypes = false);
void loadTW3StringsAndFiles(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files);

#endif // UTILS_TW_H
