#ifndef UTILS_LOADERS_H
#define UTILS_LOADERS_H

#include <ISkinnedMesh.h>
#include <IReadFile.h>
#include <irrArray.h>

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
bool readBool(io::IReadFile* file);

template <class T>
core::array<T> readDataArray(io::IReadFile* f, s32 nbElem)
{
    core::array<T> values(nbElem);
    values.set_used(nbElem);
    f->read(values.pointer(), nbElem * sizeof(T));
    return values;
}

core::stringc readString(io::IReadFile* file, int nbChars);
core::stringc readStringUntilNull(io::IReadFile* file);
core::stringc readStringFixedSize(io::IReadFile* file, int nbChars);


class JointHelper
{
public:
    static bool HasJoint(scene::ISkinnedMesh* mesh, core::stringc jointName);
    static scene::ISkinnedMesh::SJoint* GetJointByName(scene::ISkinnedMesh* mesh, core::stringc jointName);
};

#endif // UTILS_LOADERS_H
