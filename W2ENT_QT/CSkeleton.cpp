#include "CSkeleton.h"

#include <iostream>

CSkeleton::CSkeleton() : nbBones(0)
{

}
core::array<scene::ISkinnedMesh::SJoint*> getRootJoints(const scene::ISkinnedMesh* mesh)
{
    core::array<scene::ISkinnedMesh::SJoint*> roots;

    core::array<scene::ISkinnedMesh::SJoint*> allJoints = mesh->getAllJoints();
    for (u32 i = 0; i < allJoints.size(); i++)
    {
        bool isRoot = true;
        scene::ISkinnedMesh::SJoint* testedJoint = allJoints[i];
        for (u32 j = 0; j < allJoints.size(); j++)
        {
           scene::ISkinnedMesh::SJoint* testedJoint2 = allJoints[j];
           for (u32 k = 0; k < testedJoint2->Children.size(); k++)
           {
               if (testedJoint == testedJoint2->Children[k])
                    isRoot = false;
           }
        }
        if (isRoot)
            roots.push_back(testedJoint);
    }

    return roots;
}

scene::ISkinnedMesh::SJoint* getRealParent(const scene::ISkinnedMesh* mesh, scene::ISkinnedMesh::SJoint* joint)
{
    core::array<scene::ISkinnedMesh::SJoint*> allJoints = mesh->getAllJoints();
    for (u32 j = 0; j < allJoints.size(); j++)
    {
       scene::ISkinnedMesh::SJoint* testedJoint = allJoints[j];
       for (u32 k = 0; k < testedJoint->Children.size(); k++)
       {
           if (joint == testedJoint->Children[k])
                return testedJoint;
       }
    }

    return 0;
}


void computeLocal(const scene::ISkinnedMesh* mesh, scene::ISkinnedMesh::SJoint* joint)
{
    // Parent bone is necessary to compute the local matrix from global
    scene::ISkinnedMesh::SJoint* jointParent = getRealParent(mesh, joint);

    if (jointParent)
    {
        irr::core::matrix4 globalParent = jointParent->GlobalMatrix;
        irr::core::matrix4 invGlobalParent;
        globalParent.getInverse(invGlobalParent);

        joint->LocalMatrix = invGlobalParent * joint->GlobalMatrix;
    }
    else
        joint->LocalMatrix = joint->GlobalMatrix;
}


scene::ISkinnedMesh::SJoint* getJointByName(scene::ISkinnedMesh* mesh, core::stringc name)
{
    //std::cout << "count= " << mesh->getJointCount() << std::endl;
    //for (u32 i = 0; i < mesh->getJointCount(); ++i)
    //    std::cout << mesh->getAllJoints()[i]->Name.c_str() << std::endl;

    s32 jointID = mesh->getJointNumber(name.c_str());
    if (jointID == -1)
        return 0;

    return mesh->getAllJoints()[jointID];
}


bool CSkeleton::applyToModel(scene::ISkinnedMesh* mesh)
{
    // is it the good skeleton ?
    //if (!checkIfPerfectlyCorresponding(mesh))
    //    return false;

    // Clear the previous hierarchy
    for (u32 i = 0; i < mesh->getJointCount(); ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i];
        joint->Children.clear();
    }

    // Set the hierarchy
    for (u32 i = 0; i < nbBones; ++i)
    {
        core::stringc boneName = names[i];
        scene::ISkinnedMesh::SJoint* joint = getJointByName(mesh, boneName);
        if (!joint)
        {
            joint = mesh->addJoint();
            joint->Name = boneName;
        }

        scene::ISkinnedMesh::SJoint* parentJoint = 0;
        short parent = i;
        while (!parentJoint && parent != -1)
        {
            parent = parentId[parent];
            if (parent == -1)   // root
                break;

            parentJoint = getJointByName(mesh, names[parent]);
            if (parentJoint)
                parentJoint->Children.push_back(joint);
        }

    }

    // Set matrix from CSkeleton
    for (u32 i = 0; i < nbBones; ++i)
    {
        core::stringc boneName = names[i];

        scene::ISkinnedMesh::SJoint* joint = getJointByName(mesh, boneName);
        if (!joint)
            continue;

        core::matrix4 mat = matrix[i];
        joint->LocalMatrix = mat;

        joint->Animatedposition = positions[i];
        joint->Animatedrotation = rotations[i];
        joint->Animatedrotation.makeInverse();
        joint->Animatedscale = scales[i];
    }

    return true;
}

bool CSkeleton::checkIfPerfectlyCorresponding(scene::ISkinnedMesh* mesh)
{
    for (u32 i = 0; i < nbBones; ++i)
    {
        core::stringc bone = names[i];
        //std::cout << bone.c_str() << std::endl;

        scene::ISkinnedMesh::SJoint* joint = getJointByName(mesh, bone);
        if (!joint)
            return false;
    }

    return true;
}
