#include "Utils_Loaders_Irr.h"
#include <iostream>

bool readBool(io::IReadFile* file)
{
    u8 valChar = readU8(file);
    return (valChar > 0);
}

core::stringc readString(io::IReadFile* file, s32 nbChars)
{
    char returnedString[nbChars + 1];
    file->read(returnedString, nbChars);
    returnedString[nbChars] = '\0';
    return returnedString;
}

core::stringc readStringUntilNull(io::IReadFile* file)
{
    core::stringc returnedString;
    char c;
    while (1)
    {
       file->read(&c, 1);
       if (c == 0x00)
           break;
       returnedString.append(c);
    }

    return returnedString;
}

core::stringc readStringFixedSize(io::IReadFile* file, int nbChars)
{
    long back = file->getPos();
    core::stringc returnedString = readString(file, nbChars);
    file->seek(back + nbChars);

    return returnedString;
}


void chechNaNErrors(core::vector3df& vector3)
{
    if (std::isnan(vector3.X) || std::isinf(vector3.X))
        vector3.X = 0.f;

    if (std::isnan(vector3.Y) || std::isinf(vector3.Y))
        vector3.Y = 0.f;

    if (std::isnan(vector3.Z) || std::isinf(vector3.Z))
        vector3.Z = 0.f;
}


// JointHelper functions
bool JointHelper::HasJoint(const scene::ISkinnedMesh* mesh, const core::stringc jointName)
{
    return mesh->getJointNumber(jointName.c_str()) != -1;
}

scene::ISkinnedMesh::SJoint* JointHelper::GetJointByName(const scene::ISkinnedMesh* mesh, const core::stringc jointName)
{
    s32 number = mesh->getJointNumber(jointName.c_str());
    if (number != -1)
    {
        return mesh->getAllJoints()[number];
    }
    else
    {
        return nullptr;
    }
}

scene::ISkinnedMesh::SJoint* JointHelper::GetParent(const scene::ISkinnedMesh* mesh, const scene::ISkinnedMesh::SJoint* joint)
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

    return nullptr;
}

void JointHelper::SetParent(const scene::ISkinnedMesh* mesh, scene::ISkinnedMesh::SJoint* joint, scene::ISkinnedMesh::SJoint* parent)
{
    // First, search if we already have a parent and remove it
    core::array<scene::ISkinnedMesh::SJoint*> allJoints = mesh->getAllJoints();
    for (u32 j = 0; j < allJoints.size(); j++)
    {
       scene::ISkinnedMesh::SJoint* parentJoint = allJoints[j];
       for (u32 k = 0; k < parentJoint->Children.size(); k++)
       {
           if (joint == parentJoint->Children[k])
                parentJoint->Children.erase(k);
       }
    }

    // And add to the parent
    parent->Children.push_back(joint);
}

void JointHelper::ComputeGlobalMatrixRecursive(const scene::ISkinnedMesh* mesh, scene::ISkinnedMesh::SJoint* joint)
{
    scene::ISkinnedMesh::SJoint* parent = GetParent(mesh, joint);
    if (parent)
        joint->GlobalMatrix = parent->GlobalMatrix * joint->LocalMatrix;
    else
        joint->GlobalMatrix = joint->LocalMatrix;

    for (u32 i = 0; i < joint->Children.size(); ++i)
    {
        ComputeGlobalMatrixRecursive(mesh, joint->Children[i]);
    }
}

core::array<scene::ISkinnedMesh::SJoint*> JointHelper::GetRoots(const scene::ISkinnedMesh* mesh)
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

void debugJointRecursive(scene::ISkinnedMesh::SJoint* joint, int depth)
{
    for (int i = 0; i < depth; ++i)
        std::cout << "-";
    std::cout << "> " << joint->Name.c_str() << std::endl;

    for (u32 i = 0; i < joint->Children.size(); ++i)
    {
        debugJointRecursive(joint->Children[i], depth+1);
    }
}

void JointHelper::DebugJointsHierarchy(const scene::ISkinnedMesh* mesh)
{
    core::array<scene::ISkinnedMesh::SJoint*> roots = GetRoots(mesh);
    for (u32 i = 0; i < roots.size(); ++i)
    {
        debugJointRecursive(roots[i], 0);
    }
}

