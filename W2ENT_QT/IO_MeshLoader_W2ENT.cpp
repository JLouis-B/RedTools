// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#define _IRR_COMPILE_WITH_W2ENT_LOADER_
#ifdef _IRR_COMPILE_WITH_W2ENT_LOADER_

#include "IO_MeshLoader_W2ENT.h"

#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "IReadFile.h"
#include "IWriteFile.h"

#include "Utils_RedEngine.h"
#include "Utils_Qt_Irr.h"
#include "Utils_Halffloat.h"
#include "Utils_Loaders_Irr.h"

#include "Settings.h"

#include <vector>
#include <sstream>
#include "IrrAssimp/IrrAssimpImport.h" // SkinnedVertex struct


namespace irr
{
namespace scene
{

//! Constructor
IO_MeshLoader_W2ENT::IO_MeshLoader_W2ENT(scene::ISceneManager* smgr, io::IFileSystem* fs)
: SceneManager(smgr),
  FileSystem(fs),
  AnimatedMesh(nullptr),
  log(nullptr)
{
	#ifdef _DEBUG
	setDebugName("CW2ENTMeshFileLoader");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool IO_MeshLoader_W2ENT::isALoadableFileExtension(const io::path& filename) const
{
    io::IReadFile* file = SceneManager->getFileSystem()->createAndOpenFile(filename);
    if (!file)
        return false;

    bool checkIsLoadable = (getRedEngineFileType(file) == REV_WITCHER_2);

    file->drop();
    return checkIsLoadable;
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* IO_MeshLoader_W2ENT::createMesh(io::IReadFile* f)
{
	if (!f)
        return nullptr;

    log = Log::Instance();

    #ifdef _IRR_WCHAR_FILESYSTEM
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsStringW("TW_GAME_PATH");
    #else
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsString("TW_GAME_PATH");
    #endif

    // log
    log->addLine("");
    log->addLine(formatString("-> File : %s", f->getFileName().c_str()));

    log->add("_________________________________________________________\n\n\n");

    log->addLine("Start loading");
    log->flush();

    AnimatedMesh = SceneManager->createSkinnedMesh();

	if (load(f))
	{
		AnimatedMesh->finalize();
	}
	else
	{
		AnimatedMesh->drop();
        AnimatedMesh = nullptr;
    }

	//Clear up
    Files.clear();
    Strings.clear();
    Materials.clear();
    BonesOffsetMatrix.clear();

    if (!m_skeletonsLoaderMode)
        Skeletons.clear();

	return AnimatedMesh;
}

void IO_MeshLoader_W2ENT::addVectorToLog(core::stringc name, core::vector3df vec)
{
    log->addLineAndFlush(formatString("Vector %s : %f %f %f", name.c_str(), vec.X, vec.Y, vec.Z));
}

void IO_MeshLoader_W2ENT::addMatrixToLog(core::matrix4 mat)
{
    core::stringc logContent = "Matrix4 : \n";
    for (u32 i = 0; i < 16; ++i)
    {
        logContent += mat[i];
        if (i % 4 == 3)
            logContent += "\n";
        else
            logContent += "  ";
    }
    logContent += "\n";
    log->addAndFlush(logContent);

    addVectorToLog("mat translation", mat.getTranslation());
    addVectorToLog("mat euler", mat.getRotationDegrees());

    logContent = "End matrix4\n\n";

    log->addAndFlush(logContent);
}

bool IO_MeshLoader_W2ENT::load(io::IReadFile* file)
{
    long back = file->getPos();
    file->seek(4, true); // "CR2W" magic string

    RedEngineFileHeader header;
    loadTW2FileHeader(file, header, false);
    Version = header.Version;
    Strings = header.Strings;
    Files = header.Files;

    log->addLineAndFlush("Strings and filenames OK");

    core::array<s32> headerData = readDataArray<s32>(file, 10);

    //std::cout << std::endl << "Liste 3 (de taille " << data[9] << ") : " << std::endl;

    /*file->seek(back+data[8]);
    for (int i = 0; i < data[9]; i++)
    {
        int back2 = file->getPos();
        std::vector<unsigned char> data1 = readUnsignedChars(file, 2);
        file->seek(back2);
        if (data1[1] == 1)
        {
            std::cout << " -> (data[1] = 1) "<< split(1, readWord(file, readUnsignedChars(file, 1)[0]-127)) << std::endl;
        }
        else
        {
            std::cout << " -> "<< readWord(file, readUnsignedChars(file, 1)[0]-128) << std::endl;
        }
        // le dernier ne s'affiche pas
    }*/
    // useless for the loader

    core::array<ChunkDescriptor> meshesToLoad;

    // Ok, now we can read the materials and meshes
    file->seek(back + headerData[4]);
    for(int i = 0; i < headerData[5]; ++i) // Now, the list of all the components of the files
    {
        // Read data
        const u16 dataTypeId = readU16(file) - 1;
        core::stringc dataTypeName = Strings[dataTypeId];
        log->addLineAndFlush(formatString("[%d] dataTypeName=%s", i, dataTypeName.c_str()));

        core::array<s32> data2 = readDataArray<s32>(file, 5);             // Data info (adress...)

        ChunkDescriptor chunkInfos;
        chunkInfos.size = data2[1];
        chunkInfos.adress = data2[2];

        core::stringc meshSource;

        if (data2[0] == 0)
        {
            const u8 size = readU8(file) - 128;
            const u8 offset = readU8(file);
            if (offset != 1)
                file->seek(-1, true);

            meshSource = readString(file, size);
        }
        else
        {
            // Seem to be always 0
            const u8 unk = readU8(file) - 128;
            //log->addLineAndFlush(formatString("Unk is %d", unk));
        }

        // now all stuff readed
        const int back2 = file->getPos();

        if (dataTypeName == "CMaterialInstance")
        {
            log->addAndFlush("\nCMaterialInstance\n");

            CMaterialInstance(file, chunkInfos, i+1);

            log->addLineAndFlush("CMaterialInstance OK");
        }
        else if (dataTypeName == "CMesh")
        {
            //std::cout << "CMesh" << std::endl;            
            log->addAndFlush("\nCMesh\n");

            // Just get the filename from the filepath
            int index = meshSource.findLast('\\');
            core::stringc meshName = meshSource.subString(index, meshSource.size() - index);

            // if there is no filepath ? we set a default name
            if(meshName.size() == 0)
                meshName = "model";

            meshesToLoad.push_back(chunkInfos);

            log->addLineAndFlush("CMesh OK");
        }
        else if (dataTypeName == "CSkeleton")
        {
            Skeletons.push_back(CSkeleton(file, chunkInfos));
        }
        else if (dataTypeName == "CSkeletalAnimation")
        {
            // TODO
        }
        else
        {
#ifdef IS_A_DEVELOPMENT_BUILD
            //CUnknown(file, chunkInfos);
#endif
        }

        file->seek(back2);
    }
    log->addLineAndFlush("Textures and mesh data OK");

    // When we have loaded all the materials and all the 'mesh headers', we load the meshes
    for (u32 i = 0; i < meshesToLoad.size(); ++i)
    {
        // Create mesh
        CMesh(file, meshesToLoad[i]);
    }

    // some skeletons aren't in CSkeleton
    for (u32 i = 0; i < Files.size(); ++i)
    {
        core::stringc filename = Files[i];
        if (core::hasFileExtension(filename, "w2rig"))
        {
            io::IReadFile* skeletonFile = FileSystem->createAndOpenFile(ConfigGamePath + filename);
            IO_MeshLoader_W2ENT loader(SceneManager, FileSystem);
            loader.m_skeletonsLoaderMode = true;
            loader.createMesh(skeletonFile);

            for (u32 i = 0; i < loader.Skeletons.size(); ++i)
            {
                log->addLine("Add a skeleton loaded from external file");
                Skeletons.push_back(loader.Skeletons[i]);
            }

            skeletonFile->drop();
        }
    }

    if (!m_skeletonsLoaderMode)
    {

        for (u32 i = 0; i < Skeletons.size(); ++i)
        {
            createCSkeleton(Skeletons[i]);
        }

        core::array<scene::ISkinnedMesh::SJoint*> roots = JointHelper::GetRoots(AnimatedMesh);
        for (u32 i = 0; i < roots.size(); ++i)
        {
            //std::cout << "root : " << roots[i]->Name.c_str() << std::endl;
            JointHelper::ComputeGlobalMatrixRecursive(AnimatedMesh, roots[i]);
        }

        SkinMesh();

        //JointHelper::DebugJointsHierarchy(AnimatedMesh);
    }
    log->addLineAndFlush("All is loaded");
	return true;
}

void IO_MeshLoader_W2ENT::CUnknown(io::IReadFile* file, ChunkDescriptor infos)
{
    const long back = file->getPos();
    file->seek(infos.adress);

    while(1)
    {
        PropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
            break;


        log->addLineAndFlush(propHeader.toString());

        // CAnimatedAttachment
        if (propHeader.propName == "boneMapping" && propHeader.propType == "@SBoneMapping")
        {
            u32 bonesCount = readU32(file);
            std::cout << "@SBoneMapping bonesCount = " << bonesCount << std::endl;
            file->seek(6, true);
            for (u32 i = 0; i < bonesCount; ++i)
            {
                while (1)
                {
                    PropertyHeader boneMappingProp;
                    if (!ReadPropertyHeader(file, boneMappingProp))
                        break;

                    std::cout << "---> " << boneMappingProp.toString().c_str() << std::endl;
                    u32 value = readU32(file);
                    std::cout << "Value: " << value << std::endl;
                }
                file->seek(-2, true);
                std::cout << std::endl;
            }
        }

        /* CMeshSkinningAttachment
         * Link the bone of a CMesh with the corresponding bones of a CSkeleton
         * For each bone of the following CMesh, we have the ID of the corresponding bone in the previous CSkeleton
         *
         * Basically the structure of a model is the following :
         * CSekeleton
         * CMeshSkinningAttachment 1
         * CMeshComponent 1
         * CMesh 1
         * CMeshSkinningAttachment 2
         * CMeshComponent 2
         * CMesh 2
         * ...
         *
         * In fact, we can simply check by looking the name of the bones
         */
        if (propHeader.propName == "boneMapping" && propHeader.propType == "@Int")
        {
            u32 bonesCount = readU32(file);
            //std::cout << "bonesCount = " << bonesCount << std::endl;
            file->seek(4, true);
            for (u32 i = 0; i < bonesCount; ++i)
            {
                s32 value = readS32(file);
                std::cout << "Value: " << value << std::endl;
            }
        }

        if (propHeader.propType == "*CSkeleton")
        {
            u32 value = readU32(file);
            std::cout << "*CSkeleton value: " << value << std::endl;
        }

        file->seek(propHeader.endPos);
    }

    file->seek(back);
}

bool seemToBeAnASCIICharacter(char c)
{
    return c != 0 && c != 1;
}

TW2_CSkeleton IO_MeshLoader_W2ENT::CSkeleton(io::IReadFile* file, ChunkDescriptor infos)
{
    TW2_CSkeleton skeleton;

    file->seek(infos.adress);

    //std::cout << "begin at " << infos.adress << " and end at " << infos.adress + infos.size << ", so size = " << infos.size << std::endl;
    while(1)
    {
        PropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
            break;


        log->addLineAndFlush(propHeader.toString());
        file->seek(propHeader.endPos);
    }
    file->seek(-4, true);

    log->addLineAndFlush(formatString("CSKeleton start at %d", file->getPos()));
    file->seek(3, true);
    u32 chunkSize = readU32(file);
    file->seek(36, true); // unk
    file->seek(28, true); // app info : version (4 bytes) + 24 bytes string
    file->seek(116, true); // "__classname__", "__type__"...

    u32 unk = readU32(file);
    u32 endOfBonesNamesAdress = readU32(file);
    u32 endOfBonesUnk1Adress = readU32(file);
    u32 endOfBonesUnk2Adress = readU32(file);
    u32 dataSize = readU32(file);
    file->seek(8, true); // 2x dataSize
    log->addLineAndFlush(formatString("unk = %d, endOfBonesNamesAdress = %d, endOfBonesUnk1Adress = %d, endOfBonesUnk2Adress = %d, dataSize = %d", unk, endOfBonesNamesAdress, endOfBonesUnk1Adress, endOfBonesUnk2Adress, dataSize));

    file->seek(112, true);

    //  Data chunk start
    const long dataStartAdress = file->getPos();
    log->addLineAndFlush(formatString("dataStartAdress = %d", dataStartAdress));
    file->seek(8, true);
    u32 nbBones = readU32(file);
    skeleton.setBonesCount(nbBones);
    log->addLineAndFlush(formatString("%d bones at %d", nbBones, file->getPos()-4));
    file->seek(20, true); // 3x bones count

    file->seek(16, true);

    core::stringc rootName = readStringFixedSize(file, 16);
    log->addLineAndFlush(formatString("Root = %s", rootName.c_str()));

    long bonesParentIdChunkAdress = file->getPos();
    //long bonesNameChunkAdress = (dataStartAdress + endOfBonesNamesAdress) - totalNamesSize;
    //long bonesNameChunkAdress = (dataStartAdress + endOfBonesNamesAdress) - (nbBones * 48);


    core::array<u32> boneNameSizes;
    boneNameSizes.set_used(nbBones);
    u32 totalNamesSize = 0;

    // Search bone names size
    file->seek(dataStartAdress + endOfBonesNamesAdress-1);
    for (u32 i = 0; i < nbBones; ++i)
    {
        bool isInText = false;
        u32 textSize = 0;
        while (1)
        {
            char c = readS8(file);
            file->seek(-2, true);

            if (!isInText && seemToBeAnASCIICharacter(c))
            {
                isInText = true;
            }
            if (isInText && !seemToBeAnASCIICharacter(c))
            {
                file->seek(1, true);
                boneNameSizes[nbBones-(i+1)] = textSize;
                totalNamesSize += textSize;
                //log->addLineAndFlush(formatString("Text size is : %d", textSize));
                break;
            }
            textSize++;
        }
    }
    long bonesNameChunkAdress = (dataStartAdress + endOfBonesNamesAdress) - totalNamesSize;


    // really a piece of crap but couldn't find a way to get the info any other way yet
    u32 offset = 0;
    file->seek(bonesNameChunkAdress - 8);
    while (1)
    {
        f32 f = readF32(file);
        if (f > 0.09f && f < 10.1f)
        {
            break;
        }
        file->seek(-8, true);
        offset += 4;
    }
    long bonesTransformChunkAdress = bonesNameChunkAdress - (offset + nbBones * 48);



    file->seek(bonesNameChunkAdress);
    for (u32 i = 0; i < nbBones; ++i)
    {
        core::stringc boneName = readStringFixedSize(file, boneNameSizes[i]);
        skeleton.names.push_back(boneName);
        log->addLineAndFlush(formatString("Bone %s (@%d)", boneName.c_str(), file->getPos() - boneNameSizes[i]));
    }

    file->seek(bonesParentIdChunkAdress);
    for (u32 i = 0; i < nbBones; ++i)
    {
        const s16 parentId = readS16(file); // -1 if root
        skeleton.parentId.push_back(parentId);
        log->addLineAndFlush(formatString("Parent %d", parentId));
    }

    log->addLineAndFlush(formatString("needed size is %d", nbBones * 48));
    log->addLineAndFlush(formatString("Transform: %d", bonesTransformChunkAdress));
    file->seek(bonesTransformChunkAdress);
    for (u32 i = 0; i < nbBones; ++i)
    {
        // position (vector 4) + quaternion (4 float) + scale (vector 4)
        core::vector3df position;
        position.X = readF32(file);
        position.Y = readF32(file);
        position.Z = readF32(file);
        readF32(file); // the w component
        //addVectorToLog("position", position);

        core::quaternion orientation;
        orientation.X = readF32(file);
        orientation.Y = readF32(file);
        orientation.Z = readF32(file);
        orientation.W = readF32(file);
        //log->addLineAndFlush(formatString("Orientation : %f, %f, %f, %f", orientation.X, orientation.Y, orientation.Z, orientation.W));

        core::vector3df scale;
        scale.X = readF32(file);
        scale.Y = readF32(file);
        scale.Z = readF32(file);
        readF32(file); // the w component
        //addVectorToLog("scale", scale);

        core::matrix4 posMat;
        posMat.setTranslation(position);

        core::matrix4 rotMat;
        core::vector3df euler;
        orientation.toEuler(euler);
        //std::cout << "Position = " << position.X << ", " << position.Y << ", " << position.Z << std::endl;
        //std::cout << "Rotation (radians) = " << euler.X << ", " << euler.Y << ", " << euler.Z << std::endl;
        chechNaNErrors(euler);

        rotMat.setRotationRadians(euler);

        core::matrix4 scaleMat;
        scaleMat.setScale(scale);

        core::matrix4 localTransform = posMat * rotMat * scaleMat;
        orientation.makeInverse();


        skeleton.matrix.push_back(localTransform);
        skeleton.positions.push_back(position);
        skeleton.rotations.push_back(orientation);
        skeleton.scales.push_back(scale);
    }

    return skeleton;
}

void IO_MeshLoader_W2ENT::createCSkeleton(TW2_CSkeleton skeleton)
{
    const u32 nbBones = skeleton.getBonesCount();

    core::array<scene::ISkinnedMesh::SJoint*> bones;
    for (u32 i = 0; i < nbBones; ++i)
    {
        const core::stringc boneName = skeleton.names[i];
        scene::ISkinnedMesh::SJoint* bone = JointHelper::GetJointByName(AnimatedMesh, boneName);

        if (!bone)
        {
            log->addLineAndFlush(formatString("Bone %s doesn't exist", boneName.c_str()));
            bone = AnimatedMesh->addJoint();
            bone->Name = boneName;
        }
        else
        {
            log->addLineAndFlush(formatString("Bone %s exist", boneName.c_str()));
        }
        bones.push_back(bone);
    }


    for (u32 i = 0; i < nbBones; ++i)
    {
        const s16 parentId = skeleton.parentId[i];
        scene::ISkinnedMesh::SJoint* joint = bones[i];
        if (!joint)
        {
            continue;
        }

        if (parentId >= 0)
        {
            scene::ISkinnedMesh::SJoint* parent = bones[parentId];
            if (!parent)
            {
                log->addLineAndFlush(formatString("Parent %d doesn't exist", parentId));
                continue;
            }
            else
            {
                JointHelper::SetParent(AnimatedMesh, joint, parent);
                log->addLineAndFlush(formatString("%s -> %s", parent->Name.c_str(), joint->Name.c_str()));
            }
        }
        else
        {
            if (parentId == -1)
                log->addLineAndFlush(formatString("%s is root", joint->Name.c_str()));
            else
                log->addLineAndFlush(formatString("Invalid parent ID: %d", parentId));
        }
    }



    for (u32 i = 0; i < nbBones; ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = bones[i];
        if (!joint)
        {
            log->addLineAndFlush("Transform : joint dont exist!");
            continue;
        }
        else
        {
            joint->LocalMatrix = skeleton.matrix[i];

            joint->Animatedposition = skeleton.positions[i];
            joint->Animatedrotation = skeleton.rotations[i];
            joint->Animatedscale = skeleton.scales[i];
        }
    }
}

void IO_MeshLoader_W2ENT::SkinMesh()
{
    // prepare the skinning array
    std::vector<std::vector<SkinnedVertex> > skinnedVertex;
    skinnedVertex.resize(AnimatedMesh->getMeshBufferCount());
    for (u32 i = 0; i < AnimatedMesh->getMeshBufferCount(); ++i)
    {
        const scene::IMeshBuffer* buffer = AnimatedMesh->getMeshBuffer(i);
        skinnedVertex[i].resize(buffer->getVertexCount());
    }


    // Skin
    for (u32 i = 0; i < AnimatedMesh->getJointCount(); ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = AnimatedMesh->getAllJoints()[i];

        core::matrix4 jointOffset = BonesOffsetMatrix[joint];

        const core::matrix4 boneMat = joint->GlobalMatrix * jointOffset; //* InverseRootNodeWorldTransform;

        for (u32 j = 0; j < joint->Weights.size(); ++j)
        {
            const scene::ISkinnedMesh::SWeight weight = joint->Weights[j];
            const u16 bufferId = weight.buffer_id;
            const u32 vertexId = weight.vertex_id;

            core::vector3df sourcePos = AnimatedMesh->getMeshBuffer(bufferId)->getPosition(vertexId);
            core::vector3df sourceNorm = AnimatedMesh->getMeshBuffer(bufferId)->getNormal(vertexId);
            core::vector3df destPos, destNormal;
            boneMat.transformVect(destPos, sourcePos);
            boneMat.rotateVect(destNormal, sourceNorm);

            skinnedVertex[bufferId][vertexId].Moved = true;
            skinnedVertex[bufferId][vertexId].Position += destPos * weight.strength;
            skinnedVertex[bufferId][vertexId].Normal += destNormal * weight.strength;
        }
    }

    // And apply on the mesh
    for (u32 i = 0; i < AnimatedMesh->getMeshBufferCount(); ++i)
    {
        scene::IMeshBuffer* buffer = AnimatedMesh->getMeshBuffer(i);
        for (u32 j = 0; j < buffer->getVertexCount(); ++j)
        {
            if (skinnedVertex[i][j].Moved)
            {
                buffer->getPosition(j) = skinnedVertex[i][j].Position;
                buffer->getNormal(j) = skinnedVertex[i][j].Normal;
            }
        }
    }
}

void IO_MeshLoader_W2ENT::CMesh(io::IReadFile* file, ChunkDescriptor infos)
{
    log->addLineAndFlush("Load a mesh...");

    // we go to the adress of the data
    file->seek(infos.adress);

    core::array<u32> materialIds;

    // Read all the properties of the mesh
    while(1)
    {
        PropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
            break;
        log->addLineAndFlush(propHeader.toString());

        if (propHeader.propName == "materials" && propHeader.propType == "@*IMaterial")
        {
            u32 nbMaterials = readU32(file);
            file->seek(4, true);
            for (u32 i = 0; i < nbMaterials; ++i)
            {
                u32 matId = readU32(file); // aka the data index of the corresponding CMaterial
                materialIds.push_back(matId);
                //std::cout << "Material: " << value << std::endl;
            }
        }

        file->seek(propHeader.endPos);
    }
    file->seek(-4, true);

    // Read the LODS data ?
    vert_format(file);

    u8 nbBones = readU8(file);
    const bool hasBones = nbBones != 128;

    int back = file->getPos();

    u8 unk = readU8(file);
    if (unk != 1)
        file->seek(-1, true);



    core::array<core::stringc> boneNames;
    if (hasBones)
    {
        boneNames.reallocate(nbBones);
        for (u8 i = 0; i < nbBones; i++)
        {
            core::matrix4 boneMatrix;
            file->read(boneMatrix.pointer(), 4 * 16);

            u16 boneNameId = readU16(file) - 1;
            core::stringc boneName = "";
            if (boneNameId < Strings.size())
            {
                boneName = Strings[boneNameId];
            }
            else
            {
                // Is it still necessary ?
                log->addLineAndFlush(formatString("Wrong bone ID : %d (max=%d", boneNameId, Strings.size()-1));
                boneName = "bone-";
                boneName += i;
            }

            bool boneAlreadyCreated = JointHelper::HasJoint(AnimatedMesh, boneName);
            if (!boneAlreadyCreated)
            {
                scene::ISkinnedMesh::SJoint* joint = AnimatedMesh->addJoint();
                joint->Name = boneName;


                core::vector3df position = boneMatrix.getTranslation();
                core::matrix4 invBoneMatrix;
                boneMatrix.getInverse(invBoneMatrix);

                core::vector3df rotation = invBoneMatrix.getRotationDegrees();
                position = -position;
                core::vector3df scale = boneMatrix.getScale();


                //Build GlobalMatrix:
                core::matrix4 positionMatrix;
                positionMatrix.setTranslation(position);
                core::matrix4 rotationMatrix;
                rotationMatrix.setRotationDegrees(rotation);
                core::matrix4 scaleMatrix;
                scaleMatrix.setScale(scale);

                joint->GlobalMatrix = scaleMatrix * rotationMatrix * positionMatrix;
                joint->LocalMatrix = joint->GlobalMatrix;

                joint->Animatedposition = joint->LocalMatrix.getTranslation();
                joint->Animatedrotation = core::quaternion(joint->LocalMatrix.getRotationDegrees()).makeInverse();
                joint->Animatedscale = joint->LocalMatrix.getScale();

                BonesOffsetMatrix.insert(std::make_pair(joint, boneMatrix));
            }

            boneNames.push_back(boneName);
            log->addLineAndFlush(formatString("Mesh BONENAME : %s", boneName.c_str()));

            file->seek(4, true); //float data12 = readFloats(file, 1)[0];
        }
    }
    nbBones = readU8(file);
    back = file->getPos();

    if (hasBones)
    {
        if (readS32(file) > 128 || readS32(file) > 128)
        {
            if (nbBones != 1)
                file->seek(back + 1);
            else
                file->seek(back);
        }
        else
            file->seek(back);

        readDataArray<s32>(file, nbBones);
    }

    loadSubmeshes(file, materialIds, boneNames);

    log->addLineAndFlush("Mesh loaded");
}

SubmeshData readSubmeshData(io::IReadFile* file)
{
    SubmeshData submesh;
    submesh.vertexType = readU8(file);

    submesh.verticesStart = readU32(file);
    submesh.indicesStart = readU32(file);
    submesh.verticesCount = readU32(file);
    submesh.indicesCount = readU32(file);

    s8 bonesCount = readS8(file);
    if (bonesCount > 0)
        submesh.bonesId = readDataArray<u16>(file, bonesCount);

    submesh.materialId = readU32(file);

    return submesh;
}

void IO_MeshLoader_W2ENT::loadSubmeshes(io::IReadFile* file, core::array<u32> materialIds, core::array<core::stringc> boneNames)
{
    log->addLineAndFlush("loadSubmeshes");

    // We read submeshes infos
    long back = file->getPos();
    int subMeshesInfosOffset = readS32(file);
    file->seek(back + subMeshesInfosOffset);

    file->seek(8, true);
    u32 meshIndicesOffset = readU32(file);
    file->seek(12, true);
    u8 nbSubMesh = readU8(file);

    core::array<SubmeshData> subMeshesData;
    for (u8 i = 0; i < nbSubMesh; i++)
    {
        SubmeshData submesh = readSubmeshData(file);
        subMeshesData.push_back(submesh);

        log->addLineAndFlush(formatString("submesh : vertEnd = %d, vertype = %d", submesh.verticesCount, submesh.vertexType));
    }

    for (u8 i = 0; i < nbSubMesh; i++)
    {
        if (i >= IdLOD[0][0]) // Load only the first LOD ?
            continue;

        file->seek(back + 4);
        loadSubmesh(file, subMeshesData[i], meshIndicesOffset, materialIds, boneNames);
    }

    log->addLineAndFlush("loadSubmeshes OK");
}

void IO_MeshLoader_W2ENT::loadSubmesh(io::IReadFile* file, SubmeshData submesh, u32 meshIndicesOffset, core::array<u32> materialIds, core::array<core::stringc> boneNames)
{
    const long submeshStartPos = file->getPos();
    const video::SColor defaultColor(255, 255, 255, 255);

    u8 vertexSize = 0;
    bool hasSecondUVLayer = false;
    bool isSkinned = false;
    switch (submesh.vertexType)
    {
        case 0: // POS
            vertexSize = 36;
            break;

        case 6: // POS + 2 UV
            vertexSize = 44;
            hasSecondUVLayer = true;
            break;

        case 9: // POS + ?
        case 5: // POS + ?
            vertexSize = 60;
            break;

        case 1: // POS + SKIN
        case 11:// POS + SKIN ?
            vertexSize = 44;
            isSkinned = true;
            break;

        case 7: // POS + SKIN + 2 UV
            vertexSize = 52;
            isSkinned = true;
            hasSecondUVLayer = true;
            break;

        default: // ?
            vertexSize = 52;
            isSkinned = true;
            log->addLineAndFlush(formatString("Unknown vertexType: %d", submesh.vertexType));
            break;
    }

    log->addLineAndFlush(formatString("submesh (vertype: %d, vertsize: %d, vertStart = %d)", submesh.vertexType, vertexSize, file->getPos()));
    file->seek(submeshStartPos + submesh.verticesStart * vertexSize);

    SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
    if (hasSecondUVLayer)
    {
        buffer->VertexType = video::EVT_2TCOORDS;
        buffer->Vertices_2TCoords.reallocate(submesh.verticesCount);
    }
    else
    {
        buffer->VertexType = video::EVT_STANDARD;
        buffer->Vertices_Standard.reallocate(submesh.verticesCount);
    }

    // Vertices
    for (u32 i = 0; i < submesh.verticesCount; ++i)
    {
        long vertexAdress = file->getPos();
        core::array<f32> position = readDataArray<f32>(file, 3);

        // Weights
        if (isSkinned)
        {
            core::array<u8> weightsData = readDataArray<u8>(file, 8);
            for (int vertexWeightsId = 0; vertexWeightsId < 4; vertexWeightsId++)
            {
                u8 strength = weightsData[vertexWeightsId + 4];
                if (strength != 0)
                {
                    u16 boneId = submesh.bonesId[weightsData[vertexWeightsId]];
                    core::stringc boneName = boneNames[boneId];

                    ISkinnedMesh::SJoint* bone = JointHelper::GetJointByName(AnimatedMesh, boneName);
                    if (bone)
                    {
                        ISkinnedMesh::SWeight* wt = AnimatedMesh->addWeight(bone);
                        wt->strength = ((f32)strength) / 255.0f;
                        wt->buffer_id = AnimatedMesh->getMeshBufferCount() - 1;
                        wt->vertex_id = i;

                        if (wt->strength > 1.0f || wt->strength < 0.0f)
                            log->addLineAndFlush(formatString("Error, strength range : %f", wt->strength));
                    }
                }
            }
        }

        // Normals. + bi-normal/tangent ?
        core::array<u8> bytes = readDataArray<u8>(file, 8);
        f32 nx = ((s16)bytes[0] - 127) / 127.f;
        f32 ny = ((s16)bytes[1] - 127) / 127.f;
        f32 nz = ((s16)bytes[2] - 127) / 127.f;

        core::array<f32> uv = readDataArray<f32>(file, 2);
        if (hasSecondUVLayer)
        {
            core::array<f32> uv2 = readDataArray<f32>(file, 2);

            video::S3DVertex2TCoords vertex;
            vertex.Pos = core::vector3df(position[0], position[1], position[2]);
            vertex.TCoords = core::vector2df(uv[0], uv[1]);
            vertex.TCoords2 = core::vector2df(uv2[0], uv2[1]);
            vertex.Color = defaultColor;
            vertex.Normal = core::vector3df(nx, ny, nz);
            buffer->Vertices_2TCoords.push_back(vertex);
            //std::cout << "UV2: " << uv2[0] << ", " << uv2[1] << std::endl;
        }
        else
        {
            video::S3DVertex vertex;
            vertex.Pos = core::vector3df(position[0], position[1], position[2]);
            vertex.TCoords = core::vector2df(uv[0], uv[1]);
            vertex.Color = defaultColor;
            vertex.Normal = core::vector3df(nx, ny, nz);
            buffer->Vertices_Standard.push_back(vertex);
        }

        file->seek(vertexAdress + vertexSize);
    }

    // Faces
    file->seek(submeshStartPos + meshIndicesOffset + submesh.indicesStart * 2);
    buffer->Indices.set_used(submesh.indicesCount);
    for (u32 j = 0; j < submesh.indicesCount; ++j)
    {
        const u16 indice = readU16(file);

        // Indice need to be inversed for the normals
        if (j % 3 == 0)
            buffer->Indices[j] = indice;
        else if (j % 3 == 1)
            buffer->Indices[j+1] = indice;
        else if (j % 3 == 2)
            buffer->Indices[j-1] = indice;
    }

    // Material
    u32 materialKey = materialIds[submesh.materialId];
    if (Materials.find(materialKey) != Materials.end())
        buffer->Material = Materials[materialIds[submesh.materialId]];

    buffer->recalculateBoundingBox();
}

video::ITexture* IO_MeshLoader_W2ENT::getTexture(core::stringc textureFilepath)
{
    video::ITexture* texture = nullptr;

    if (core::hasFileExtension(textureFilepath.c_str(), "xbm"))
    {
        io::path ddsFilepath;
        core::cutFilenameExtension(ddsFilepath, textureFilepath);
        ddsFilepath += ".dds";

        if (FileSystem->existFile(ddsFilepath))
            texture = SceneManager->getVideoDriver()->getTexture(ddsFilepath.c_str());

        if (!texture)
        {
            // Make a DDS file from the XBM file
            generateDDSFromXBM(textureFilepath, ddsFilepath);
            texture = SceneManager->getVideoDriver()->getTexture(ddsFilepath.c_str());
        }
    }
    else
    {
        if (FileSystem->existFile(textureFilepath))
            texture = SceneManager->getVideoDriver()->getTexture(textureFilepath.c_str());
    }

    return texture;
}

bool IO_MeshLoader_W2ENT::ReadPropertyHeader(io::IReadFile* file, PropertyHeader &propHeader)
{
    u16 propName = readU16(file);
    u16 propType = readU16(file);

    if (propName == 0 || propType == 0 || propName > Strings.size() || propType > Strings.size())
        return false;

    propHeader.propName = Strings[propName - 1];
    propHeader.propType = Strings[propType - 1];

    // The difference with TW3
    file->seek(2, true);

    const long back = file->getPos();
    propHeader.propSize = readS32(file);
    propHeader.endPos = back + propHeader.propSize;

    return true;
}

void IO_MeshLoader_W2ENT::CMaterialInstance(io::IReadFile* file, ChunkDescriptor infos, u32 matId)
{
    file->seek(infos.adress);

    log->addLineAndFlush("Read material...");

    video::SMaterial material;
    material.MaterialType = video::EMT_SOLID;

    while (1)
    {
        PropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
            break;

        //std::cout << propHeader.propName.c_str() << std::endl;
        //std::cout << propHeader.propType.c_str() << std::endl;
        if (propHeader.propType == "*IMaterial")
        {
            file->seek(1, true); //FilesTable[255-readU8(file)];
            file->seek(6, true); //readUnsignedChars(file, 6);

            s32 nMatElement = readS32(file);
            log->addLineAndFlush(formatString("nMatElement = %d", nMatElement));

            for (int n = 0; n < nMatElement; n++)
            {
                long propertyStart = file->getPos();
                int propertySize = readS32(file);

                u16 propertyIndex = readU16(file) - 1;
                u16 propertyTypeIndex = readU16(file) - 1;

                core::stringc propertyName = Strings[propertyIndex];
                core::stringc propertyType = Strings[propertyTypeIndex];

                if (propertyType == "*ITexture")
                {
                    int imageID = readU8(file);
                    if (imageID>0)
                    {
                        // ImageID is the index of the texture file in the FilesTable
                        //std::cout << "Image ID : " << imageID << ", image name : " << FilesTable[255-imageID] << std::endl;
                        core::stringc texturePath = ConfigGamePath + Files[255 - imageID];
                        file->seek(3, true); //readUnsignedChars(file, 3);

                        if (propertyName == "diffusemap" || propertyName == "tex_Diffuse" || propertyName == "Diffuse" || propertyName == "sptTexDiffuse")
                        {
                            video::ITexture* tex = getTexture(texturePath);
                            material.setTexture(0, tex);
                        }
                        else if (propertyName == "normalmap" || propertyName == "tex_Normal" || propertyName == "Normal" || propertyName == "sptTexNormal")
                        {
                            material.MaterialType = video::EMT_PARALLAX_MAP_SOLID ;

                            // normal map
                            video::ITexture* tex = getTexture(texturePath);
                            material.setTexture(1, tex);
                        }
                        else if (propertyName == "specular" || propertyName == "tex_Specular" || propertyName == "Specular" || propertyName == "sptTexSpecular")
                        {
                            // not handled by irrlicht
                            video::ITexture* tex = getTexture(texturePath);
                            material.setTexture(2, tex);
                        }
                    }

                }
                file->seek(propertyStart + propertySize);
            }
        }
        file->seek(propHeader.endPos);
    }

    Materials.insert(std::make_pair(matId, material));
    //std::cout << "Texture : " << FilesTable[255-readUnsignedChars(1)[0]] << std::endl;
}

void IO_MeshLoader_W2ENT::generateDDSFromXBM(core::stringc xbmFilepath, core::stringc ddsFilepath)
{
    log->addLineAndFlush("XBM to DDS");

    // Open the XBM file
    io::IReadFile* xbmFile = FileSystem->createAndOpenFile((xbmFilepath).c_str());
    if (!xbmFile)
    {
        SceneManager->getParameters()->setAttribute("TW_FEEDBACK", "Some textures havn't been found, check your 'Base directory'.");
        log->addAndFlush(core::stringc("Error : the file ") + xbmFilepath + core::stringc(" can't be opened.\n"));
        return;
    }

    log->addLineAndFlush("XBM file opened");
    xbmFile->seek(4); // the magic

    core::array<s32> data = readDataArray<s32>(xbmFile, 10);
    xbmFile->seek(data[2]);

    core::array<core::stringc> stringsXBM;
    for (int i = 0; i < data[3]; i++)
        stringsXBM.push_back(readString(xbmFile, readU8(xbmFile)-128));

    log->addLineAndFlush("List ok");


    // data
    xbmFile->seek(data[4]);
    for (int i = 0; i < data[5]; i++)
    {
        const u16 dataTypeId = readU16(xbmFile) - 1;
        const core::stringc dataTypeName = stringsXBM[dataTypeId];
        log->addLineAndFlush(formatString("dataTypeName=%s", dataTypeName.c_str()));

        core::array<s32> data2 = readDataArray<s32>(xbmFile, 5);

        ChunkDescriptor chunkInfos;
        chunkInfos.size = data2[1];
        chunkInfos.adress = data2[2];

        if (data2[0] == 0)
        {
            const u8 size = readU8(xbmFile) - 128;
            const u8 offset = readU8(xbmFile);
            if (offset != 1)
                xbmFile->seek(-1, true);

            const core::stringc meshSource = readString(xbmFile, size);
        }
        else
        {
            // Seem to be always 0
            const u8 unk = readU8(xbmFile) - 128;
            log->addLineAndFlush(formatString("Unk is %d", unk));

            //file->seek(1, true);
        }


        int back3 = xbmFile->getPos();

        // If the data is a CBitmapTexture, we read the data
        if (dataTypeName == "CBitmapTexture")
            XBM_CBitmapTexture(xbmFile, ddsFilepath, chunkInfos, stringsXBM);

        xbmFile->seek(back3);
    }
    xbmFile->drop();

    log->addLineAndFlush("XBM to DDS OK");
}

void IO_MeshLoader_W2ENT::XBM_CBitmapTexture(io::IReadFile* xbmFile, core::stringc filenameDDS, ChunkDescriptor chunk, core::array<core::stringc> XbmStrings)
{
    // int back = file->getPos();
    log->addLineAndFlush("CBitmapTexture");

    xbmFile->seek(chunk.adress);

    const u8 ddsheader[] = "\x44\x44\x53\x20\x7C\x00\x00\x00\x07\x10\x0A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x05\x00\x00\x00\x44\x58\x54\x31\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x10\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

    core::stringc dxt;
    int height1, width1;

    // Read the header infos
    while(1)
    {
        log->addLineAndFlush("Read header data...");

        core::stringc propertyName = XbmStrings[readU16(xbmFile)-1];
        core::stringc propertyType = XbmStrings[readU16(xbmFile)-1];

        xbmFile->seek(2, true);  //readUnsignedChars(fileXBM, 2);
        int back2 = xbmFile->getPos();

        s32 seek = readS32(xbmFile); // size of the property
        // The dimensions of the textures
        if (propertyName == "width" && propertyType == "Uint")
        {
            width1 = readS32(xbmFile);
        }
        else if (propertyName == "height" && propertyType == "Uint")
        {
            height1 = readS32(xbmFile);
        }
        // Compression format
        else if (propertyType == "ETextureCompression")
        {
            dxt = XbmStrings[readU16(xbmFile)-1];

            if  (dxt == "TCM_DXTNoAlpha")
                dxt = "\x44\x58\x54\x31";
            else if (dxt == "TCM_DXTAlpha")
                dxt = "\x44\x58\x54\x35";
            else if (dxt == "TCM_NormalsHigh")
                dxt = "\x44\x58\x54\x35";
            else if (dxt == "TCM_Normals")
                dxt = "\x44\x58\x54\x31";
        }

        xbmFile->seek(back2+seek);
        if (propertyName == "importFile")
            break;
    }
    log->addLineAndFlush("Read header ok");

    xbmFile->seek(27, true); //readUnsignedChars(fileXBM, 27);

    // If a compression method has been found
    if (dxt.size() > 0)
    {
        // Create the DDS file
        io::IWriteFile* fileDDS = FileSystem->createAndWriteFile((filenameDDS).c_str());

        if (!fileDDS)
        {
            log->addLineAndFlush(formatString("Error : the file %s can't be created.", filenameDDS.c_str()));
        }
        else
        {
            log->addLineAndFlush(formatString("File %s created", filenameDDS.c_str()));
        }

        // The static part of the header
        fileDDS->write(ddsheader, 128);

        // And the informations that we have read
        fileDDS->seek(0xC);
        fileDDS->write(&height1, 4);
        fileDDS->seek(0x10);
        fileDDS->write(&width1, 4);
        fileDDS->seek(0x54);
        fileDDS->write(dxt.c_str(), dxt.size());
        fileDDS->seek(128);

        // Header is ok
        log->addLineAndFlush("DDS header OK");

        // copy the content of the file
        const long sizeToCopy = xbmFile->getSize() - xbmFile->getPos();


        char* buffer = new char[sizeToCopy];
        xbmFile->read(buffer, sizeToCopy);

        log->addLineAndFlush("Read XBM OK");

        fileDDS->write(buffer, sizeToCopy);
        delete[] buffer;

        log->addLineAndFlush("Write DDS OK");

        fileDDS->drop();
    }
    else
    {
        log->addLineAndFlush("TODO: Empty texture !");
    }
}

void IO_MeshLoader_W2ENT::vert_format(io::IReadFile* file)
{
    IdLOD.clear();

    core::array<u8> data = readDataArray<u8>(file, 8);

    // ???
    if (data[3] != 5)
        return;

    // Read the LODS data
    u8 nLODS = readU8(file);
    for (u8 i = 0; i < nLODS; i++)
    {
        data = readDataArray<u8>(file, 5);
        readDataArray<u8>(file, data[0]*2);
        readDataArray<u8>(file, 6);
        IdLOD.push_back(data);
    }
}

} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_W2ENT_LOADER_

