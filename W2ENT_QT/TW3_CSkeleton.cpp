#include "TW3_CSkeleton.h"

#include "Utils_Loaders_Irr.h"

#include <iostream>

TW3_CSkeleton::TW3_CSkeleton() : nbBones(0)
{

}

// Definition in IrrAssimpExport
core::array<scene::ISkinnedMesh::SJoint*> getRootJoints(const scene::ISkinnedMesh* mesh);





void computeLocal(const scene::ISkinnedMesh* mesh, scene::ISkinnedMesh::SJoint* joint)
{
    // Parent bone is necessary to compute the local matrix from global
    scene::ISkinnedMesh::SJoint* jointParent = JointHelper::GetParent(mesh, joint);

    if (jointParent)
    {
        core::matrix4 globalParent = jointParent->GlobalMatrix;
        core::matrix4 invGlobalParent;
        globalParent.getInverse(invGlobalParent);

        joint->LocalMatrix = invGlobalParent * joint->GlobalMatrix;
    }
    else
        joint->LocalMatrix = joint->GlobalMatrix;
}




bool TW3_CSkeleton::applyToModel(scene::ISkinnedMesh* mesh)
{
    // Create the bones
    for (u32 i = 0; i < nbBones; ++i)
    {
        core::stringc boneName = names[i];
        scene::ISkinnedMesh::SJoint* joint = JointHelper::GetJointByName(mesh, boneName);
        if (!joint)
        {
            joint = mesh->addJoint();
            joint->Name = boneName;
        }
    }

    // Set the hierarchy
    core::array<scene::ISkinnedMesh::SJoint*> roots;
    for (u32 i = 0; i < nbBones; ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i]; // TODO: this is probably buggy (need to use JointHelper::GetJointByName(mesh, names[i]))
        s16 parent = parentId[i];
        if (parent != -1) // root
        {
            scene::ISkinnedMesh::SJoint* parentJoint = JointHelper::GetJointByName(mesh, names[parent]);
            if (parentJoint)
                parentJoint->Children.push_back(joint);
        }
        else
        {
            roots.push_back(joint);
        }
    }

    // Set the transformations
    for (u32 i = 0; i < nbBones; ++i)
    {
        core::stringc boneName = names[i];

        scene::ISkinnedMesh::SJoint* joint = JointHelper::GetJointByName(mesh, boneName);
        if (!joint)
            continue;

        core::matrix4 mat = matrix[i];
        joint->LocalMatrix = mat;

        joint->Animatedposition = positions[i];
        joint->Animatedrotation = rotations[i];
        joint->Animatedscale = scales[i];
    }

    // Compute the global matrix
    for (u32 i = 0; i < roots.size(); ++i)
    {
        JointHelper::ComputeGlobalMatrixRecursive(mesh, roots[i]);
    }

    return true;
}
