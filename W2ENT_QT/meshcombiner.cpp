#include "meshcombiner.h"

scene::ISkinnedMesh* copySkinnedMesh(scene::ISceneManager* Smgr, scene::ISkinnedMesh* meshToCopy)
{
    scene::ISkinnedMesh* newMesh = Smgr->createSkinnedMesh();

    for (u32 i = 0; i < meshToCopy->getMeshBufferCount(); ++i)
    {
        scene::SSkinMeshBuffer* buffer = newMesh->addMeshBuffer();
        (*buffer) = (*(scene::SSkinMeshBuffer*)meshToCopy->getMeshBuffer(i));
    }

    for (u32 i = 0; i < meshToCopy->getJointCount(); ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = newMesh->addJoint();
        (*joint) = *(meshToCopy->getAllJoints()[i]);
    }


    //newMesh->setDirty();
    //newMesh->finalize();
    return newMesh;
}

void combineMeshes(scene::ISkinnedMesh* source, scene::ISkinnedMesh* addition)
{
    const u32 bufferIdOffset = source->getMeshBufferCount();
    for (u32 i = 0; i < addition->getMeshBufferCount(); ++i)
    {
        scene::SSkinMeshBuffer* buffer = source->addMeshBuffer();
        (*buffer) = (*(scene::SSkinMeshBuffer*)addition->getMeshBuffer(i));
    }

    for (u32 i = 0; i < addition->getJointCount(); ++i)
    {
        const scene::ISkinnedMesh::SJoint* jointToCopy = addition->getAllJoints()[i];
        scene::ISkinnedMesh::SJoint* joint = 0;
        if (source->getJointNumber(jointToCopy->Name.c_str()) == -1)
        {
            joint = source->addJoint();
            (*joint) = (*jointToCopy);

            for (u32 j = 0; j < joint->Weights.size(); ++j)
            {
                joint->Weights[j].buffer_id += bufferIdOffset;
            }
        }
        else
        {
            joint = source->getAllJoints()[source->getJointNumber(jointToCopy->Name.c_str())];
            for (u32 j = 0; j < jointToCopy->Weights.size(); ++j)
            {
                scene::ISkinnedMesh::SWeight* w = source->addWeight(joint);
                (*w) = jointToCopy->Weights[j];
                w->buffer_id += bufferIdOffset;
            }
        }


    }
}
