#include "Utils_Loaders_Irr.h"

bool readBool(io::IReadFile* file)
{
    u8 valChar = readU8(file);
    return (valChar > 0);
}

core::stringc readString(io::IReadFile* file, s32 nbChars)
{
    char returnedString[nbChars + 1];
    file->read(returnedString, nbChars);
    returnedString[nbChars] = '\0';
    return returnedString;
}

core::stringc readStringUntilNull(io::IReadFile* file)
{
    core::stringc returnedString;
    char c;
    while (1)
    {
       file->read(&c, 1);
       if (c == 0x00)
           break;
       returnedString.append(c);
    }

    return returnedString;
}

core::stringc readStringFixedSize(io::IReadFile* file, int nbChars)
{
    long back = file->getPos();
    core::stringc returnedString = readString(file, nbChars);
    file->seek(back + nbChars);

    return returnedString;
}
