#ifndef LOADERSUTILS_H
#define LOADERSUTILS_H

#include <irrlicht.h>

using namespace irr;

template <class T>
T readData(io::IReadFile* f)
{
    T buf;
    f->read(&buf, sizeof(T));
    return buf;
}

template <class T>
core::array<T> readDataArray(io::IReadFile* f, s32 nbElem)
{
    core::array<T> values;
    for (s32 i = 0; i < nbElem; ++i)
        values.push_back(readData<T>(f));

    return values;
}

core::stringc readString(io::IReadFile* f, s32 nbLetters);
core::stringc readStringUntilNull(io::IReadFile* file);
core::stringc readStringFixedSize(io::IReadFile* file, int count);



#endif // LOADERSUTILS_H
