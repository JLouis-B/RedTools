#include "TW_Utils.h"
#include "LoadersUtils.h"

bool checkTWFileExtension(io::path filename)
{
    return core::hasFileExtension ( filename, "w2ent" ) || core::hasFileExtension ( filename, "w2mesh" ) || core::hasFileExtension ( filename, "w2rig" ) || core::hasFileExtension ( filename, "w2anims" );
}

WitcherFileType checkTWFileFormatVersion(io::IReadFile* file)
{
    if (!file)
        return WFT_NOT_WITCHER;

    const long pos = file->getPos();

    file->seek(4);
    s32 version = readS32(file);
    file->seek(pos);

    if (version == 115)
        return WFT_WITCHER_2;
    else if (version >= 162)
        return WFT_WITCHER_3;
    else
        return WFT_NOT_WITCHER;
}
