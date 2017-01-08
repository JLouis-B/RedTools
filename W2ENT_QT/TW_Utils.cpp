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


void loadTW2StringsAndFiles(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files, bool withTypes)
{
    const long initialPos = file->getPos();
    file->seek(4);
    core::array<s32> header = readDataArray<s32>(file, 10);

    // strings
    file->seek(header[2]);
    for (int i = 0; i < header[3]; ++i)
    {
        strings.push_back(readString(file, readU8(file) -128));
    }

    // files
    file->seek(header[6]);
    for (int i = 0; i < header[7]; i++)
    {
        u8 format_name, size;
        file->read(&size, 1);
        file->read(&format_name, 1);

        file->seek(-1, true);

        if (format_name == 1)
            file->seek(1, true);

        core::stringc filename = readString(file, size);

        // Type of the file (CMesh, CMaterialInstance...)
        u32 fileTypeIndex = readU32(file) - 1;
        core::stringc fileType = strings[fileTypeIndex];

        core::stringc file = filename;
        if (withTypes)
            file = fileType + " : " + filename;

        //cout << file << endl;
        files.push_back(file);
    }


    file->seek(initialPos);
}

void loadTW3StringsAndFiles(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files)
{
    const long initialPos = file->getPos();

    file->seek(12);

    core::array<s32> headerData = readDataArray<s32>(file, 38);

    s32 stringChunkStart = headerData[7];
    s32 stringChunkSize = headerData[8];
    file->seek(stringChunkStart);
    while (file->getPos() - stringChunkStart < stringChunkSize)
    {
        core::stringc str = readStringUntilNull(file);
        strings.push_back(str);
    }

    s32 nbFiles = headerData[14];
    for (s32 i = 0; i < nbFiles; ++i)
    {
        files.push_back(strings[strings.size() - nbFiles + i]);
    }


    file->seek(initialPos);
}

void loadTWStringsAndFiles(io::IReadFile* file, WitcherFileType fileType, core::array<core::stringc>& strings, core::array<core::stringc>& files, bool withTypes)
{
    strings.clear();
    files.clear();

    switch (fileType)
    {
        case WFT_WITCHER_2:
            loadTW2StringsAndFiles(file, strings, files, withTypes);
            break;
        case WFT_WITCHER_3:
            loadTW3StringsAndFiles(file, strings, files);
        break;
        default:
            return;
    }
}
