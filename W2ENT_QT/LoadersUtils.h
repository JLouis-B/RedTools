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

#define readU32 readData<u32>
#define readS32 readData<s32>
#define readU16 readData<u16>
#define readS16 readData<s16>
#define readU8 readData<u8>
#define readS8 readData<s8>
#define readF32 readData<f32>


template <class T>
core::array<T> readDataArray(io::IReadFile* f, s32 nbElem)
{
    core::array<T> values(nbElem);
    values.set_used(nbElem);
    f->read(values.pointer(), nbElem * sizeof(T));
    return values;
}

core::stringc readString(io::IReadFile* f, s32 nbLetters);
core::stringc readStringUntilNull(io::IReadFile* file);
core::stringc readStringFixedSize(io::IReadFile* file, int count);



#endif // LOADERSUTILS_H
