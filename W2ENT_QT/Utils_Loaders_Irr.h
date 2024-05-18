#ifndef UTILS_LOADERS_H
#define UTILS_LOADERS_H

#include <ISkinnedMesh.h>
#include <IReadFile.h>
#include <irrArray.h>

using namespace irr;

// Check if what we read is still in the file size (to know if we will crash!), and log if it's the case
void checkOutOfBound(io::IReadFile* file, size_t sizeToRead);

template <class T>
T readData(io::IReadFile* file)
{
    size_t sizeToRead = sizeof(T);
    checkOutOfBound(file, sizeToRead);

    T buf;
    file->read(&buf, sizeToRead);
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
core::array<T> readDataArray(io::IReadFile* file, s32 arraySize)
{
    size_t sizeToRead = arraySize * sizeof(T);
    checkOutOfBound(file, sizeToRead);

    core::array<T> values(arraySize);
    values.set_used(arraySize);
    file->read(values.pointer(), sizeToRead);
    return values;
}

core::stringc readString(io::IReadFile* file, int nbChars);
core::stringc readStringUntilNull(io::IReadFile* file);

void chechNaNErrors(core::vector3df& vector3);

// for debugging purposes
core::stringc getBinaryRepresentation(u8 byte);

class JointHelper
{
public:
    static bool HasJoint(const scene::ISkinnedMesh* mesh, const core::stringc jointName);
    static scene::ISkinnedMesh::SJoint* GetJointByName(const scene::ISkinnedMesh* mesh, const core::stringc jointName);
    static scene::ISkinnedMesh::SJoint* GetParent(const scene::ISkinnedMesh* mesh, const scene::ISkinnedMesh::SJoint* joint);
    static void ComputeGlobalMatrixRecursive(const scene::ISkinnedMesh* mesh, scene::ISkinnedMesh::SJoint* joint);
    static core::array<scene::ISkinnedMesh::SJoint*> GetRoots(const scene::ISkinnedMesh* mesh);
    static void SetParent(const scene::ISkinnedMesh* mesh, scene::ISkinnedMesh::SJoint* joint, scene::ISkinnedMesh::SJoint* parent);
    static void DebugJointsHierarchy(const scene::ISkinnedMesh* mesh);
};

#endif // UTILS_LOADERS_H
