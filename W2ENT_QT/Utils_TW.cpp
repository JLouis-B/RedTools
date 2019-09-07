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

bool loadTW2FileHeader(io::IReadFile* file, TWFileHeader& header, bool loadFilenamesWithTypes)
{
    if (!file)
        return false;

    const long initialPos = file->getPos();
    file->seek(4);
    core::array<s32> headerData = readDataArray<s32>(file, 10);
    header.Version = headerData[0];

    // strings
    file->seek(headerData[2]);
    for (int i = 0; i < headerData[3]; ++i)
    {
        core::stringc string = readString(file, readU8(file) -128);
        header.Strings.push_back(string);
        Log::Instance()->addLineAndFlush(string);
    }

    // files
    file->seek(headerData[6]);
    for (int i = 0; i < headerData[7]; i++)
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
        core::stringc fileType = header.Strings[fileTypeIndex];

        core::stringc file = filename;
        if (loadFilenamesWithTypes)
            file = fileType + " : " + filename;

        Log::Instance()->addLineAndFlush(file);
        //cout << file << endl;
        header.Files.push_back(file);
    }


    file->seek(initialPos);

    return true;
}

bool loadTW3FileHeader(io::IReadFile* file, TWFileHeader &header)
{
    if (!file)
        return false;

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
            header.Strings.push_back(str);
            std::cout << "-->" << str.c_str() << std::endl;
            nbStringsRead++;
        }
        else
        {
            header.Files.push_back(str);
            std::cout << "--> FILE: " << str.c_str() << std::endl;
        }
    }

    file->seek(initialPos);

    return true;
}

bool loadTWFileHeader(io::IReadFile* file, TWFileHeader& header, bool loadFilenamesWithTypes)
{
    header.Strings.clear();
    header.Files.clear();

    if (!hasWitcherMagicCode(file))
        return false;

    WitcherFileType version = getTWFileType(file);

    switch (version)
    {
        case WFT_WITCHER_2:
            return loadTW2FileHeader(file, header, loadFilenamesWithTypes);
            break;
        case WFT_WITCHER_3:
            return loadTW3FileHeader(file, header);
        break;
        default:
            return false;
    }
}
