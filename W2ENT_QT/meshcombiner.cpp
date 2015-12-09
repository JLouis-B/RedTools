#include "meshcombiner.h"

void combineMeshes(scene::ISkinnedMesh* source, scene::ISkinnedMesh* addition)
{
    for (u32 i = 0; i < addition->getMeshBufferCount(); ++i)
    {
        scene::SSkinMeshBuffer* buffer = source->addMeshBuffer();
        (*buffer) = (*(scene::SSkinMeshBuffer*)addition->getMeshBuffer(i));
    }

    for (u32 i = 0; i < addition->getJointCount(); ++i)
    {
        const scene::ISkinnedMesh::SJoint* jointToCopy = addition->getAllJoints()[i];
        if (source->getJointNumber(jointToCopy->Name.c_str()) != -1)
        {
            scene::ISkinnedMesh::SJoint* joint = source->addJoint();
            (*joint) = *(jointToCopy);
        }
    }
}
