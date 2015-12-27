// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#define _IRR_COMPILE_WITH_W2ENT_LOADER_
#ifdef _IRR_COMPILE_WITH_W2ENT_LOADER_

#include "CW3ENTMeshFileLoader.h"

#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "IReadFile.h"
#include "IWriteFile.h"
#include "halffloat.h"

#include <sstream>

//#define _DEBUG


template <typename T> core::stringc toStr(const T& t) {
   std::ostringstream os;
   os << t;
   return core::stringc(os.str().c_str());
}

namespace irr
{
namespace scene
{

//! Constructor
CW3ENTMeshFileLoader::CW3ENTMeshFileLoader(scene::ISceneManager* smgr, io::IFileSystem* fs)
: SceneManager(smgr), FileSystem(fs), AnimatedMesh(0), log(0)
{
	#ifdef _DEBUG
    setDebugName("CW3ENTMeshFileLoader");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CW3ENTMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{

    if (core::hasFileExtension ( filename, "w2ent_MEMORY" ))
        return true;

    irr::io::IReadFile* file = SceneManager->getFileSystem()->createAndOpenFile(filename);
    if (!file)
        return false;

    file->seek(4);

    int version = 0;
    file->read(&version, 4);

    if (version >= 162)
    {
        file->drop();
        return core::hasFileExtension ( filename, "w2ent" ) || core::hasFileExtension ( filename, "w2mesh" ) || core::hasFileExtension ( filename, "w2rig" );
    }


    file->drop();
    return false;
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CW3ENTMeshFileLoader::createMesh(io::IReadFile* f)
{
    // Create and enable the log file if the option is selected on the soft
    log = new Log(SceneManager, "debug.log");
    log->enable(SceneManager->getParameters()->getAttributeAsBool("TW_DEBUG_LOG"));
    //log->setConsoleOutput(true);
    Feedback = "";

    if (!log->works())
    {
        Feedback += "\nError : The log file can't be created\nCheck that you don't use special characters in your software path. (Unicode isn't supported)\n";
        return 0;
    }

	if (!f)
		return 0;

    #ifdef _IRR_WCHAR_FILESYSTEM
        GamePath = SceneManager->getParameters()->getAttributeAsStringW("TW_GAME_PATH");
        GameTexturesPath = GameTexturesPath = SceneManager->getParameters()->getAttributeAsStringW("TW_TW3_TEX_PATH");
    #else
        GamePath = SceneManager->getParameters()->getAttributeAsString("TW_GAME_PATH");
        GameTexturesPath = SceneManager->getParameters()->getAttributeAsString("TW_TW3_TEX_PATH");
    #endif

    //Clear up
    Strings.clear();
    Materials.clear();
    Files.clear();
    Meshes.clear();

    log->add("-> ");
    log->add(f->getFileName().c_str());
    log->add("\n-> Load Sekeleton is ");
    if (SceneManager->getParameters()->getAttributeAsBool("TW_TW3_LOAD_SKEL"))
        log->add("enabled\n");
    else
        log->add("disabled\n");

    log->add("Start loading\n");
    log->push();


    AnimatedMesh = SceneManager->createSkinnedMesh();

	if (load(f))
	{
        for (u32 i = 0; i < Meshes.size(); ++i)
        {
            combineMeshes(AnimatedMesh, Meshes[i]);
            Meshes[i]->drop();
        }

		AnimatedMesh->finalize();
        Feedback += "done";

        SceneManager->getParameters()->setAttribute("TW_FEEDBACK", Feedback.c_str());

		//SceneManager->getMeshManipulator()->recalculateNormals(AnimatedMesh);
        //SceneManager->getMeshManipulator()->flipSurfaces(AnimatedMesh);

	}
	else
	{
		AnimatedMesh->drop();
		AnimatedMesh = 0;
	}

    log->addAndPush("LOADING FINISHED\n");
    delete log;

	return AnimatedMesh;
}



core::stringc CW3ENTMeshFileLoader::readStringUntilNull(io::IReadFile* file)
{
    core::stringc returnedString;
    char c;
    while (1) {
       file->read(&c, 1);
       if (c == 0x00)
           break;
       returnedString.append(c);
    }

    return returnedString;
}

SMeshInfos CW3ENTMeshFileLoader::createSMeshInfos()
{
    SMeshInfos infos;

    infos.firstIndice = 0;
    infos.firstVertex = 0;
    infos.numBonesPerVertex = 4;

    infos.vertexType = EMVT_STATIC;
    infos.numIndices = 0;
    infos.numVertices = 0;

    infos.materialID = 0;

    return infos;
}

SBufferInfos CW3ENTMeshFileLoader::createSBufferInfo()
{
    SBufferInfos bufferInfos;

    bufferInfos.indicesBufferOffset = 0;
    bufferInfos.indicesBufferSize = 0;
    bufferInfos.quantizationOffset = core::vector3df(0, 0, 0);
    bufferInfos.quantizationScale = core::vector3df(1, 1, 1);
    bufferInfos.verticesBufferOffset = 0;
    bufferInfos.verticesBufferSize = 0;

    return bufferInfos;
}

void checkMaterial(video::SMaterial mat)
{
    if (mat.getTexture(0))
        ;//std::cout << "SLOT 1 = " <<mat.getTexture(0)->getName().getPath().c_str() << std::endl;
    else
        ;//std::cout << "Le material n'a pas de tex slot 1" << std::endl;
}

bool CW3ENTMeshFileLoader::W3_load(io::IReadFile* file)
{
    file->seek(0);

    readWord(file, 4); // CR2W

    int fileFormatVersion;
    file->read(&fileFormatVersion, 4);
    file->read(&fileFormatVersion, 4);

    core::array<int> headerData = readInts(file, 38);
    log->addAndPush("Read header\n");

    /*
        The header :
        - data[7/8]   : adress/size string chunk
        - data[10/11] : adress/size content chunk
    */

    int stringChunkStart = headerData[7];
    int stringChunkSize = headerData[8];
    file->seek(stringChunkStart);
    while (file->getPos() - stringChunkStart < stringChunkSize)
    {
        core::stringc str = readStringUntilNull(file);
        Strings.push_back(str);
        //std::cout << str.c_str() << std::endl;
    }
    log->addAndPush("Read strings\n");

    // The files linked in the file
    for (u32 i = 0; i < Strings.size(); ++i)
        if (Strings[i].findFirst('.') != -1)
        {
            Files.push_back(Strings[i]);
            //std::cout << Files.size() - 1 << "--> " << Strings[i].c_str() << std::endl;
        }
    log->addAndPush("Textures list created\n");


    int contentChunkStart = headerData[19];
    int contentChunkSize = headerData[20];

    core::array<W3_DataInfos> meshes;
    file->seek(contentChunkStart);
    for (int i = 0; i < contentChunkSize; ++i)
    {
        W3_DataInfos infos;
        unsigned short dataType = readUnsignedShorts(file, 1)[0];
        core::stringc dataTypeName = Strings[dataType];
        log->addAndPush(core::stringc("dataTypeName=") + dataTypeName + "\n");

        file->seek(6, true);

        file->read(&infos.size, 4);
        file->read(&infos.adress, 4);
        //std::cout << "begin at " << infos.adress << " and end at " << infos.adress + infos.size << std::endl;

        file->seek(8, true);

        int back = file->getPos();
        if (dataTypeName == "CMesh")
        {
            meshes.push_back(infos);
            //W3_CMesh(file, infos);
            log->addAndPush("Find a mesh\n");
        }
        if (dataTypeName == "CMaterialInstance")
        {
            log->addAndPush("Find a material\n");
            video::SMaterial mat = W3_CMaterialInstance(file, infos);
            //checkMaterial(mat);
            log->addAndPush("Material loaded\n");
            Materials.push_back(mat);
            log->addAndPush("Added to mat list\n");
        }
        if (dataTypeName == "CEntityTemplate")
        {
            W3_CEntityTemplate(file, infos);
        }
        if (dataTypeName == "CEntity")
        {
            W3_CEntity(file, infos);
        }
        if (dataTypeName == "CMeshComponent")
        {
            W3_CMeshComponent(file, infos);
        }
        if (dataTypeName == "CSkeleton")
        {
            W3_CSkeleton(file, infos);
        }
        if (dataTypeName == "CMimicFace")
        {
            W3_CMimicFace(file, infos);
        }
        file->seek(back);
    }

    for (u32 i = 0; i < meshes.size(); ++i)
    {
        log->addAndPush("Load mesh...");
        W3_CMesh(file, meshes[i]);
        log->addAndPush("done\n");
    }

    return true;
}



void CW3ENTMeshFileLoader::W3_ReadBuffer(io::IReadFile* file, SBufferInfos bufferInfos, SMeshInfos meshInfos)
{
    scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
    buffer->VertexType = video::EVT_STANDARD;

    io::IReadFile* bufferFile = FileSystem->createAndOpenFile(file->getFileName() + ".1.buffer");
    if (!bufferFile)
    {
        //std::cout << "Fail to open buffer file" << std::endl;
        Feedback += "\nThe .buffer file associated to the mesh hasn't been found.\nHave you extracted the necessary bundle ?\n";
        log->addAndPush(" fail to open .buffer file ");
        return;
    }

    //std::cout << "Num vertices=" << meshInfos.numVertices << std::endl;
    buffer->Vertices_Standard.reallocate(meshInfos.numVertices);
    buffer->Vertices_Standard.set_used(meshInfos.numVertices);

    u32 vertexSize = 20;
    if (meshInfos.vertexType == EMVT_SKINNED)
        vertexSize += meshInfos.numBonesPerVertex * 2;

    //std::cout << "first vertex = " << meshInfos.firstVertex << std::endl;

    SVertexBufferInfos* inf = 0;
    u32 sum = 0;
    for (u32 i = 0; i < bufferInfos.verticesBuffer.size(); ++i)
    {
        sum += bufferInfos.verticesBuffer[i].nbVertices;
        if (sum > meshInfos.firstVertex)
        {
            inf = &bufferInfos.verticesBuffer[i];
            break;
        }

    }
    bufferFile->seek(inf->verticesCoordsOffset + (meshInfos.firstVertex - (sum - inf->nbVertices)) * vertexSize);
    //std::cout << "POS=" << bufferFile->getPos() << std::endl;
    for (u32 i = 0; i < meshInfos.numVertices; ++i)
    {
        unsigned short x, y, z, tmp;

        bufferFile->read(&x, 2);
        bufferFile->read(&y, 2);
        bufferFile->read(&z, 2);
        bufferFile->read(&tmp, 2);

        // skip skinning data
        if (meshInfos.vertexType == EMVT_SKINNED && !SceneManager->getParameters()->getAttributeAsBool("TW_TW3_LOAD_SKEL"))
        {
            bufferFile->seek(meshInfos.numBonesPerVertex * 2, true);
        }
        else if (meshInfos.vertexType == EMVT_SKINNED)
        {
            //bufferFile->seek(meshInfos.numBonesPerVertex * 2, true);

            unsigned char skinningData[meshInfos.numBonesPerVertex * 2];
            bufferFile->read(&skinningData[0], meshInfos.numBonesPerVertex * 2);

            for (u32 j = 0; j < meshInfos.numBonesPerVertex; ++j)
            {
                unsigned char boneId = skinningData[j];
                unsigned char weight = skinningData[j + meshInfos.numBonesPerVertex];

                if (boneId >= AnimatedMesh->getJointCount()) // If bone don't exist
                    continue;

                if (weight != 0)
                {
                    ISkinnedMesh::SJoint* joint = AnimatedMesh->getAllJoints()[boneId];
                    u32 bufferId = AnimatedMesh->getMeshBufferCount() - 1;
                    float fweight = (float)weight / 255.f;

                    ISkinnedMesh::SWeight* w = AnimatedMesh->addWeight(joint);
                    w->buffer_id = bufferId;
                    w->strength = fweight;
                    w->vertex_id = i;
                    //std::cout << "TEST:" << fweight << ", " << bufferId << ", " << i << std::endl;
                }
            }

        }

        //std::cout << "Position=" << x * bufferInfos.quantizationScale.X / 30000<< ", " << y * bufferInfos.quantizationScale.Y / 30000<< ", " << bufferInfos.quantizationScale.Z * z / 30000<< std::endl;
        buffer->Vertices_Standard[i].Pos = core::vector3df(x, y, z) / 65535.f * bufferInfos.quantizationScale + bufferInfos.quantizationOffset;
        buffer->Vertices_Standard[i].Color = video::SColor(255, 255, 255, 255);
    }
    bufferFile->seek(inf->uvOffset + (meshInfos.firstVertex - (sum - inf->nbVertices)) * 4);
    //std::cout << "avant UV=" << bufferFile->getPos() << std::endl;
    //bufferFile->seek(8, true);

    for (u32 i = 0; i < meshInfos.numVertices; ++i)
    {
        unsigned short u, v;
        bufferFile->read(&u, 2);
        bufferFile->read(&v, 2);

        f32 uf = halfToFloat(u);
        f32 vf = halfToFloat(v);

        //std::cout << "UV = " << uf << ", " << vf << std::endl;
        buffer->Vertices_Standard[i].TCoords = core::vector2df(uf, vf);
    }
    // Not 100% sure...
    bufferFile->seek(inf->normalsOffset + (meshInfos.firstVertex - (sum - inf->nbVertices)) * 12);
    for (u32 i = 0; i < meshInfos.numVertices; ++i)
    {
        unsigned short x, y, z, tmp;

        bufferFile->read(&x, 2);
        bufferFile->read(&y, 2);
        bufferFile->read(&z, 2);
        bufferFile->read(&tmp, 2);

        //std::cout << "Position=" << x * infos.quantizationScale.X<< ", " << y * infos.quantizationScale.Y<< ", " << z << std::endl;
        buffer->Vertices_Standard[i].Normal = core::vector3df(x, y, z) / 65535.f * bufferInfos.quantizationScale + bufferInfos.quantizationOffset;
    }

    // Indices -------------------------------------------------------------------
    bufferFile->seek(bufferInfos.indicesBufferOffset + meshInfos.firstIndice * 2);

    buffer->Indices.reallocate(meshInfos.numIndices);
    buffer->Indices.set_used(meshInfos.numIndices);

    //std::cout << "offset=" << meshInfos.firstIndice << std::endl;
    //std::cout << "num indices=" << meshInfos.numIndices << std::endl;
    for (u32 i = 0; i < meshInfos.numIndices; ++i)
    {
        unsigned short indice;
        bufferFile->read(&indice, 2);

        // Indice need to be inversed for the normals
        //buffer->Indices[i] = indice;

        if (i % 3 == 0)
            buffer->Indices[i] = indice;
        else if (i % 3 == 1)
            buffer->Indices[i+1] = indice;
        else if (i % 3 == 2)
            buffer->Indices[i-1] = indice;
    }

    buffer->setDirty();
    SceneManager->getMeshManipulator()->recalculateNormals(buffer);

    bufferFile->drop();
}

u32 CW3ENTMeshFileLoader::ReadUInt32Property(io::IReadFile* file)
{
    int sizeToGoToNext;
    u32 value;
    file->read(&sizeToGoToNext, 4);
    file->read(&value, 4);

    //std::cout << "Value = " << value << std::endl;
    return value;
}

char CW3ENTMeshFileLoader::ReadUInt8Property(io::IReadFile* file)
{
    int sizeToGoToNext;
    file->read(&sizeToGoToNext, 4);

    char value;
    file->read(&value, 1);

    //std::cout << "Value = " << value << std::endl;
    return value;
}

bool CW3ENTMeshFileLoader::ReadBoolProperty(io::IReadFile* file)
{
    int sizeToGoToNext;
    file->read(&sizeToGoToNext, 4);

    char valueChar;
    file->read(&valueChar, 1);
    bool value = (valueChar == 0) ? false : true;
    return value;
}

float CW3ENTMeshFileLoader::ReadFloatProperty(io::IReadFile* file)
{
    int sizeToGoToNext;
    file->read(&sizeToGoToNext, 4);

    float value;
    file->read(&value, 4);
    return value;
}

core::vector3df CW3ENTMeshFileLoader::ReadVector3Property(io::IReadFile* file)
{
    int sizeToGoNext;
    file->read(&sizeToGoNext, 4);

    core::vector3df value;
    file->seek(1, true);

    file->seek(4, true);    // 2 index of the Strings table (Name + type -> X, Float)
    value.X = ReadFloatProperty(file);
    file->seek(4, true);
    value.Y = ReadFloatProperty(file);
    file->seek(4, true);
    value.Z = ReadFloatProperty(file);

    file->seek(-41, true);
    file->seek(sizeToGoNext, true);

    return value;
}

void CW3ENTMeshFileLoader::ReadUnknowProperty(io::IReadFile* file)
{
    int sizeToGoToNext;
    file->read(&sizeToGoToNext, 4);
    sizeToGoToNext -= 4;
    file->seek(sizeToGoToNext, true);
}

void CW3ENTMeshFileLoader::ReadMaterialsProperty(io::IReadFile* file)
{
    int back = file->getPos();

    int sizeToGoNext, nbChunks;
    file->read(&sizeToGoNext, 4);

    file->read(&nbChunks, 4);
    //std::cout << "NB material = -> " << nbChunks << std::endl;

    //file->seek(1, true);

    core::array<video::SMaterial> matMats;

    int nb = 0;
    for (u32 i = 0; i < nbChunks; ++i)
    {

        unsigned char matFileID;
        file->read(&matFileID, 1);
        matFileID = 255 - matFileID;

        if (matFileID > Files.size())
        {
            file->seek(-1, true);
            u32 value;
            file->read(&value, 4);
            //std::cout << "val = " << value << std::endl;
            //Materials.push_back(Materials[value-1]);
        }
        else
        {
            //std::cout << "w2mi file = " << Files[matFileID].c_str() << std::endl;
            matMats.push_back(ReadW2MIFile(Files[matFileID]));
            file->seek(3, true);
        }

    }
    for (u32 i = 0; i < matMats.size(); ++i)
    {
        Materials.push_front(matMats[matMats.size() - 1 - i]);
    }
    file->seek(back + sizeToGoNext);
}

EMeshVertexType CW3ENTMeshFileLoader::ReadEMVTProperty(io::IReadFile* file)
{
    int sizeToGoToNext;
    file->read(&sizeToGoToNext, 4);

    unsigned short enumStringId;
    file->read(&enumStringId, 2);

    EMeshVertexType vertexType = EMVT_STATIC;

    core::stringc enumString = Strings[enumStringId];
    if (enumString == "MVT_SkinnedMesh")
    {
        vertexType = EMVT_SKINNED;
    }

    return vertexType;
}

core::array<SMeshInfos> CW3ENTMeshFileLoader::ReadSMeshChunkPackedProperty(io::IReadFile* file)
{
    core::array<SMeshInfos> meshes;
    SMeshInfos meshInfos = createSMeshInfos();

    int back = file->getPos();

    int sizeToGoNext, nbChunks;
    file->read(&sizeToGoNext, 4);

    file->read(&nbChunks, 4);
    //std::cout << "NB = -> " << nbChunks << std::endl;

    file->seek(1, true);

    int chunkId = 0;

    while(1)
    {
        unsigned short propertyID, propertyTypeID;
        file->read(&propertyID, 2);
        file->read(&propertyTypeID, 2);

        if (propertyID == 0 || propertyTypeID == 0 || propertyID > Strings.size() || propertyTypeID > Strings.size())
        {
            meshes.push_back(meshInfos);
            chunkId++;

            if (chunkId >= nbChunks)
                break;
            else
            {
                SMeshInfos newMeshInfos = createSMeshInfos();
                newMeshInfos.vertexType = meshInfos.vertexType;
                newMeshInfos.numBonesPerVertex = meshInfos.numBonesPerVertex;
                meshInfos = newMeshInfos;

                file->seek(-1, true);

                file->read(&propertyID, 2);
                file->read(&propertyTypeID, 2);
            }
        }

        core::stringc property = Strings[propertyID];
        core::stringc propertyType = Strings[propertyTypeID];


        //std::cout << "@" << file->getPos() <<", property = " << property.c_str() << ", type = " << propertyType.c_str() << std::endl;

        if (property == "numIndices")
        {
            meshInfos.numIndices = ReadUInt32Property(file);
            //std::cout << "numIndices = " << meshInfos.numIndices << std::endl;
        }
        else if (property == "numVertices")
        {
            meshInfos.numVertices = ReadUInt32Property(file);
            //std::cout << "numVertices = " << meshInfos.numVertices << std::endl;
        }
        else if (property == "firstVertex")
        {
            meshInfos.firstVertex = ReadUInt32Property(file);
            //std::cout << "first vertex found (=" << meshInfos.firstVertex << ")" << std::endl;
        }
        else if (property == "firstIndex")
        {
            meshInfos.firstIndice = ReadUInt32Property(file);
            //std::cout << "firstIndice = " << meshInfos.firstIndice << std::endl;
        }
        else if (property == "vertexType")
        {
            meshInfos.vertexType = ReadEMVTProperty(file);
        }
        else if (property == "numBonesPerVertex")
        {
            meshInfos.numBonesPerVertex = ReadUInt8Property(file);
        }
        else if (property == "materialID")
        {
            meshInfos.materialID = ReadUInt32Property(file);
            //std::cout << "material ID = " << meshInfos.materialID << std::endl;
        }
        else
            ReadUnknowProperty(file);
    }

    file->seek(back + sizeToGoNext);


    return meshes;
}

void CW3ENTMeshFileLoader::ReadRenderChunksProperty(io::IReadFile* file, SBufferInfos* buffer)
{
    int back = file->getPos();

    int sizeToGoToNext;
    file->read(&sizeToGoToNext, 4);


    file->seek(5, true);

    char nbBuffers;
    file->read(&nbBuffers, 1);

    //for (u32 i = 0; i < nbBuffers; ++i)
    while(file->getPos() - back < sizeToGoToNext)
    {
        SVertexBufferInfos buffInfos;
        file->read(&buffInfos.verticesCoordsOffset, 4);
        file->read(&buffInfos.uvOffset, 4);
        file->read(&buffInfos.normalsOffset, 4);

        //std::cout << "adresses = " << buffInfos.verticesCoordsOffset << ", " << buffInfos.uvOffset << ", " << buffInfos.normalsOffset << std::endl;

        file->seek(14, true);

        file->read(&buffInfos.nbVertices, 2);
        //std::cout << "Nb VERT=" << buffInfos.nbVertices << std::endl;

        file->seek(9, true);

        buffer->verticesBuffer.push_back(buffInfos);
    }
    file->seek(back + sizeToGoToNext);
}

video::SMaterial CW3ENTMeshFileLoader::ReadIMaterialProperty(io::IReadFile* file)
{
    log->addAndPush("IMaterial\n");
    video::SMaterial mat;
    mat.MaterialType = video::EMT_SOLID;

    int nbProperty;
    file->read(&nbProperty, 4);
    //std::cout << "nb property = " << nbProperty << std::endl;
    //std::cout << "adress = " << file->getPos() << std::endl;

    // Read the properties of the material
    for (u32 i = 0; i < nbProperty; ++i)
    {
        log->addAndPush("property...");
        const int back = file->getPos();

        int sizeToGoToNext;
        file->read(&sizeToGoToNext, 4);

        unsigned short propId, propTypeId;
        file->read(&propId, 2);
        file->read(&propTypeId, 2);

        if (propId >= Strings.size())
            break;

        //std::cout << "The property is " << Strings[propId].c_str() << " of the type " << Strings[propTypeId].c_str() << std::endl;

        const int textureLayer = getTextureLayerFromTextureType(Strings[propId]);
        if (textureLayer != -1)
        {

            unsigned char texId;
            file->read(&texId, 1);
            texId = 255 - texId;

            if (texId < Files.size())
            {
                video::ITexture* texture = 0;
                texture = getTexture(Files[texId]);

                if (texture)
                {
                    log->addAndPush(core::stringc(" ") + Strings[propId].c_str() + " ");
                    mat.setTexture(textureLayer, texture);

                    if (textureLayer == 1)  // normal map
                        mat.MaterialType = video::EMT_NORMAL_MAP_SOLID;
                }
                else
                {
                    Feedback += "Some textures havn't been found, have you correctly set your textures directory ?\n";
                    log->addAndPush(core::stringc("Error : the file ") + Files[texId] + core::stringc(" can't be opened.\n"));
                }
            }
        }

        file->seek(back + sizeToGoToNext);
        log->addAndPush("OK\n");
    }

    log->addAndPush("IMaterial OK\n");
    //checkMaterial(mat);
    return mat;
}

core::array<core::vector3df> CW3ENTMeshFileLoader::ReadBonesPosition(io::IReadFile* file)
{
    int back = file->getPos();

    int sizeToGoNext, nbBones;
    file->read(&sizeToGoNext, 4);

    file->read(&nbBones, 4);
    file->seek(1, true);

    core::array<core::vector3df> positions;
    for (u32 i = 0; i < nbBones; ++i)
    {
        file->seek(4, true);
        float x = ReadFloatProperty(file);
        file->seek(4, true);
        float y = ReadFloatProperty(file);
        file->seek(4, true);
        float z = ReadFloatProperty(file);
        file->seek(4, true);
        float w = ReadFloatProperty(file);

        core::vector3df position = core::vector3df(x, y, z);
        positions.push_back(position);

        //std::cout << "position = " << x << ", " << y << ", " << z << ", " << w << std::endl;
        file->seek(3, true);
    }
    file->seek(back + sizeToGoNext);
    return positions;
}

SBufferInfos CW3ENTMeshFileLoader::ReadSMeshCookedDataProperty(io::IReadFile* file)
{
    SBufferInfos bufferInfos = createSBufferInfo();

    int back = file->getPos();

    int sizeToGoNext;
    file->read(&sizeToGoNext, 4);

    file->seek(1, true);

    while(1)
    {
        unsigned short propertyID, propertyTypeID;
        file->read(&propertyID, 2);
        file->read(&propertyTypeID, 2);

        if (propertyID == 0 || propertyTypeID == 0 || propertyID > Strings.size() || propertyTypeID > Strings.size())
        {
            break;
        }

        //std::cout << "@" << file->getPos() <<", property = " << Strings[propertyID].c_str() << ", type = " << Strings[propertyTypeID].c_str() << std::endl;

        core::stringc property = Strings[propertyID];

        if (property == "indexBufferSize")
        {
            bufferInfos.indicesBufferSize = ReadUInt32Property(file);
        }
        else if (property == "indexBufferOffset")
        {
            bufferInfos.indicesBufferOffset = ReadUInt32Property(file);
        }
        else if (property == "vertexBufferSize")
        {
            bufferInfos.verticesBufferSize = ReadUInt32Property(file);
        }
        else if (property == "quantizationScale")
        {
            bufferInfos.quantizationScale = ReadVector3Property(file);
        }
        else if (property == "quantizationOffset")
        {
            bufferInfos.quantizationOffset = ReadVector3Property(file);
        }
        else if (property == "bonePositions")
        {
            core::array<core::vector3df> positions = ReadBonesPosition(file);
            nbBonesPos = positions.size();
        }
        //else if (property == "collisionInitPositionOffset")
        //    ReadVector3Property(file, &bufferInfos);
        else if (property == "renderChunks")
                ReadRenderChunksProperty(file, &bufferInfos);
        else
            ReadUnknowProperty(file);
    }

    file->seek(back + sizeToGoNext);

    return bufferInfos;
}


CSkeleton CW3ENTMeshFileLoader::W3_CMimicFace(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    //std::cout << "W3_CMimicFace, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;
    log->addAndPush("W3_CMimicFace\n");

    CSkeleton skeleton;

    while (1)
    {
        unsigned short propertyID, propertyTypeID;
        file->read(&propertyID, 2);
        file->read(&propertyTypeID, 2);

        if (propertyID == 0 || propertyTypeID == 0 || propertyID > Strings.size() || propertyTypeID > Strings.size())
            break;

        core::stringc property = Strings[propertyID];
        core::stringc propertyType = Strings[propertyTypeID];

        //std::cout << "-> @" << file->getPos() <<", property = " << property.c_str() << ", type = " << propertyType.c_str() << std::endl;
        ReadUnknowProperty(file);
    }
    log->addAndPush("W3_CMimicFace end\n");
    return skeleton;
}

CSkeleton CW3ENTMeshFileLoader::W3_CSkeleton(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    //std::cout << "W3_CSkeleton, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;
    log->addAndPush("W3_CSkeleton\n");

    CSkeleton skeleton;

    while (1)
    {
        unsigned short propertyID, propertyTypeID;
        file->read(&propertyID, 2);
        file->read(&propertyTypeID, 2);

        if (propertyID == 0 || propertyTypeID == 0 || propertyID > Strings.size() || propertyTypeID > Strings.size())
            break;

        core::stringc property = Strings[propertyID];
        core::stringc propertyType = Strings[propertyTypeID];

        std::cout << "-> @" << file->getPos() <<", property = " << property.c_str() << ", type = " << propertyType.c_str() << std::endl;

        if (property == "bones")
        {
            const int back = file->getPos();

            int sizeToNext, nbBones;
            file->read(&sizeToNext, 4);
            file->read(&nbBones, 4);
            file->seek(1, true);

            skeleton.nbBones = nbBones;

            for (int i = 0; i < nbBones; ++i)
            {
                file->seek(4, true);    // refer to string table : name + StringANSI
                int size;
                file->read(&size, 4);
                file->seek(1, true); // a char with word size

                size -= 5; // 5 characters readed

                core::stringc name = readWord(file, size);
                skeleton.names.push_back(name);

                std::cout << "name=" << name.c_str() << std::endl;

                file->seek(13, true); // nameAsCName + CName + size + CName string ID + 3 0x00 octets
            }

            file->seek(back + sizeToNext);


        }
        else if (property == "parentIndices")
        {
            int sizeToNext, nbBones;
            file->read(&sizeToNext, 4);

            //std::cout << "end supposed to be at " << file->getPos() + sizeToNext - 4 << std::endl;
            file->read(&nbBones, 4);
            for (int i = 0; i < nbBones; ++i)
            {
                short parentId;
                file->read(&parentId, 2);
                //std::cout << "parent ID=" << parentId << std::endl;

                skeleton.parentId.push_back(parentId);
            }

        }
        else
            ReadUnknowProperty(file);
    }

    // Now there are the matrix
    file->seek(-2, true);
    std::cout << file->getPos() << std::endl;

    //std::cout << "read the matrix" << std::endl;
    for (u32 i = 0; i < skeleton.nbBones; ++i)
    {
        core::matrix4 mat(core::matrix4::EM4CONST_IDENTITY);
        u32 m = 0;
        for (u32 j = 0; j < 12; ++j)
        {
            if (m == 3 || m == 7 || m == 11)
                m++;

            float value;
            file->read(&value, 4);
            //mat[m] = value;
            //std::cout << value << std::endl;
            m++;
        }
        skeleton.matrix.push_back(mat);
    }

    //std::cout << "end read the matrix" << std::endl;
    Skeleton = skeleton;

    log->addAndPush("W3_CSkeleton end\n");
    return skeleton;
}

void CW3ENTMeshFileLoader::W3_CMeshComponent(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    //std::cout << "W3_CMeshComponent, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;
    log->addAndPush("W3_CMeshComponent\n");

    while (1)
    {
        core::array<unsigned short> propertyData = readUnsignedShorts(file, 2);
        unsigned short propertyID = propertyData[0];
        unsigned short propertyTypeID = propertyData[1];

        if (propertyID == 0 || propertyTypeID == 0 || propertyID > Strings.size() || propertyTypeID > Strings.size())
            break;

        core::stringc property = Strings[propertyID];
        core::stringc propertyType = Strings[propertyTypeID];

        //std::cout << "-> @" << file->getPos() <<", property = " << property.c_str() << ", type = " << propertyType.c_str() << std::endl;

        if (property == "mesh")
        {
            file->seek(4, true); // size to skip
            char fileId;
            file->read(&fileId, 1);
            fileId = 255 - fileId;
            file->seek(3, true);
            scene::ISkinnedMesh* mesh = ReadW2MESHFile(GamePath + Files[fileId]);
            Meshes.push_back(mesh);
            //AnimatedMesh = ReadW2MESHFile(GamePath + Files[fileId]);
        }
        else
            ReadUnknowProperty(file);
    }

    log->addAndPush("W3_CMeshComponent end\n");
}

void CW3ENTMeshFileLoader::W3_CEntityTemplate(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    log->addAndPush("W3_CEntityTemplate\n");
    //std::cout << "W3_CEntityTemplate, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;


    while (1)
    {
        core::array<unsigned short> propertyData = readUnsignedShorts(file, 2);
        unsigned short propertyID = propertyData[0];
        unsigned short propertyTypeID = propertyData[1];

        if (propertyID == 0 || propertyTypeID == 0 || propertyID > Strings.size() || propertyTypeID > Strings.size())
            break;

        core::stringc property = Strings[propertyID];
        core::stringc propertyType = Strings[propertyTypeID];

        //std::cout << "-> @" << file->getPos() <<", property = " << property.c_str() << ", type = " << propertyType.c_str() << std::endl;

        if (property == "flatCompiledData")
        {
            s32 sizeToNext;
            file->seek(4, true);
            file->read(&sizeToNext, 4);
            sizeToNext -= 4;


            //std::cout << file->getPos() << std::endl;

            unsigned char data[sizeToNext];
            file->read(&data[0], sizeToNext);


            io::IReadFile* entityFile = SceneManager->getFileSystem()->createMemoryReadFile(data, sizeToNext, "tmpMemFile.w2ent_MEMORY", true);
            if (!entityFile)
                log->addAndPush("fail\n");
            //SceneManager->getMesh(entityFile);
            CW3ENTMeshFileLoader w3Loader(SceneManager, FileSystem);
            IAnimatedMesh* m = w3Loader.createMesh(entityFile);
            if (m)
                m->drop();

            entityFile->drop();
        }

        ReadUnknowProperty(file);
    }
    log->addAndPush("W3_CEntityTemplate end\n");
}

void CW3ENTMeshFileLoader::W3_CEntity(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);
    //std::cout << "W3_CEntity, @infos.adress=" << infos.adress << ", end @" << infos.adress + infos.size << std::endl;
}

bool CW3ENTMeshFileLoader::checkBones(io::IReadFile* file, char nbBones)
{
    const long back = file->getPos();
    for (char i = 0; i < nbBones; ++i)
    {
        unsigned short jointName;
        file->read(&jointName, 2);
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
    char nbBones;
    file->read(&nbBones, 1);

    char o;
    file->read(&o, 1);
    if (o != 1)
        file->seek(-1, true);

    return nbBones;
}

void CW3ENTMeshFileLoader::W3_CMesh(io::IReadFile* file, W3_DataInfos infos)
{
    log->addAndPush("W3_CMesh\n");

    SBufferInfos bufferInfos = createSBufferInfo();
    core::array<SMeshInfos> meshes;

    bool isStatic = false;

    file->seek(infos.adress + 1);

    while (1)
    {
        core::array<unsigned short> propertyData = readUnsignedShorts(file, 2);
        unsigned short propertyID = propertyData[0];
        unsigned short propertyTypeID = propertyData[1];

        if (propertyID == 0 || propertyTypeID == 0 || propertyID > Strings.size() || propertyTypeID > Strings.size())
            break;

        //std::cout << "-> @" << file->getPos() <<", property = " << Strings[propertyID].c_str() << ", type = " << Strings[propertyTypeID].c_str() << std::endl;

        core::stringc property = Strings[propertyID];
        core::stringc propertyType = Strings[propertyTypeID];


        if (propertyType == "SMeshCookedData")
        {
            log->addAndPush("Buffer infos\n");
            bufferInfos = ReadSMeshCookedDataProperty(file);
        }
        else if (property == "chunks")
        {
            log->addAndPush("Chunks\n");
            meshes = ReadSMeshChunkPackedProperty(file);
        }
        else if (property == "materials")
        {
            log->addAndPush("Mats\n");
            ReadMaterialsProperty(file);
        }
        else if (property == "isStatic")
        {
            isStatic = ReadBoolProperty(file);
        }
        else
            ReadUnknowProperty(file);
   }

   log->addAndPush(core::stringc("All properties readed, @=") + toStr(file->getPos()) + "\n");

   if (!isStatic && nbBonesPos > 0 && SceneManager->getParameters()->getAttributeAsBool("TW_TW3_LOAD_SKEL"))
   {
        ReadBones(file);
   }

   for (u32 i = 0; i < meshes.size(); ++i)
   {
        log->addAndPush("Read buffer...");
        W3_ReadBuffer(file, bufferInfos, meshes[i]);
        //std::cout << "Read a buffer, Material ID = "  << meshes[i].materialID << std::endl;

        if (meshes[i].materialID < Materials.size())
        {
            //std::cout << "Material assigned to meshbuffer" << std::endl;
            AnimatedMesh->getMeshBuffer(i)->getMaterial() = Materials[meshes[i].materialID];
        }
        else
        {
            //std::cout << "Error, mat " << meshes[i].materialID << "doesn't exist" << std::endl;
            /*
            if (Materials.size() >= 1)
                AnimatedMesh->getMeshBuffer(i)->getMaterial() = Materials[0];
            */
        }
        log->addAndPush("OK\n");
   }
    log->addAndPush("W3_CMesh end\n");
}

void CW3ENTMeshFileLoader::ReadBones(io::IReadFile* file)
{
    log->addAndPush("Load bones\n");

    // cancel property
    file->seek(-4, true);

    /*
    file->seek(2, true);
    char offsetInd;
    file->read(&offsetInd, 1);
    file->seek(offsetInd * 7, true);
    */


    // TODO
    //log->addAndPush("NbBonesPos = ");
    log->addAndPush(core::stringc("NbBonesPos = ") + toStr(nbBonesPos) + "\n");
    long pos;
    unsigned char nbRead;
    do
    {
        pos = file->getPos();
        nbRead = readBonesNumber(file);

        if (nbRead == nbBonesPos)
        {
            if (!checkBones(file, nbRead))
            {
                nbRead = -1;
            }
        }

        if (file->getPos() >= file->getSize())
            return;
    }   while (nbRead != nbBonesPos);

    file->seek(pos);


    // Name of the bones
    char nbBones = readBonesNumber(file);

    //log->addAndPush("nbBones = " + (int)nbBones);
    //std::cout << "m size= " << meshes.size() << std::endl;

    for (char i = 0; i < nbBones; ++i)
    {
        unsigned short jointName;
        file->read(&jointName, 2);
        log->addAndPush(core::stringc("string id = ") + toStr(jointName) + "\n");

        scene::ISkinnedMesh::SJoint* joint = 0;
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
            float value;
            file->read(&value, 4);

            matrix[j] = value;
        }



        core::matrix4 matr = matrix;


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
            joint->LocalMatrix = joint->GlobalMatrix;


            joint->Animatedposition = joint->LocalMatrix.getTranslation();
            joint->Animatedrotation = joint->LocalMatrix.getRotationDegrees();
            joint->Animatedscale = joint->LocalMatrix.getScale();
        }
    }

    // 1 float per bone ???
     readBonesNumber(file);
     for (char i = 0; i < nbBones; ++i)
     {
         float value;
         file->read(&value, 4);   // ??
         log->addAndPush(core::stringc("value = ") + toStr(value) + "\n");
     }

     // 1 int par bone. parent ID ? no
     readBonesNumber(file);
     for (char i = 0; i < nbBones; ++i)
     {
         u32 parent;
         file->read(&parent, 4);
         //std::cout << "= " << joints[parent]->Name.c_str() << "->" << joints[i]->Name.c_str() << std::endl;
     }
     log->addAndPush("Bones loaded\n");
}


ISkinnedMesh* CW3ENTMeshFileLoader::ReadW2MESHFile(core::stringc filename)
{
    io::IReadFile* meshFile = FileSystem->createAndOpenFile(filename);
    if (!meshFile)
    {
        log->addAndPush("FAIL TO OPEN THE W2MESH FILE\n");
        return 0;
    }

    CW3ENTMeshFileLoader w3Loader(SceneManager, FileSystem);
    IAnimatedMesh* mesh = 0;
    mesh = w3Loader.createMesh(meshFile);
    if (mesh == 0)
        log->addAndPush("Fail to load MatMesh\n");

    meshFile->drop();
    return (ISkinnedMesh*)mesh;
    //return (ISkinnedMesh*)SceneManager->getMesh(filename);
}

video::SMaterial CW3ENTMeshFileLoader::ReadW2MIFile(core::stringc filename)
{
    io::IReadFile* matFile = FileSystem->createAndOpenFile(filename);
    if (!matFile)
    {
        log->addAndPush("FAIL TO OPEN THE W2MI FILE\n");
        return video::SMaterial();
        //break;
    }

    CW3ENTMeshFileLoader w2miLoader(SceneManager, FileSystem);
    IAnimatedMesh* matMesh = 0;
    matMesh = w2miLoader.createMesh(matFile);
    if (matMesh == 0)
        log->addAndPush("Fail to load MatMesh\n");
    else
        matMesh->drop();


    video::SMaterial mat = w2miLoader.Materials[0];
    matFile->drop();

    return mat;
}

video::SMaterial CW3ENTMeshFileLoader::W3_CMaterialInstance(io::IReadFile* file, W3_DataInfos infos)
{
    file->seek(infos.adress + 1);

    video::SMaterial mat;

    const int endOfChunk = infos.adress + infos.size;

    while (file->getPos() < endOfChunk)
    {
        log->addAndPush("Read property...");
        core::array<unsigned short> propertyData = readUnsignedShorts(file, 2);
        unsigned short propertyID = propertyData[0];
        unsigned short propertyTypeID = propertyData[1];

        if (propertyID == 0 || propertyTypeID == 0 || propertyID > Strings.size() || propertyTypeID > Strings.size())
        {
            file->seek(-2, true);
            mat = ReadIMaterialProperty(file);
            return mat;
        }

        core::stringc property = Strings[propertyID];
        core::stringc propertyType = Strings[propertyTypeID];

        //std::cout << "@" << file->getPos() <<", property = " << property.c_str() << ", type = " << propertyType.c_str() << std::endl;


        if (property == "baseMaterial")
        {
            file->seek(4, true);

            unsigned char fileId;
            file->read(&fileId, 1);
            fileId = 255 - fileId;

            file->seek(3, true);

            log->addAndPush("baseMat find");

            //std::cout << "MATERIAL FILE IS " << Files[fileId].c_str() << std::endl;

            if (core::hasFileExtension(Files[fileId], "w2mi"))
            {
                mat = ReadW2MIFile(GamePath + Files[fileId]);
                return mat;
            }
        }
        else
            ReadUnknowProperty(file);

        log->addAndPush("Done");
    }
    //checkMaterial(mat);
    return mat;
}

bool CW3ENTMeshFileLoader::load(io::IReadFile* file)
{
    readWord(file,4); // CR2W

    core::array<int> data = readInts(file, 10);

    const int fileFormatVersion = data[0];
    if (fileFormatVersion == 162)
    {
        return W3_load(file);
    }
    return false;
}


video::ITexture* CW3ENTMeshFileLoader::getTexture(io::path filename)
{
    io::path baseFilename;
    if (core::hasFileExtension(filename.c_str(), "xbm"))
    {
        core::cutFilenameExtension(baseFilename, filename);
    }
    else
        return SceneManager->getVideoDriver()->getTexture(filename);

    video::ITexture* texture = 0;

    // Check for textures extracted with the LUA tools
    if (FileSystem->existFile(filename))

    filename = baseFilename + core::stringc(".dds");
    filename.replace("\\", "#");
    filename = GameTexturesPath + filename;

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
        filename = GameTexturesPath + baseFilename + possibleExtensions[i];

        if (FileSystem->existFile(filename))
            texture = SceneManager->getVideoDriver()->getTexture(filename);

        if (texture)
            return texture;
    }


