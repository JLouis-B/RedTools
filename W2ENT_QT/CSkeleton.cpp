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


scene::ISkinnedMesh* CSkeleton::copySkinnedMesh(scene::ISceneManager* Smgr, scene::ISkinnedMesh* meshToCopy)
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

void computeLocal(const scene::ISkinnedMesh* mesh, scene::ISkinnedMesh::SJoint* joint)
{
    // Parent bone is necessary to compute the local matrix from global
    scene::ISkinnedMesh::SJoint* jointParent = getRealParent(mesh, joint);

    if (jointParent)
    {
        //
        //if (jointParent->LocalMatrix == jointParent->GlobalMatrix)
            computeLocal(mesh, jointParent);

        irr::core::matrix4 globalParent = jointParent->GlobalMatrix;
        irr::core::matrix4 invGlobalParent;
        globalParent.getInverse(invGlobalParent);

        joint->LocalMatrix = invGlobalParent * joint->GlobalMatrix;
    }
    else
        joint->LocalMatrix = joint->GlobalMatrix;
    // -----------------------------------------------------------------
}


scene::ISkinnedMesh::SJoint* getJointByName(scene::ISkinnedMesh* mesh, core::stringc name)
{
    std::cout << "count= " << mesh->getJointCount() << std::endl;
    for (u32 i = 0; i < mesh->getJointCount(); ++i)
        std::cout << mesh->getAllJoints()[i]->Name.c_str() << std::endl;

    s32 jointID = mesh->getJointNumber(name.c_str());
    if (jointID == -1)
        return 0;

    return mesh->getAllJoints()[jointID];
}


bool CSkeleton::applyToModel(scene::ISkinnedMesh* mesh)
{
    std::cout << "enter here" << std::endl;
    // is it the good skeleton ?
    //if (!checkIfPerfectlyCorresponding(mesh))
    //    return false;

    // Clear the previous hierarchy
    for (u32 i = 0; i < mesh->getJointCount(); ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i];
        joint->Children.clear();
    }

    for (u32 i = 0; i < nbBones; ++i)
    {
        core::stringc bone = names[i];

        scene::ISkinnedMesh::SJoint* joint = getJointByName(mesh, bone);
        if (!joint)
        {
            //joint = mesh->addJoint();
            //joint->Name = bone;
            continue;
        }


        short parent = parentId[i];
        if (parent == -1)   // root
            continue;

        scene::ISkinnedMesh::SJoint* parentJoint = getJointByName(mesh, names[parent]);
        if (parentJoint)
            parentJoint->Children.push_back(joint);

    }
std::cout << "enter here" << std::endl;
/*
    for (u32 i = 0; i < nbBones; ++i)
    {
        core::stringc bone = names[i];

        scene::ISkinnedMesh::SJoint* joint = getJointByName(mesh, bone);
        if (!joint)
            continue;

        core::matrix4 mat = matrix[i];
        joint->GlobalMatrix = mat;

        core::matrix4 matr = mat;
        irr::core::vector3df position = matr.getTranslation();
        irr::core::matrix4 invRot;
        matr.getInverse(invRot);
        invRot.rotateVect(position);

        core::vector3df rotation = invRot.getRotationDegrees();
        rotation = core::vector3df(0, 0, 0);
        position = - position;
        irr::core::vector3df scale = matr.getScale();

        if (joint)
        {
            //Build GlobalMatrix:
            core::matrix4 positionMatrix;
            positionMatrix.setTranslation( position );
            core::matrix4 scaleMatrix;
            scaleMatrix.setScale( scale );
            core::matrix4 rotationMatrix;
            rotationMatrix.setRotationDegrees(rotation);

            joint->GlobalMatrix = positionMatrix * rotationMatrix * scaleMatrix;
            // The local matrix will be computed in make_localMatrix_from_global
            joint->LocalMatrix = positionMatrix * rotationMatrix * scaleMatrix;
        }
    }*/
std::cout << "enter here" << std::endl;
    // Local matrix need to be re-computed
    for (u32 i = 0; i < mesh->getJointCount(); ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i];
        computeLocal(mesh, joint);

        joint->Animatedposition = joint->LocalMatrix.getTranslation();
        joint->Animatedrotation = joint->LocalMatrix.getRotationDegrees();
        joint->Animatedscale = joint->LocalMatrix.getScale();
    }

    std::cout << "line 197" << std::endl;
    return true;
}

bool CSkeleton::checkIfPerfectlyCorresponding(scene::ISkinnedMesh* mesh)
{
    for (u32 i = 0; i < nbBones; ++i)
    {
        core::stringc bone = names[i];
        std::cout << bone.c_str() << std::endl;

        scene::ISkinnedMesh::SJoint* joint = getJointByName(mesh, bone);
        if (!joint)
            return false;
    }

    return true;
}
