#ifndef CSKELETON_H
#define CSKELETON_H

#include <irrlicht.h>

using namespace irr;

scene::ISkinnedMesh::SJoint* getJointByName(scene::ISkinnedMesh* mesh, core::stringc name);

class CSkeleton
{
public:
    CSkeleton();

    u32 nbBones;
    core::array<core::stringc> names;
    core::array<short> parentId;
    core::array<core::matrix4> matrix;

    core::array<core::vector3df> positions;
    core::array<core::quaternion> rotations;
    core::array<core::vector3df> scales;

    bool applyToModel(scene::ISkinnedMesh* mesh);
};

#endif // CSKELETON_H