    return texture;
}


// Read functions
core::stringc CW3ENTMeshFileLoader::readWord(io::IReadFile* f, int nbLetters)
{
    core::stringc str;

    char buf;
    for (int i = 0; i < nbLetters; ++i)
    {
        f->read(&buf, 1);
        if (buf != 0)
        {
            str += buf;
            if (str.size() > 300)
               break;
        }
    }

    return str;
}

core::array<int> CW3ENTMeshFileLoader::readInts (io::IReadFile* f, int nbInt)
{
    core::array<int> intVect;
    int buf;

    for (int i = 0; i < nbInt; ++i)
    {
        f->read(&buf, 4);
        intVect.push_back(buf);
    }

    return intVect;
}

core::array<unsigned short> CW3ENTMeshFileLoader::readUnsignedShorts (io::IReadFile* f, int nbShorts)
{
    core::array<unsigned short> unShortVect;
    unsigned short buf;

    for (int i = 0; i < nbShorts; ++i)
    {
        f->read(&buf, 2);
        unShortVect.push_back(buf);
    }

    return unShortVect;
}

core::array<unsigned char> CW3ENTMeshFileLoader::readUnsignedChars (io::IReadFile* f, int nbChar)
{
    core::array<unsigned char> unCharVect;
    unsigned char buf;

    for (int i = 0; i < nbChar; ++i)
    {
        f->read(&buf, 1);
        unCharVect.push_back(buf);
    }

    return unCharVect;
}

core::array<float> CW3ENTMeshFileLoader::readFloats (io::IReadFile* f, int nbFloat)
{
    core::array<float> floatVect;
    float buf;

    for (int i = 0; i < nbFloat; ++i)
    {
        f->read(&buf, 4);
        floatVect.push_back(buf);
    }

    return floatVect;
}


int CW3ENTMeshFileLoader::getTextureLayerFromTextureType(core::stringc textureType)
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




#endif // _IRR_COMPILE_WITH_W2ENT_LOADER_

