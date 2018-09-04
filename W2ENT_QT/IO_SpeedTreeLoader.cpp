#include "IO_SpeedTreeLoader.h"

#include "Utils_Loaders_Irr.h"
#include <iostream>

using namespace irr;

IO_SpeedTreeLoader::IO_SpeedTreeLoader(io::IFileSystem* fileSystem) : FileSystem(fileSystem)
{

}


void IO_SpeedTreeLoader::readSpeedTreeFile (core::stringc filename)
{
    io::IReadFile* file = FileSystem->createAndOpenFile(filename);
    if (!file)
    {
        std::cout << "fail to open speedtree file : " << filename.c_str() << std::endl;
        return;
    }

    /*
    fileSpeedTree->seek(4); // 1000
    u32 nameSize = readU32(fileSpeedTree);
    core::stringc name = readString(fileSpeedTree, nameSize);
    std::cout << "Name : " << name.c_str() << std::endl;
    fileSpeedTree->seek(8, true); // 2 uint : 1002 2000
    */
    // plot twist : the 28 bytes are  alwayws the same


    /*
    u32 textureNameSize = readU32(fileSpeedTree);
    core::stringc textureName = readString(fileSpeedTree, textureNameSize);
    std::cout << "Texture name : " << textureName.c_str() << std::endl;

    // 1 uint = 2001 + unknown + 2002 + 1 empty byte + 2003 + unknow + 2005 + unkno + 2006 + 16 unknow + 3 + 1016
    fileSpeedTree->seek(49, true);
    u32 nbIterations = readU32(fileSpeedTree);

    // the body :
    for (u32 i = 0; i < nbIterations; ++i)
    {
        fileSpeedTree->seek(8, true);
        SpeedTreeBezierCurve distorsionControl = readSpeedtreeSpline(fileSpeedTree);
        file->seek(4, true);
        SpeedTreeBezierCurve weightControl = readSpeedtreeSpline(fileSpeedTree);
        file->seek(4, true);
        SpeedTreeBezierCurve roughWindResponseControl = readSpeedtreeSpline(fileSpeedTree);
        file->seek(4, true);
        SpeedTreeBezierCurve fineWindControl = readSpeedtreeSpline(fileSpeedTree);
        file->seek(4, true);
        SpeedTreeBezierCurve highLenghtControl = readSpeedtreeSpline(fileSpeedTree);
        file->seek(4, true);
        SpeedTreeBezierCurve distorsionControl = readSpeedtreeSpline(fileSpeedTree);
        file->seek(4, true);
        SpeedTreeBezierCurve distorsionControl = readSpeedtreeSpline(fileSpeedTree);
        file->seek(4, true);
        SpeedTreeBezierCurve distorsionControl = readSpeedtreeSpline(fileSpeedTree);
        file->seek(4, true);
        SpeedTreeBezierCurve distorsionControl = readSpeedtreeSpline(fileSpeedTree);
        file->seek(4, true);
    }
    */
    video::ITexture* trunkBranchesTexture = nullptr;
    while (file->getPos() < file->getSize())
    {

        u32 chunkPartId = readU32(file);
        switch(chunkPartId)
        {
            case 1000:
            {
                u32 nameSize = readU32(file);
                core::stringc name = readString(file, nameSize);
                std::cout << "Header version name : " << name.c_str() << std::endl;
            }
            case 2000: // trunk and branches texture map
            {
                u32 textureNameSize = readU32(file);
                core::stringc textureName = readString(file, textureNameSize);
                std::cout << "Texture name : " << textureName.c_str() << std::endl;
                //trunkBranchesTexture = getTexture(textureName);
            }
            case 2001:
            {
                f32 lodFarDistance = readF32(file);
            }
            case 2002:
            {
                file->seek(1, true);
            }
            case 2003:
            {
                f32 lodNearDistance = readF32(file);
            }
            case 2005:
            {
                file->seek(4, true);
            }
            case 2006:
            {
                f32 size = readF32(file);
            }
            case 2007:
            {
                f32 sizeVariance = readF32(file);
            }
            case 1014:
            {

            }
        }
    }
}

SpeedTreeBezierCurve IO_SpeedTreeLoader::readSpeedtreeSpline(io::IReadFile* file)
{
    SpeedTreeBezierCurve curve;

    u32 chunckSize = readU32(file);
    bool isInChunckBody = false;
    core::stringc chunckBody;
    // read the body
    for (u32 i = 0; i < chunckSize; ++i)
    {
        char c = readS8(file);
        if (c == '}')
            isInChunckBody = false;

        if (isInChunckBody)
            chunckBody.append(c);
        else if (c == '{')
            isInChunckBody = true;
    }
    core::array<core::stringc> tokens;
    chunckBody.split(tokens, " \n\t");
    // debug
    for (u32 i = 0; i < tokens.size(); ++i)
    {
        std::cout << "token " << i << " : " << tokens[i].c_str() << std::endl;
    }

    int nbPoints = std::atoi(tokens[0].c_str());
    for (int i = 0; i < nbPoints; ++i)
    {
        u32 pointId = std::atoi(tokens[1 + i*4 + 0].c_str());
        f32 x = std::atof(tokens[1 + i*4 + 1].c_str());
        f32 y = std::atof(tokens[1 + i*4 + 2].c_str());
        f32 z = std::atof(tokens[1 + i*4 + 3].c_str());

        SpeedTreeBezierPoint pt;
        pt.pointId = pointId;
        pt.point = core::vector3df(x, y, z);
        curve.push_back(pt);
    }

    return curve;
}

