#ifndef UTILS_TW_H
#define UTILS_TW_H

#include <IFileSystem.h>
#include <irrArray.h>

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
/*
struct WitcherFileDesc
{
    WitcherFileType _version;
    WitcherContentType _contentType;
    bool _hasWitcherMagicCode;

    // test _hasWitcherMagicCode + _version
    WitcherFileType _fileType;
};
*/

// unused yet. remove ?
//WitcherFileDesc getFullTWFileDescription(io::IReadFile* file, io::path filename);

struct TWFileHeader
{
    s32 Version;
    core::array<core::stringc> Strings;
    core::array<core::stringc> Files;
};

WitcherContentType getTWFileContentType(io::path filename);
WitcherFileType hasTWFileFormatVersion(io::IReadFile* file);
bool hasWitcherMagicCode(io::IReadFile* file);
WitcherFileType getTWFileType(io::IReadFile* file);

bool loadTWFileHeader(io::IReadFile* file, TWFileHeader &header, bool loadFilenamesWithTypes = false);
bool loadTW2FileHeader(io::IReadFile* file, TWFileHeader &header, bool loadFilenamesWithTypes = false);
bool loadTW3FileHeader(io::IReadFile* file, TWFileHeader &header);

#endif // UTILS_TW_H
