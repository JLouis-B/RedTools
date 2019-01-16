#include "Utils_TW.h"

#include "Utils_Loaders_Irr.h"
#include "Log.h"

#include <iostream>

// useless ?
/*
WitcherFileDesc getFullTWFileDescription(io::IReadFile* file, io::path filename)
{
    WitcherFileDesc description;
    description._contentType = getTWFileContentType(filename);
    description._version = hasTWFileFormatVersion(file);
    description._hasWitcherMagicCode = hasWitcherMagicCode(file);

    description._fileType = getTWFileType(file);
    return description;
}
*/

WitcherContentType getTWFileContentType(io::path filename)
{
    if (core::hasFileExtension ( filename, "w2ent" ))
        return WCT_WITCHER_ENTITY;
    else if (core::hasFileExtension ( filename, "w2mesh" ))
        return WCT_WITCHER_MESH;
    else if (core::hasFileExtension ( filename, "w2rig" ))
        return WCT_WITCHER_RIG;
    else if (core::hasFileExtension ( filename, "w2anims" ))
        return WCT_WITCHER_ANIMATIONS;
    else if (core::hasFileExtension ( filename, "w2mi" ))
        return WCT_WITCHER_MATERIAL;
    else
        return WCT_WITCHER_OTHER;
}

WitcherFileType hasTWFileFormatVersion(io::IReadFile* file)
{
    if (!file)
        return WFT_UNKNOWN;

    const long pos = file->getPos();

    file->seek(4);
    s32 version = readS32(file);
    file->seek(pos);

    if (version == 115)
        return WFT_WITCHER_2;
    else if (version >= 162)
        return WFT_WITCHER_3;
    else
        return WFT_UNKNOWN;
}

bool hasWitcherMagicCode(io::IReadFile* file)
{
    if (!file)
        return false;

    const long pos = file->getPos();
    core::stringc magic = readString(file, 4);
    file->seek(pos);

    return (magic == "CR2W");
}

WitcherFileType getTWFileType(io::IReadFile* file)
{
    if (!hasWitcherMagicCode(file))
        return WFT_UNKNOWN;

    return hasTWFileFormatVersion(file);
}

void loadTW2StringsAndFiles(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files, bool withTypes)
{
    if (!file)
        return;

    const long initialPos = file->getPos();
    file->seek(4);
    core::array<s32> header = readDataArray<s32>(file, 10);

    // strings
    file->seek(header[2]);
    for (int i = 0; i < header[3]; ++i)
    {
        strings.push_back(readString(file, readU8(file) -128));
        Log::Instance()->addLineAndFlush(strings[i]);
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

        Log::Instance()->addLineAndFlush(file);
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
    // debug
    /*
    for (int i = 0; i < 38; ++i)
    {
        std::cout << "Header data [" << i << "]: " << headerData[i] << std::endl;
    }
    */

    s32 stringChunkStart = headerData[7];
    s32 stringChunkSize = headerData[8];
    s32 calculatedStringChunkSize = stringChunkStart + stringChunkSize;

    s32 stringChunkEnd = headerData[10]; // or the adress of a new chunk ?
    s32 nbStrings = headerData[11];

    // in many case seem similar to file count, but no
    //s32 nbFiles = headerData[14];

    int nbStringsRead = 0;
    file->seek(stringChunkStart);
    while (file->getPos() < calculatedStringChunkSize)
    {
        core::stringc str = readStringUntilNull(file);
        if (nbStringsRead < nbStrings)
        {
            strings.push_back(str);
            std::cout << "-->" << str.c_str() << std::endl;
            nbStringsRead++;
        }
        else
        {
            files.push_back(str);
            std::cout << "--> FILE: " << str.c_str() << std::endl;
        }
    }

    file->seek(initialPos);
}

void loadTWStringsAndFiles(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files, bool withTypes)
{
    strings.clear();
    files.clear();

    if (!hasWitcherMagicCode(file))
        return;

    WitcherFileType version = getTWFileType(file);

    switch (version)
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
