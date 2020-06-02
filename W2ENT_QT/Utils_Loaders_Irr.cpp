#include "Utils_Loaders_Irr.h"

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


// JointHelper functions
bool JointHelper::HasJoint(scene::ISkinnedMesh* mesh, core::stringc jointName)
{
    return mesh->getJointNumber(jointName.c_str()) != -1;
}

scene::ISkinnedMesh::SJoint* JointHelper::GetJointByName(scene::ISkinnedMesh* mesh, core::stringc jointName)
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
