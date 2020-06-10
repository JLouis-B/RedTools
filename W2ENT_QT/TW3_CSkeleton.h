#ifndef TW3_CSKELETON_H
#define TW3_CSKELETON_H

#include <ISkinnedMesh.h>

using namespace irr;

class TW3_CSkeleton
{
public:
    TW3_CSkeleton();

    u32 nbBones;
    core::array<core::stringc> names;
    core::array<short> parentId;
    core::array<core::matrix4> matrix;

    core::array<core::vector3df> positions;
    core::array<core::quaternion> rotations;
    core::array<core::vector3df> scales;

    bool applyToModel(scene::ISkinnedMesh* mesh);
};

#endif // TW3_CSKELETON_H
