// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#define _IRR_COMPILE_WITH_W3ENT_LOADER_
#ifdef _IRR_COMPILE_WITH_W3ENT_LOADER_

#include "IO_MeshLoader_W3ENT.h"

#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "IReadFile.h"
#include "IWriteFile.h"

#include "Utils_Halffloat.h"
#include "Utils_Loaders_Irr.h"
#include "Utils_Qt_Irr.h"

#include "Settings.h"



namespace irr
{
namespace scene
{

//! Constructor
IO_MeshLoader_W3ENT::IO_MeshLoader_W3ENT(scene::ISceneManager* smgr, io::IFileSystem* fs)
: meshToAnimate(nullptr),
  SceneManager(smgr),
  FileSystem(fs),
  AnimatedMesh(nullptr),
  FrameOffset(0),
  log(nullptr),
  ConfigLoadSkeleton(true),
  ConfigLoadOnlyBestLOD(false)
{
	#ifdef _DEBUG
    setDebugName("CW3ENTMeshFileLoader");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool IO_MeshLoader_W3ENT::isALoadableFileExtension(const io::path& filename) const
{
    io::IReadFile* file = SceneManager->getFileSystem()->createAndOpenFile(filename);
    if (!file)
        return false;

    bool checkIsLoadable = (getRedEngineFileType(file) == REV_WITCHER_3);
    file->drop();

    return checkIsLoadable;
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* IO_MeshLoader_W3ENT::createMesh(io::IReadFile* f)
{
    Feedback = "";
	if (!f)
        return nullptr;

    #ifdef _IRR_WCHAR_FILESYSTEM
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsStringW("TW_GAME_PATH");
        ConfigGameTexturesPath = SceneManager->getParameters()->getAttributeAsStringW("TW_TW3_TEX_PATH");
    #else
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsString("TW_GAME_PATH");
        ConfigGameTexturesPath = SceneManager->getParameters()->getAttributeAsString("TW_TW3_TEX_PATH");
    #endif

    ConfigLoadSkeleton = SceneManager->getParameters()->getAttributeAsBool("TW_TW3_LOAD_SKEL");
    ConfigLoadOnlyBestLOD = SceneManager->getParameters()->getAttributeAsBool("TW_TW3_LOAD_BEST_LOD_ONLY");

    //Clear up
    Strings.clear();
    Materials.clear();
    Files.clear();
    Meshes.clear();

    log = Log::Instance();

    if (log->isEnabled() && !log->works())
    {
        Feedback += "\nError : The log file can't be created\nCheck that you don't use special characters in your software path. (Unicode isn't supported)\n";
        return nullptr;
    }

    writeLogHeader(f);
    log->addLineAndFlush("Start loading");

    AnimatedMesh = SceneManager->createSkinnedMesh();

	if (load(f))
	{
        /*
        for (u32 i = 0; i < Meshes.size(); ++i)
        {
            combineMeshes(AnimatedMesh, Meshes[i], true);
            Meshes[i]->drop();
        }
        */

		AnimatedMesh->finalize();
        //Feedback += "done";
        // No feedback = 'done' feedback so it's necesseray to fill it

		//SceneManager->getMeshManipulator()->recalculateNormals(AnimatedMesh);
        //SceneManager->getMeshManipulator()->flipSurfaces(AnimatedMesh);
	}
	else
	{
		AnimatedMesh->drop();
        AnimatedMesh = nullptr;
	}

    log->addLineAndFlush("LOADING FINISHED");

    SceneManager->getParameters()->setAttribute("TW_FEEDBACK", Feedback.c_str());

	return AnimatedMesh;
}

void IO_MeshLoader_W3ENT::writeLogBoolProperty(core::stringc name, bool value)
{
    log->add(formatString("-> %s is ", name.c_str()));
    if (value)
        log->addLine("enabled");
    else
        log->addLine("disabled");
}

void IO_MeshLoader_W3ENT::writeLogHeader(const io::IReadFile* f)
{
    log->addLine("");
    log->addLine(formatString("-> File : %s", f->getFileName().c_str()));

    writeLogBoolProperty("Load Sekeleton", ConfigLoadSkeleton);
    writeLogBoolProperty("Load only best LOD", ConfigLoadOnlyBestLOD);

    log->add("_________________________________________________________\n\n\n");
}

void checkMaterial(video::SMaterial mat)
{
    if (mat.getTexture(0))
        ;//std::cout << "SLOT 1 = " <<mat.getTexture(0)->getName().getPath().c_str() << std::endl;
    else
        ;//std::cout << "Le material n'a pas de tex slot 1" << std::endl;
}

bool IO_MeshLoader_W3ENT::W3_load(io::IReadFile* file)
{
    RedEngineFileHeader header;
    loadTW3FileHeader(file, header);
    Strings = header.Strings;
    Files = header.Files;

    log->addLineAndFlush("Read strings and files");

    file->seek(12);
    core::array<s32> headerData = readDataArray<s32>(file, 38);
    s32 contentChunkStart = headerData[19];
    s32 contentChunkSize = headerData[20];

    core::array<W3_DataInfos> meshes;
    file->seek(contentChunkStart);
    for (s32 i = 0; i < contentChunkSize; ++i)
    {
        W3_DataInfos infos;
        u16 dataType = readU16(file);
        core::stringc dataTypeName = Strings[dataType];
        log->addLineAndFlush(formatString("dataTypeName=%s", dataTypeName.c_str()));

        file->seek(6, true);

        file->read(&infos.size, 4);
        file->read(&infos.adress, 4);
        //std::cout << "begin at " << infos.adress << " and end at " << infos.adress + infos.size << std::endl;

        file->seek(8, true);

        s32 back = file->getPos();
        if (dataTypeName == "CMesh")
        {
            meshes.push_back(infos);
            //W3_CMesh(file, infos);
            log->addLineAndFlush("Find a mesh");
        }
        else if (dataTypeName == "CMaterialInstance")
        {
            log->addLineAndFlush("Find a material");
            video::SMaterial mat = W3_CMaterialInstance(file, infos);
            //checkMaterial(mat);
            log->addLineAndFlush("Material loaded");
            Materials.push_back(mat);
            log->addLineAndFlush("Added to mat list");
        }
        else if (dataTypeName == "CEntityTemplate")
        {
            W3_CEntityTemplate(file, infos);
        }
        else if (dataTypeName == "CEntity")
        {
            W3_CEntity(file, infos);
        }
        else if (dataTypeName == "CMeshComponent")
        {
            W3_CMeshComponent(file, infos);
        }
        else if (dataTypeName == "CSkeleton")
        {
            W3_CSkeleton(file, infos);
        }
        else if (dataTypeName == "CAnimationBufferBitwiseCompressed" && meshToAnimate)
        {
            W3_CAnimationBufferBitwiseCompressed(file, infos);
        }
        else
        {
            W3_CUnknown(file, infos);
        }
        file->seek(back);
    }

    for (u32 i = 0; i < meshes.size(); ++i)
    {
        log->addAndFlush("Load mesh...");
        W3_CMesh(file, meshes[i]);
        log->addLineAndFlush("done");
    }

    return true;
}



bool IO_MeshLoader_W3ENT::W3_ReadBuffer(io::IReadFile* file, SBufferInfos bufferInfos, SMeshInfos meshInfos)
{
    SVertexBufferInfos vBufferInf;
    u32 nbVertices = 0;
    u32 firstVertexOffset = 0;
    u32 nbIndices = 0;
    u32 firstIndiceOffset = 0;
    for (u32 i = 0; i < bufferInfos.verticesBuffer.size(); ++i)
    {
        nbVertices += bufferInfos.verticesBuffer[i].nbVertices;
        if (nbVertices > meshInfos.firstVertex)
        {
            vBufferInf = bufferInfos.verticesBuffer[i];
            // the index of the first vertex in the buffer
            firstVertexOffset = meshInfos.firstVertex - (nbVertices - vBufferInf.nbVertices);
            //std::cout << "firstVertexOffset=" << firstVertexOffset << std::endl;
            break;
        }
    }
    for (u32 i = 0; i < bufferInfos.verticesBuffer.size(); ++i)
    {
        nbIndices += bufferInfos.verticesBuffer[i].nbIndices;
        if (nbIndices > meshInfos.firstIndice)
        {
            vBufferInf = bufferInfos.verticesBuffer[i];
            firstIndiceOffset = meshInfos.firstIndice - (nbIndices - vBufferInf.nbIndices);
            //std::cout << "firstIndiceOffset=" << firstVertexOffset << std::endl;
            break;
        }
    }

    // Check if it's the best LOD
    if (ConfigLoadOnlyBestLOD && vBufferInf.lod != 1)
        return false;

    io::IReadFile* bufferFile = FileSystem->createAndOpenFile(file->getFileName() + ".1.buffer");
    if (!bufferFile)
    {
        Feedback += "\nThe .buffer file associated to the mesh hasn't been found.\nHave you extracted the necessary bundle ?\n";
        log->addAndFlush(" fail to open .buffer file ");
        return false;
    }


    scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
    buffer->VertexType = video::EVT_STANDARD;
    //std::cout << "Num vertices=" << meshInfos.numVertices << std::endl;
    buffer->Vertices_Standard.reallocate(meshInfos.numVertices);

    u32 vertexSize = 8;
    if (meshInfos.vertexType == EMVT_SKINNED)
        vertexSize += meshInfos.numBonesPerVertex * 2;

    //std::cout << "first vertex = " << meshInfos.firstVertex << std::endl;
    // Maybe it's simply 1 verticesBufferInfo/buffer, seems correct and more simple
    //if (bufferInfos.verticesBuffer.size() == AnimatedMesh->getMeshBufferCount())
    //    std::cout << "--> 1 verticesBufferInfo/buffer" << std::endl;


    bufferFile->seek(vBufferInf.verticesCoordsOffset + firstVertexOffset * vertexSize);
    //std::cout << "POS vCoords=" << bufferFile->getPos() << std::endl;

    const video::SColor defaultColor(255, 255, 255, 255);
    for (u32 i = 0; i < meshInfos.numVertices; ++i)
    {
        u16 x, y, z, w;

        bufferFile->read(&x, 2);
        bufferFile->read(&y, 2);
        bufferFile->read(&z, 2);
        bufferFile->read(&w, 2);

        // skip skinning data
        if (meshInfos.vertexType == EMVT_SKINNED && !ConfigLoadSkeleton)
        {
            bufferFile->seek(meshInfos.numBonesPerVertex * 2, true);
        }
        else if (meshInfos.vertexType == EMVT_SKINNED)
        {
            unsigned char skinningData[meshInfos.numBonesPerVertex * 2];
            bufferFile->read(&skinningData[0], meshInfos.numBonesPerVertex * 2);

            for (u32 j = 0; j < meshInfos.numBonesPerVertex; ++j)
            {
                unsigned char boneId = skinningData[j];
                unsigned char weightStrength = skinningData[j + meshInfos.numBonesPerVertex];

                if (boneId >= AnimatedMesh->getJointCount()) // If bone don't exist
                    continue;

                if (weightStrength != 0)
                {
                    scene::ISkinnedMesh::SJoint* joint = AnimatedMesh->getAllJoints()[boneId];
                    u32 bufferId = AnimatedMesh->getMeshBufferCount() - 1;
                    f32 fWeightStrength = weightStrength / 255.f;

                    scene::ISkinnedMesh::SWeight* weight = AnimatedMesh->addWeight(joint);
                    weight->buffer_id = bufferId;
                    weight->strength = fWeightStrength;
                    weight->vertex_id = i;
                    //std::cout << "TEST:" << fweight << ", " << bufferId << ", " << i << std::endl;

                    TW3_DataCache::_instance.addVertexEntry(boneId, bufferId, i, fWeightStrength);
                }
            }

        }

        buffer->Vertices_Standard.push_back(video::S3DVertex());
        buffer->Vertices_Standard[i].Pos = core::vector3df(x, y, z) / 65535.f * bufferInfos.quantizationScale + bufferInfos.quantizationOffset;
        buffer->Vertices_Standard[i].Color = defaultColor;
        //std::cout << "Position=" << x << ", " << y << ", " << z << std::endl;
    }
    bufferFile->seek(vBufferInf.uvOffset + firstVertexOffset * 4);
    //std::cout << "POS vUV=" << bufferFile->getPos() << std::endl;

    for (u32 i = 0; i < meshInfos.numVertices; ++i)
    {
        u16 u, v;
        bufferFile->read(&u, 2);
        bufferFile->read(&v, 2);

        f32 uf = halfToFloat(u);
        f32 vf = halfToFloat(v);

        buffer->Vertices_Standard[i].TCoords = core::vector2df(uf, vf);
    }


    // Not sure...
    /*
    bufferFile->seek(vBufferInf.normalsOffset + firstVertexOffset * 8);
    std::cout << "POS vNormals=" << bufferFile->getPos() << std::endl;
    for (u32 i = 0; i < meshInfos.numVertices; ++i)
    {
        s16 x, y, z, w;

        bufferFile->read(&x, 2);
        bufferFile->read(&y, 2);
        bufferFile->read(&z, 2);
        bufferFile->read(&w, 2);

        //std::cout << "Position=" << x * infos.quantizationScale.X<< ", " << y * infos.quantizationScale.Y<< ", " << z << std::endl;
        buffer->Vertices_Standard[i].Normal = core::vector3df(x, y, z);
        buffer->Vertices_Standard[i].Normal.normalize();
    }
    */


    // Indices -------------------------------------------------------------------
    bufferFile->seek(bufferInfos.indicesBufferOffset + vBufferInf.indicesOffset + firstIndiceOffset * 2);

    //std::cout << "POS Indices=" << bufferFile->getPos() - bufferInfos.indicesBufferOffset << std::endl;
    //std::cout << "num indices=" << meshInfos.numIndices << std::endl;
    buffer->Indices.set_used(meshInfos.numIndices);
    for (u32 i = 0; i < meshInfos.numIndices; ++i)
    {
        const u16 indice = readU16(bufferFile);

        // Indice need to be inversed for the normals
        if (i % 3 == 0)
            buffer->Indices[i] = indice;
        else if (i % 3 == 1)
            buffer->Indices[i+1] = indice;
        else if (i % 3 == 2)
            buffer->Indices[i-1] = indice;
    }

    SceneManager->getMeshManipulator()->recalculateNormals(buffer);
    bufferFile->drop();

    return true;
}

bool IO_MeshLoader_W3ENT::ReadPropertyHeader(io::IReadFile* file, SPropertyHeader& propHeader)
{
    u16 propName = readU16(file);
    u16 propType = readU16(file);

    if (propName == 0 || propType == 0 || propName >= Strings.size() || propType >= Strings.size())
        return false;

    propHeader.propName = Strings[propName];
    propHeader.propType = Strings[propType];

    const long back = file->getPos();
    propHeader.propSize = readS32(file);
    //file->seek(-4, true);

    propHeader.endPos = back + propHeader.propSize;

    return true;
}

inline u32 IO_MeshLoader_W3ENT::ReadUInt32Property(io::IReadFile* file)
{
    return readU32(file);
}

inline u8 IO_MeshLoader_W3ENT::ReadUInt8Property(io::IReadFile* file)
{
    return readU8(file);
}

inline f32 IO_MeshLoader_W3ENT::ReadFloatProperty(io::IReadFile* file)
{
    return readF32(file);
}

bool IO_MeshLoader_W3ENT::ReadBoolProperty(io::IReadFile* file)
{
    u8 valueChar = readU8(file);
    bool value = (valueChar == 0) ? false : true;
    return value;
}

SAnimationBufferBitwiseCompressedData IO_MeshLoader_W3ENT::ReadSAnimationBufferBitwiseCompressedDataProperty(io::IReadFile* file)
{
    file->seek(1, true);
    SAnimationBufferBitwiseCompressedData dataInf;

    while(1)
    {
        SPropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
            break;

        //std::cout << "@" << file->getPos() <<", property = " << propHeader.propName.c_str() << ", type = " << propHeader.propType.c_str() << std::endl;
        if (propHeader.propName == "dataAddr")
            dataInf.dataAddr = ReadUInt32Property(file);
        if (propHeader.propName == "dataAddrFallback")
            dataInf.dataAddrFallback = ReadUInt32Property(file);
        if (propHeader.propName == "numFrames")
            dataInf.numFrames = readU16(file);
        if (propHeader.propName == "dt")
            dataInf.dt = readF32(file);
        if (propHeader.propName == "compression")
            dataInf.compression = readS8(file);

        file->seek(propHeader.endPos);
    }
    return dataInf;
}

core::array<core::array<SAnimationBufferBitwiseCompressedData> > IO_MeshLoader_W3ENT::ReadSAnimationBufferBitwiseCompressedBoneTrackProperty(io::IReadFile* file)
{
    s32 arraySize = readS32(file);
    file->seek(1, true);
    log->addLineAndFlush(formatString("Array size = %d", arraySize));

    core::array<core::array<SAnimationBufferBitwiseCompressedData> > inf;
    inf.push_back(core::array<SAnimationBufferBitwiseCompressedData>());

    int i = 0;              // array index
    while(i <= arraySize)   // the array index = bone index
    {
        SPropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
        {
            ++i;
            if (i == arraySize) // end of the array
                break;

            // go back and re-read
            file->seek(-1, true);
            ReadPropertyHeader(file, propHeader);
            inf.push_back(core::array<SAnimationBufferBitwiseCompressedData>());

            log->addLineAndFlush(formatString("New bone : (index = %d)", i));
        }


        //std::cout << "@" << file->getPos() <<", property = " << propHeader.propName.c_str() << ", type = " << propHeader.propType.c_str() << std::endl;
        if (propHeader.propType == "SAnimationBufferBitwiseCompressedData")
        {
            SAnimationBufferBitwiseCompressedData animInf = ReadSAnimationBufferBitwiseCompressedDataProperty(file);

            if (propHeader.propName == "position")
                animInf.type = EATT_POSITION;
            else if (propHeader.propName == "orientation")
                animInf.type = EATT_ORIENTATION;
            else if (propHeader.propName == "scale")
                animInf.type = EATT_SCALE;

            inf[inf.size() - 1].push_back(animInf);
        }

        file->seek(propHeader.endPos);
    }

    return inf;
}

core::vector3df IO_MeshLoader_W3ENT::ReadVector3Property(io::IReadFile* file)
{
    float x, y, z, w;
    file->seek(1, true);

    file->seek(8, true);    // 2 index of the Strings table (Name + type -> X, Float) + prop size
    x = ReadFloatProperty(file);
    file->seek(8, true);
    y = ReadFloatProperty(file);
    file->seek(8, true);
    z = ReadFloatProperty(file);
    file->seek(8, true);
    w = ReadFloatProperty(file);

    return core::vector3df(x, y, z);
}

void IO_MeshLoader_W3ENT::ReadMaterialsProperty(io::IReadFile* file)
{
    s32 nbChunks = readS32(file);

    //std::cout << "NB material = -> " << nbChunks << std::endl;
    //file->seek(1, true);

    core::array<video::SMaterial> matMats;

    for (u32 i = 0; i < nbChunks; ++i)
    {
        u32 matValue = readU32(file);
        u32 matFileID = 0xFFFFFFFF - matValue;

        if (matFileID < Files.size()) // Refer to a w2mi file
        {
            //std::cout << "w2mi file = " << Files[matFileID].c_str() << std::endl;
            matMats.push_back(ReadMaterialFile(ConfigGamePath + Files[matFileID]));
            //file->seek(3, true);
        }
        else
        {
            u32 value = matValue;
            //std::cout << "val = " << value << std::endl;
            //Materials.push_back(Materials[value-1]);
        }

    }
    for (u32 i = 0; i < matMats.size(); ++i)
    {
        Materials.push_front(matMats[matMats.size() - 1 - i]);
    }
}

EMeshVertexType IO_MeshLoader_W3ENT::ReadEMVTProperty(io::IReadFile* file)
{
    u16 enumStringId = readU16(file);

    EMeshVertexType vertexType = EMVT_STATIC;

    core::stringc enumString = Strings[enumStringId];
    if (enumString == "MVT_SkinnedMesh")
    {
        vertexType = EMVT_SKINNED;
    }

    return vertexType;
}

SAnimationBufferOrientationCompressionMethod IO_MeshLoader_W3ENT::ReadAnimationBufferOrientationCompressionMethodProperty(io::IReadFile* file)
{
    u16 enumStringId = readU16(file);
    core::stringc enumString = Strings[enumStringId];

    if (enumString == "ABOCM_PackIn48bitsW")
        return ABOCM_PackIn48bitsW;
    else
        std::cout << "NEW ORIENTATION COMPRESSION METHOD" << std::endl;

    return (SAnimationBufferOrientationCompressionMethod)0;
}

core::array<SMeshInfos> IO_MeshLoader_W3ENT::ReadSMeshChunkPackedProperty(io::IReadFile* file)
{
    core::array<SMeshInfos> meshes;
    SMeshInfos meshInfos;

    s32 nbChunks = readS32(file);

    //std::cout << "NB = -> " << nbChunks << std::endl;

    file->seek(1, true);

    s32 chunkId = 0;

    while(1)
    {
        SPropertyHeader propHeader;

        // invalid property = next chunk
        if (!ReadPropertyHeader(file, propHeader))
        {
            meshes.push_back(meshInfos);
            chunkId++;

            if (chunkId >= nbChunks)
                break;
            else
            {
                SMeshInfos newMeshInfos;
                newMeshInfos.vertexType = meshInfos.vertexType;
                newMeshInfos.numBonesPerVertex = meshInfos.numBonesPerVertex;
                meshInfos = newMeshInfos;

                file->seek(-1, true);
                ReadPropertyHeader(file, propHeader);
            }
        }

        //std::cout << "@" << file->getPos() <<", property = " << propHeader.propName.c_str() << ", type = " << propHeader.propType.c_str() << std::endl;

        if (propHeader.propName == "numIndices")
        {
            meshInfos.numIndices = ReadUInt32Property(file);
            //std::cout << "numIndices = " << meshInfos.numIndices << std::endl;
        }
        else if (propHeader.propName == "numVertices")
        {
            meshInfos.numVertices = ReadUInt32Property(file);
            //std::cout << "numVertices = " << meshInfos.numVertices << std::endl;
        }
        else if (propHeader.propName == "firstVertex")
        {
            meshInfos.firstVertex = ReadUInt32Property(file);
            //std::cout << "first vertex found (=" << meshInfos.firstVertex << ")" << std::endl;
        }
        else if (propHeader.propName == "firstIndex")
        {
            meshInfos.firstIndice = ReadUInt32Property(file);
            //std::cout << "firstIndice = " << meshInfos.firstIndice << std::endl;
        }
        else if (propHeader.propName == "vertexType")
        {
            meshInfos.vertexType = ReadEMVTProperty(file);
        }
        else if (propHeader.propName == "numBonesPerVertex")
        {
            meshInfos.numBonesPerVertex = ReadUInt8Property(file);
        }
        else if (propHeader.propName == "materialID")
        {
            meshInfos.materialID = ReadUInt32Property(file);
            //std::cout << "material ID = " << meshInfos.materialID << std::endl;
        }

        file->seek(propHeader.endPos);
    }


    return meshes;
}

void IO_MeshLoader_W3ENT::ReadRenderChunksProperty(io::IReadFile* file, SBufferInfos* buffer)
{
    s32 nbElements = readS32(file); // array size (= bytes count here)
    //std::cout << "nbElem = " << nbElements << ", @= " << file->getPos() << ", end @=" << back + propSize << std::endl;
    //const long back = file->getPos();

    //file->seek(1, true);
    u8 nbBuffers = readU8(file);
    //std::cout << "nbBuffers = " << (u32)nbBuffers << std::endl;

    //while(file->getPos() - back < nbElements)
    for (u32 i = 0; i < nbBuffers; ++i)
    {
        //std::cout << "@@@ -> " << file->getPos() << std::endl;
        SVertexBufferInfos buffInfos;
        file->seek(1, true); // Unknown

        file->read(&buffInfos.verticesCoordsOffset, 4);
        file->read(&buffInfos.uvOffset, 4);
        file->read(&buffInfos.normalsOffset, 4);

        file->seek(9, true); // Unknown
        file->read(&buffInfos.indicesOffset, 4);
        file->seek(1, true); // 0x1D

        file->read(&buffInfos.nbVertices, 2);
        //std::cout << "Nb VERT=" << buffInfos.nbVertices << std::endl;
        file->read(&buffInfos.nbIndices, 4);
        file->seek(3, true); // Unknown
        buffInfos.lod = readU8(file); // lod ?

        buffer->verticesBuffer.push_back(buffInfos);
    }
}

video::SMaterial IO_MeshLoader_W3ENT::ReadIMaterialProperty(io::IReadFile* file)
{
    log->addLineAndFlush("IMaterial");
    video::SMaterial mat;
    mat.MaterialType = video::EMT_SOLID;

    s32 nbProperty = readS32(file);
    //std::cout << "nb property = " << nbProperty << std::endl;
    //std::cout << "adress = " << file->getPos() << std::endl;

    // Read the properties of the material
    for (u32 i = 0; i < nbProperty; ++i)
    {
        log->addAndFlush("property...");
        const s32 back = file->getPos();

        s32 propSize = readS32(file);

        u16 propId, propTypeId;
        file->read(&propId, 2);
        file->read(&propTypeId, 2);

        if (propId >= Strings.size())
            break;

        log->addAndFlush(formatString("The property is %s of the type %s", Strings[propId].c_str(), Strings[propTypeId].c_str()));

        const s32 textureLayer = getTextureLayerFromTextureType(Strings[propId]);
        if (textureLayer != -1)
        {
            u8 texId = readU8(file);
            texId = 255 - texId;

            if (texId < Files.size())
            {
                video::ITexture* texture = nullptr;
                texture = getTexture(Files[texId]);

                if (texture)
                {
                    log->addAndFlush(formatString("load %s ", Files[texId].c_str()));
                    mat.setTexture(textureLayer, texture);

                    if (textureLayer == 1)  // normal map
                        mat.MaterialType = video::EMT_NORMAL_MAP_SOLID;
                }
                else
                {
                    Feedback += "Some textures havn't been found, have you correctly set your textures directory ?\n";
                    log->addLineAndFlush(formatString("Error : the file %s can't be opened.", Files[texId].c_str()));
                }
            }
        }

        file->seek(back + propSize);
        log->addLineAndFlush("...OK");
    }

    log->addLineAndFlush("IMaterial OK");
    //checkMaterial(mat);
    return mat;
}

core::array<core::vector3df> IO_MeshLoader_W3ENT::ReadBonesPosition(io::IReadFile* file)
{
    s32 nbBones = readS32(file);

    file->seek(1, true);

    core::array<core::vector3df> positions;
    for (u32 i = 0; i < nbBones; ++i)
    {
        file->seek(8, true);
        float x = ReadFloatProperty(file);
        file->seek(8, true);
        float y = ReadFloatProperty(file);
        file->seek(8, true);
        float z = ReadFloatProperty(file);
        file->seek(8, true);
        float w = ReadFloatProperty(file);

        core::vector3df position = core::vector3df(x, y, z);
        positions.push_back(position);

        //std::cout << "position = " << x << ", " << y << ", " << z << ", " << w << std::endl;
        file->seek(3, true);
    }
    return positions;
}

void IO_MeshLoader_W3ENT::ReadRenderLODSProperty(io::IReadFile* file)
{
    // LOD distances ?
    u32 arraySize = readU32(file);
    for (u32 i = 0; i < arraySize; ++i)
    {
        f32 value = readF32(file);
        //std::cout << "Value : " << value << std::endl;
    }
}

SBufferInfos IO_MeshLoader_W3ENT::ReadSMeshCookedDataProperty(io::IReadFile* file)
{
    SBufferInfos bufferInfos;

    file->seek(1, true);

    SPropertyHeader propHeader;
    while(ReadPropertyHeader(file, propHeader))
    {
        //std::cout << "@" << file->getPos() <<", property = " << propHeader.propName.c_str() << ", type = " << propHeader.propType.c_str() << std::endl;

        if (propHeader.propName == "indexBufferSize")
        {
            bufferInfos.indicesBufferSize = ReadUInt32Property(file);
        }
        else if (propHeader.propName == "indexBufferOffset")
        {
            bufferInfos.indicesBufferOffset = ReadUInt32Property(file);
        }
        else if (propHeader.propName == "vertexBufferSize")
        {
            bufferInfos.verticesBufferSize = ReadUInt32Property(file);
        }
        else if (propHeader.propName == "quantizationScale")
        {
            bufferInfos.quantizationScale = ReadVector3Property(file);
        }
        else if (propHeader.propName == "quantizationOffset")
        {
            bufferInfos.quantizationOffset = ReadVector3Property(file);
        }
        else if (propHeader.propName == "bonePositions")
        {
            core::array<core::vector3df> positions = ReadBonesPosition(file);
            NbBonesPos = positions.size();
        }
        //else if (propHeader.propName == "collisionInitPositionOffset")
        //    ReadVector3Property(file, &bufferInfos);
        else if (propHeader.propName == "renderChunks")
            ReadRenderChunksProperty(file, &bufferInfos);
        else if (propHeader.propName == "renderLODs")
            ReadRenderLODSProperty(file);
        //else
        //    ReadUnknowProperty(file);
        file->seek(propHeader.endPos);
    }

    return bufferInfos;
}

float read16bitsFloat(io::IReadFile* file)
{
    u32 bits = readU16(file) << 16;
    f32 f;
    memcpy(&f, &bits, 4);
    return f;
}

// need a test file
float read24bitsFloat(io::IReadFile* file)
{
    u32 bits = readU16(file) << 16;
    bits |= readU8(file) << 8;
    f32 f;
    memcpy(&f, &bits, 4);
    return f;
}

float readCompressedFloat(io::IReadFile* file, u8 compressionSize)
{
    if (compressionSize == 16)
        return read16bitsFloat(file);
    else if (compressionSize == 24) // Not tested yet !
        return read24bitsFloat(file);
    else
        return readF32(file); // Not tested yet !
}

/*
// old version
float bits12ToFloat(s16 value)
{
    if (value & 0x0800)
        value = value;
    else
        value = -value;

    value = (value & 0x000007FF);
    float fVal = value / 2047.f;
    return fVal;
}
*/

// Fixed by Ákos Köte, thx
float bits12ToFloat(s16 value)
{
    float fVal = (2047.0f - value) * (1 / 2048.0f);
    return fVal;
}

core::stringc getAnimTrackString(EAnimTrackType type)
{
    if (type == EATT_POSITION)
        return "EATT_POSITION";
    else if (type == EATT_ORIENTATION)
        return "EATT_ORIENTATION";
    else
        return "EATT_SCALE";
}

void IO_MeshLoader_W3ENT::readAnimBuffer(core::array<core::array<SAnimationBufferBitwiseCompressedData> >& inf, io::IReadFile* dataFile, SAnimationBufferOrientationCompressionMethod c)
{
    // Create bones to store the keys if they doesn't exist
    /*
    if (meshToAnimate->getJointCount() < inf.size())
    {
        for (u32 i = meshToAnimate->getJointCount(); i < inf.size(); ++i)
            meshToAnimate->addJoint();
    }
    */


    for (u32 i = 0; i < inf.size(); ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = meshToAnimate->getAllJoints()[i];
        log->addLineAndFlush(formatString("---> JOINT : %s", joint->Name.c_str()));

        for (u32 j = 0; j < inf[i].size(); ++j)
        {
            SAnimationBufferBitwiseCompressedData infos = inf[i][j];
            dataFile->seek(infos.dataAddr);

            // Debug infos
            log->addLineAndFlush(formatString("--> Type : %s", getAnimTrackString(infos.type).c_str()));

            log->addLineAndFlush(formatString("numFrames = %d", infos.numFrames));
            log->addLineAndFlush(formatString("dt = %f", infos.dt));
            log->addLineAndFlush(formatString("@ = %d", dataFile->getPos()));
            //std::cout << "compression=" << (int)infos.compression << std::endl;

            // TODO
            for (u32 f = 0; f < infos.numFrames; ++f)
            {
                u32 keyframe = f;
                keyframe += FrameOffset;


                //std::cout << "Adress = " << dataFile->getPos() << std::endl;
                u8 compressionSize = 0; // no compression
                if (infos.compression == 1)
                    compressionSize = 24;
                else if (infos.compression == 2)
                    compressionSize = 16;

                if (infos.type == EATT_POSITION)
                {
                    //std::cout << "compressionSize= " << (u32)compressionSize << std::endl;
                    f32 px = readCompressedFloat(dataFile, compressionSize);
                    f32 py = readCompressedFloat(dataFile, compressionSize);
                    f32 pz = readCompressedFloat(dataFile, compressionSize);

                    log->addLineAndFlush(formatString("Position value = %f, %f, %f", px, py, pz));

                    /*
                    scene::ISkinnedMesh::SPositionKey* key = meshToAnimate->addPositionKey(joint);
                    key->position = core::vector3df(px, py, pz);
                    key->frame = keyframe;
                    */

                }
                if (infos.type == EATT_ORIENTATION)
                {
                    core::quaternion orientation;
                    if (c == ABOCM_PackIn48bitsW)
                    {
                        uint64_t b1 = readU8(dataFile);
                        uint64_t b2 = readU8(dataFile);
                        uint64_t b3 = readU8(dataFile);
                        uint64_t b4 = readU8(dataFile);
                        uint64_t b5 = readU8(dataFile);
                        uint64_t b6 = readU8(dataFile);
                        uint64_t bits = b6 | (b5 << 8) | (b4 << 16) | (b3 << 24) | (b2 << 32) | (b1 << 40);
                        //dataFile->read(&bits, sizeof(uint64_t));


                        f32 fx, fy, fz, fw;

                        s16 x = 0, y = 0, z = 0, w = 0;
                        x = (bits & 0x0000FFF000000000) >> 36;
                        y = (bits & 0x0000000FFF000000) >> 24;
                        z = (bits & 0x0000000000FFF000) >> 12;
                        w =  bits & 0x0000000000000FFF;

                        /*
                        std::cout << std::dec << x << std::endl;
                        std::cout << std::hex << x << std::endl;
                        std::cout << std::hex << y << std::endl;
                        std::cout << std::hex << z << std::endl;
                        std::cout << std::hex << w << std::endl;
                        */

                        fx = bits12ToFloat(x);
                        fy = bits12ToFloat(y);
                        fz = bits12ToFloat(z);
                        fw = -bits12ToFloat(w);

                        orientation = core::quaternion(fx, fy, fz, fw);
                        core::vector3df euler;
                        orientation.toEuler(euler);
                        euler *= core::RADTODEG;

                        if (log->isEnabled())
                        {
                            log->addLine(formatString("Quaternion : x=%f, y=%f, z=%f, w=%f", fx, fy, fz, fw));
                            log->addLine(formatString("Quaternion mult : x=%f, y=%f, z=%f, w=%f", fx * fx + fy * fy + fz * fz + fw * fw));
                            log->addLine(formatString("Euler : x=%f, y=%f, z=%f", euler.X, euler.Y, euler.Z));
                            log->flush();
                        }
                    }

                    scene::ISkinnedMesh::SRotationKey* key = meshToAnimate->addRotationKey(joint);
                    key->rotation = orientation;
                    key->frame = keyframe;
                }
                if (infos.type == EATT_SCALE)
                {
                    //std::cout << "compressionSize= " << (int)compressionSize << std::endl;
                    f32 sx = readCompressedFloat(dataFile, compressionSize);
                    f32 sy = readCompressedFloat(dataFile, compressionSize);
                    f32 sz = readCompressedFloat(dataFile, compressionSize);

                    log->addLineAndFlush(formatString("Scale value = %f, %f, %f", sx, sy, sz));

                    scene::ISkinnedMesh::SScaleKey* key = meshToAnimate->addScaleKey(meshToAnimate->getAllJoints()[i]);
                    key->scale = core::vector3df(sx, sy, sz);
                    key->frame = keyframe;
                }
            }
            std::cout << std::endl;
        }
    }
}

void IO_MeshLoader_W3ENT::W3_CUnknown(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    //std::cout << "W3_CUnknown, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;
    log->addLineAndFlush("W3_CUnknown");

    SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {
        //std::cout << "-> @" << file->getPos() <<", property = " << propHeader.propName.c_str() << ", type = " << propHeader.propType.c_str() << std::endl;
        file->seek(propHeader.endPos);
    }
    log->addLineAndFlush("W3_CUnknown end");
}


void IO_MeshLoader_W3ENT::W3_CAnimationBufferBitwiseCompressed(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    //std::cout << "W3_CUnknown, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;
    log->addLineAndFlush("W3_CAnimationBufferBitwiseCompressed");

    core::array<core::array<SAnimationBufferBitwiseCompressedData> > inf;
    core::array<s8> data;
    io::IReadFile* dataFile = 0;
    SAnimationBufferOrientationCompressionMethod compress;

    f32 animDuration = 1.0f;
    u32 numFrames = 0;
    u16 defferedData = 0;

    SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {
        if (propHeader.propType == "array:129,0,SAnimationBufferBitwiseCompressedBoneTrack")
        {
            inf = ReadSAnimationBufferBitwiseCompressedBoneTrackProperty(file);

        }
        else if (propHeader.propName == "data")
        {
            u32 arraySize = readU32(file);
            data = readDataArray<s8>(file, arraySize);
        }
        else if (propHeader.propName == "orientationCompressionMethod")
        {
            compress = ReadAnimationBufferOrientationCompressionMethodProperty(file);
        }
        else if (propHeader.propName == "duration")
        {
            animDuration = readF32(file);
            log->addAndFlush(formatString("duration = %f", animDuration));
        }
        else if (propHeader.propName == "numFrames")
        {
            numFrames = readU32(file);
            log->addAndFlush(formatString("numFrames = %d", numFrames));
        }
        else if (propHeader.propName == "deferredData")
        {
            defferedData = readU16(file);
        }

        //std::cout << "-> @" << file->getPos() <<", property = " << propHeader.propName.c_str() << ", type = " << propHeader.propType.c_str() << std::endl;
        file->seek(propHeader.endPos);
    }

    f32 animationSpeed = (f32)numFrames / animDuration;
    meshToAnimate->setAnimationSpeed(animationSpeed);

    if (defferedData == 0)
        dataFile = FileSystem->createMemoryReadFile(data.pointer(), data.size(), "tempData");
    else
    {
        core::stringc filename = file->getFileName() + "." + toStr(defferedData) + ".buffer";
        std::cout << "Filename deffered = " << filename.c_str() << std::endl;
        dataFile = FileSystem->createAndOpenFile(filename);
    }


    if (dataFile)
    {
        readAnimBuffer(inf, dataFile, compress);
        dataFile->drop();
    }

    FrameOffset += numFrames;
    log->addLineAndFlush("W3_CAnimationBufferBitwiseCompressed end");
}

// sometimes toEuler give NaN numbers
void chechNaNErrors(core::vector3df& vector3)
{
    if (std::isnan(vector3.X) || std::isinf(vector3.X))
        vector3.X = 0.f;

    if (std::isnan(vector3.Y) || std::isinf(vector3.Y))
        vector3.Y = 0.f;

    if (std::isnan(vector3.Z) || std::isinf(vector3.Z))
        vector3.Z = 0.f;

}

TW3_CSkeleton IO_MeshLoader_W3ENT::W3_CSkeleton(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    //std::cout << "W3_CSkeleton, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;
    log->addLineAndFlush("W3_CSkeleton");

    TW3_CSkeleton skeleton;

    SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {
        //std::cout << "-> @" << file->getPos() <<", property = " << property.c_str() << ", type = " << propertyType.c_str() << std::endl;

        if (propHeader.propName == "bones")
        {
            // array
            s32 nbBones = readS32(file);
            file->seek(1, true);

            skeleton.nbBones = nbBones;

            for (s32 i = 0; i < nbBones; ++i)
            {
                SPropertyHeader h;
                ReadPropertyHeader(file, h);  // name + StringANSI

                u8 nameSize = readU8(file);
                core::stringc name = readString(file, nameSize);
                skeleton.names.push_back(name);

                //std::cout << "name=" << name.c_str() << std::endl;

                // An other property (nameAsCName)
                file->seek(13, true); // nameAsCName + CName + size + CName string ID + 3 0x00 octets
            }
        }
        else if (propHeader.propName == "parentIndices")
        {
            //std::cout << "@EndOfProperty = " << propHeader.endPos << std::endl;
            s32 nbBones = readS32(file);

            for (s32 i = 0; i < nbBones; ++i)
            {
                s16 parentId = readS16(file);
                //std::cout << "parent ID=" << parentId << std::endl;

                skeleton.parentId.push_back(parentId);
            }

        }

        file->seek(propHeader.endPos);
    }

    // Now there are the transformations
    file->seek(-2, true);
    //std::cout << file->getPos() << std::endl;

    for (u32 i = 0; i < skeleton.nbBones; ++i)
    {
        //std::cout << "bone = " << skeleton.names[i].c_str() << std::endl;
        // position (vector 4) + quaternion (4 float) + scale (vector 4)
        core::vector3df position;
        position.X = readF32(file);
        position.Y = readF32(file);
        position.Z = readF32(file);
        readF32(file); // the w component

        core::quaternion orientation;
        orientation.X = readF32(file);
        orientation.Y = readF32(file);
        orientation.Z = readF32(file);
        orientation.W = readF32(file);

        core::vector3df scale;
        scale.X = readF32(file);
        scale.Y = readF32(file);
        scale.Z = readF32(file);
        readF32(file); // the w component

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

        //std::cout << "Rotation (NaN fixed) = " << euler.X << ", " << euler.Y << ", " << euler.Z << std::endl;
        //std::cout << "Rotation = " << euler.X * core::RADTODEG << ", " << euler.Y * core::RADTODEG << ", " << euler.Z * core::RADTODEG << std::endl;
        //std::cout << "Scale = " << scale.X << ", " << scale.Y << ", " << scale.Z << std::endl;
        //std::cout << std::endl;
    }

    Skeleton = skeleton;

    log->addLineAndFlush("W3_CSkeleton end");
    return skeleton;
}

void IO_MeshLoader_W3ENT::W3_CMeshComponent(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    //std::cout << "W3_CMeshComponent, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;
    log->addLineAndFlush("W3_CMeshComponent");

    SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {
        //std::cout << "-> @" << file->getPos() <<", property = " << property.c_str() << ", type = " << propertyType.c_str() << std::endl;

        if (propHeader.propName == "mesh")
        {
            u32 meshComponentValue = readU32(file);
            u32 fileId = 0xFFFFFFFF - meshComponentValue;
            TW3_DataCache::_instance._bufferID += AnimatedMesh->getMeshBufferCount();
            scene::ISkinnedMesh* mesh = ReadW2MESHFile(ConfigGamePath + Files[fileId]);
            TW3_DataCache::_instance._bufferID -= AnimatedMesh->getMeshBufferCount();
            if (mesh)
            {
                // Merge in the main mesh
                combineMeshes(AnimatedMesh, mesh, true);
                //Meshes.push_back(mesh);
            }
            else
            {
                log->addLineAndFlush(formatString("Fail to load %s", Files[fileId].c_str()));
            }
        }

        file->seek(propHeader.endPos);
    }

    log->addLineAndFlush("W3_CMeshComponent end");
}

void IO_MeshLoader_W3ENT::W3_CEntityTemplate(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    log->addLineAndFlush("W3_CEntityTemplate");
    //std::cout << "W3_CEntityTemplate, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;


    SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {

        //std::cout << "-> @" << file->getPos() <<", property = " << propHeader.propName.c_str() << ", type = " << propHeader.propType.c_str() << std::endl;

        if (propHeader.propName == "flatCompiledData") // array of u8
        {
            s32 arraySize = readS32(file);
            arraySize -= 4;

            std::cout << file->getPos() << std::endl;

            u8 data[arraySize];
            file->read(data, arraySize);


            io::IReadFile* entityFile = SceneManager->getFileSystem()->createMemoryReadFile(data, arraySize, "tmpMemFile.w2ent_MEMORY", true);
            if (!entityFile)
                log->addLineAndFlush("fail");

            IO_MeshLoader_W3ENT w3Loader(SceneManager, FileSystem);
            IAnimatedMesh* m = w3Loader.createMesh(entityFile);
            if (m)
                m->drop();

            entityFile->drop();
        }

        file->seek(propHeader.endPos);
    }
    log->addLineAndFlush("W3_CEntityTemplate end");
}

void IO_MeshLoader_W3ENT::W3_CEntity(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    //std::cout << "W3_CEntity, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;
}

bool IO_MeshLoader_W3ENT::checkBones(io::IReadFile* file, char nbBones)
{
    const long back = file->getPos();
    for (char i = 0; i < nbBones; ++i)
    {
        u16 jointName = readU16(file);
        if (jointName == 0 || jointName >= Strings.size())
        {
            file->seek(back);
            return false;
        }

    }
    file->seek(back);
    return true;
}

char readBonesNumber(io::IReadFile* file)
{
    s8 nbBones = readS8(file);

    s8 o = readS8(file);
    if (o != 1)
        file->seek(-1, true);

    return nbBones;
}

void IO_MeshLoader_W3ENT::W3_CMesh(io::IReadFile* file, W3_DataInfos infos)
{
    log->addLineAndFlush("W3_CMesh");

    SBufferInfos bufferInfos;
    core::array<SMeshInfos> meshes;

    bool isStatic = false;

    file->seek(infos.adress + 1);

    SPropertyHeader propHeader;
    while (ReadPropertyHeader(file, propHeader))
    {
        //std::cout << "-> @" << file->getPos() <<", property = " << propHeader.propName.c_str() << ", type = " << propHeader.propType.c_str() << std::endl;
        if (propHeader.propType == "SMeshCookedData")
        {
            log->addLineAndFlush("Buffer infos");
            bufferInfos = ReadSMeshCookedDataProperty(file);
        }
        else if (propHeader.propName == "chunks")
        {
            log->addLineAndFlush("Chunks");
            meshes = ReadSMeshChunkPackedProperty(file);
        }
        else if (propHeader.propName == "materials")
        {
            log->addLineAndFlush("Mats");
            ReadMaterialsProperty(file);
        }
        else if (propHeader.propName == "isStatic")
        {
            isStatic = ReadBoolProperty(file);
        }

        file->seek(propHeader.endPos);
   }

   log->addLineAndFlush(formatString("All properties readed, @=%d", file->getPos()));

   if (!isStatic && NbBonesPos > 0 && ConfigLoadSkeleton)
   {
        ReadBones(file);
   }

   for (u32 i = 0; i < meshes.size(); ++i)
   {
        log->addAndFlush("Read buffer...");
        if (!W3_ReadBuffer(file, bufferInfos, meshes[i]))
            continue;

        //std::cout << "Read a buffer, Material ID = "  << meshes[i].materialID << std::endl;
        if (meshes[i].materialID < Materials.size())
        {
            //std::cout << "Material assigned to meshbuffer" << std::endl;
            AnimatedMesh->getMeshBuffer(AnimatedMesh->getMeshBufferCount() - 1)->getMaterial() = Materials[meshes[i].materialID];
        }
        else
        {
            //std::cout << "Error, mat " << meshes[i].materialID << "doesn't exist" << std::endl;
            /*
            if (Materials.size() >= 1)
                AnimatedMesh->getMeshBuffer(AnimatedMesh->getMeshBufferCount() - 1)->getMaterial() = Materials[0];
            */
        }
        log->addLineAndFlush("OK");
   }
   log->addLineAndFlush("W3_CMesh end");
}

void IO_MeshLoader_W3ENT::ReadBones(io::IReadFile* file)
{
    log->addLineAndFlush("Load bones");

    // cancel property
    file->seek(-4, true);

    /*
    file->seek(2, true);
    char offsetInd;
    file->read(&offsetInd, 1);
    file->seek(offsetInd * 7, true);
    */


    // TODO
    //log->addAndFlush("NbBonesPos = ");
    log->addLineAndFlush(formatString("NbBonesPos = %d", NbBonesPos));
    long pos;
    unsigned char nbRead;
    do
    {
        pos = file->getPos();
        nbRead = readBonesNumber(file);

        if (nbRead == NbBonesPos)
        {
            if (!checkBones(file, nbRead))
            {
                nbRead = -1;
            }
        }

        if (file->getPos() >= file->getSize())
            return;
    }   while (nbRead != NbBonesPos);

    file->seek(pos);


    // Name of the bones
    char nbBones = readBonesNumber(file);

    //log->addAndFlush("nbBones = " + (s32)nbBones);
    //std::cout << "m size= " << meshes.size() << std::endl;

    for (char i = 0; i < nbBones; ++i)
    {
        u16 jointName = readU16(file);
        log->addLineAndFlush(formatString("string id = %d", jointName));

        scene::ISkinnedMesh::SJoint* joint = nullptr;
        //if (!AnimatedMesh->getJointCount())
             joint = AnimatedMesh->addJoint();
        //else
        //     joint = AnimatedMesh->addJoint(AnimatedMesh->getAllJoints()[0]);
        joint->Name = Strings[jointName];
    }

    // The matrix of the bones
    readBonesNumber(file);
    for (char i = 0; i < nbBones; ++i)
    {
        ISkinnedMesh::SJoint* joint = AnimatedMesh->getAllJoints()[i];
        core::matrix4 matrix;

        // the matrix
        for (u32 j = 0; j < 16; ++j)
        {
            f32 value = readF32(file);
            matrix[j] = value;
        }


        core::vector3df position = matrix.getTranslation();
        core::matrix4 invRot;
        matrix.getInverse(invRot);
        //invRot.rotateVect(position);

        core::vector3df rotation = invRot.getRotationDegrees();
        //rotation = core::vector3df(0, 0, 0);
        position = - position;
        core::vector3df scale = matrix.getScale();

        if (joint)
        {
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

            TW3_DataCache::_instance.addBoneEntry(joint->Name, matrix);
        }
    }

    // 1 float per bone ???
    readBonesNumber(file);
    for (char i = 0; i < nbBones; ++i)
    {
        float value = readF32(file);
        log->addLineAndFlush(formatString("value = %f", value));
    }

    // 1 s32 par bone. parent ID ? no
    readBonesNumber(file);
    for (char i = 0; i < nbBones; ++i)
    {
        u32 unk = readU32(file);
        //std::cout << "= " << joints[parent]->Name.c_str() << "->" << joints[i]->Name.c_str() << std::endl;
    }
    log->addLineAndFlush("Bones loaded");
}


ISkinnedMesh* IO_MeshLoader_W3ENT::ReadW2MESHFile(core::stringc filename)
{
    ISkinnedMesh* mesh = nullptr;
    io::IReadFile* meshFile = FileSystem->createAndOpenFile(filename);
    if (!meshFile)
    {
        log->addLineAndFlush(formatString("Fail to open the w2mesh file : %s", filename.c_str()));
    }
    else
    {
        IO_MeshLoader_W3ENT w3Loader(SceneManager, FileSystem);
        mesh = reinterpret_cast<ISkinnedMesh*>(w3Loader.createMesh(meshFile));
        if (!mesh)
            log->addLineAndFlush(formatString("Fail to load the w2mesh file : %s", filename.c_str()));

        meshFile->drop();
    }
    return mesh;
}

// In the materials, file can be w2mi (material) or w2mg (shader)
// We check that and load the material in the case of a w2mi file
video::SMaterial IO_MeshLoader_W3ENT::ReadMaterialFile(core::stringc filename)
{
    if (core::hasFileExtension(filename, "w2mi"))
        return ReadW2MIFile(filename);
    else if (core::hasFileExtension(filename, "w2mg"))
        ; // shader, not handled
    else
        log->addLineAndFlush(formatString("Unknown type of file for a material : %s", filename.c_str()));

    video::SMaterial material;
    return material;
}

video::SMaterial IO_MeshLoader_W3ENT::ReadW2MIFile(core::stringc filename)
{
    log->addLineAndFlush(formatString("Read W2MI : %s", filename.c_str()));
    video::SMaterial material;
    io::IReadFile* matFile = FileSystem->createAndOpenFile(filename);
    if (!matFile)
    {
        log->addLineAndFlush(formatString("Fail to open the w2mi file : %s", filename.c_str()));
    }
    else
    {
        IO_MeshLoader_W3ENT w2miLoader(SceneManager, FileSystem);
        IAnimatedMesh* matMesh = nullptr;
        matMesh = w2miLoader.createMesh(matFile);
        if (matMesh)
            matMesh->drop();
        else
            log->addLineAndFlush(formatString("Fail to load the w2mi file : %s", filename.c_str()));

        // Get the material from the w2mi file loaded
        if (w2miLoader.Materials.size() == 1)
            material = w2miLoader.Materials[0];
        else if (w2miLoader.Materials.size() > 1)
            log->addLineAndFlush(formatString("%s has more than 1 material", filename.c_str()));
        else
            log->addLineAndFlush(formatString("%s has no material", filename.c_str()));

        matFile->drop();
    }

    return material;
}

video::SMaterial IO_MeshLoader_W3ENT::W3_CMaterialInstance(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);

    video::SMaterial mat;

    const s32 endOfChunk = infos.adress + infos.size;

    while (file->getPos() < endOfChunk)
    {
        log->addAndFlush("Read property...");
        SPropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
        {
            file->seek(-2, true);
            mat = ReadIMaterialProperty(file);
            return mat;
        }

        // material in a w2mi file
        if (propHeader.propName == "baseMaterial")
        {
            u32 fileId = readU32(file);
            fileId = 0xFFFFFFFF - fileId;

            log->addAndFlush("baseMat find");
            log->addLineAndFlush(formatString("base material : %s", Files[fileId].c_str()));
            mat = ReadMaterialFile(ConfigGamePath + Files[fileId]);
        }

        file->seek(propHeader.endPos);
        log->addAndFlush("Done");
    }

    return mat;
}

// Check the file format version and load the mesh if it's ok
bool IO_MeshLoader_W3ENT::load(io::IReadFile* file)
{
    readString(file, 4); // CR2W

    const s32 fileFormatVersion = readS32(file);
    log->addLineAndFlush(formatString("File format version : %d", fileFormatVersion));

    if (getTWFileFormatVersion(file) == REV_WITCHER_3)
    {
        return W3_load(file);
    }
    else
    {
        log->addLineAndFlush("Error : Incorrect file format version");
        Feedback = "\nError : Incorrect file format version";
        return false;
    }
}


video::ITexture* IO_MeshLoader_W3ENT::getTexture(io::path filename)
{
    io::path baseFilename;
    if (core::hasFileExtension(filename.c_str(), "xbm"))
    {
        core::cutFilenameExtension(baseFilename, filename);
    }
    else
        return SceneManager->getVideoDriver()->getTexture(filename);

    video::ITexture* texture = nullptr;

    // Check for textures extracted with the LUA tools
    filename = baseFilename + core::stringc(".dds");
    filename.replace("\\", "#");
    filename = ConfigGameTexturesPath + filename;

    if (FileSystem->existFile(filename))
        texture = SceneManager->getVideoDriver()->getTexture(filename);

    if (texture)
        return texture;

    // Else, if extracted with wcc_lite, we check all the possible filename
    core::array<io::path> possibleExtensions;
    possibleExtensions.push_back(".dds");
    possibleExtensions.push_back(".bmp");
    possibleExtensions.push_back(".jpg");
    possibleExtensions.push_back(".jpeg");
    possibleExtensions.push_back(".tga");
    possibleExtensions.push_back(".png");

    for (u32 i = 0; i < possibleExtensions.size(); ++i)
    {
        filename = ConfigGameTexturesPath + baseFilename + possibleExtensions[i];

        if (FileSystem->existFile(filename))
            texture = SceneManager->getVideoDriver()->getTexture(filename);

        if (texture)
            return texture;
    }


    return texture;
}

s32 IO_MeshLoader_W3ENT::getTextureLayerFromTextureType(core::stringc textureType)
{
    if (textureType == "Diffuse")
        return 0;
    else if (textureType == "Normal")
        return 1;
    else
        return -1;
}


} // end namespace scene
} // end namespace irr




#endif // _IRR_COMPILE_WITH_W3ENT_LOADER_

