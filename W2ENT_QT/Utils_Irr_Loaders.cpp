#include "Utils_Irr_Loaders.h"

core::stringc readString(io::IReadFile* f, s32 nbLetters)
{
    core::stringc str;

    char buf;
    for (s32 i = 0; i < nbLetters; ++i)
    {
        f->read(&buf, 1);
        if (buf != 0)
            str.append(buf);
    }

    return str;
}

core::stringc readStringUntilNull(io::IReadFile* file)
{
    core::stringc returnedString;
    char c;
    while (1) {
       file->read(&c, 1);
       if (c == 0x00)
           break;
       returnedString.append(c);
    }

    return returnedString;
}

core::stringc readStringFixedSize(io::IReadFile* file, int count)
{
    core::stringc returnedString;
    char c;
    while (1) {
       file->read(&c, 1);
       if (c == 0x00)
           break;
       returnedString.append(c);
    }

    file->seek(count - (returnedString.size() + 1), true);

    return returnedString;
}
