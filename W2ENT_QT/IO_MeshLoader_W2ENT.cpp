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


#include <sstream>


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
		//SceneManager->getMeshManipulator()->recalculateNormals(AnimatedMesh);
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
    MeshesToLoad.clear();

	return AnimatedMesh;
}


void printVector(core::vector3df vect)
{
    std::cout << "Vector : " << vect.X << ", " << vect.Y << ", " << vect.Z << std::endl;
}


void IO_MeshLoader_W2ENT::make_vertex_group(Submesh_data dataSubMesh, core::array<core::array<unsigned char> > weighting)
{
    core::array<unsigned short> vertex_groups = dataSubMesh.dataH;

    for (unsigned int id_0 = 0; id_0 < weighting.size(); id_0++)
    {
        core::array<unsigned char> data = weighting[id_0]; // weights list of a vertex
        for (int id_1 = 0; id_1 < 4; id_1++)
        {
            unsigned char w  = data[id_1+4]; // # strength
            if (w != 0)
            {
                unsigned short gr = vertex_groups[data[id_1]];
                core::stringc grName = bonenames[gr];

                ISkinnedMesh::SJoint* bone = nullptr;
                // Loop to find the bone with the bonename
                for (unsigned int i = 0; i < AnimatedMesh->getJointCount(); i++)
                {
                    if (grName == AnimatedMesh->getAllJoints()[i]->Name)
                    {
                        bone = AnimatedMesh->getAllJoints()[i];
                        break;
                    }
                }
                if (! bone)
                    ; //std::cout << "bone not found " << grName.c_str()  << std::endl;
                else
                {

                    ISkinnedMesh::SWeight* wt = AnimatedMesh->addWeight(bone);
                    wt->strength = ((float)w)/255.0f;
                    wt->buffer_id = AnimatedMesh->getMeshBufferCount() - 1; // TODO
                    wt->vertex_id = id_0; // TODO

                    if (wt->strength > 1.0f || wt->strength < 0.0f)
                        std::cout << "Error, strength range : " << wt->strength << std::endl;
                    if (wt->buffer_id >= AnimatedMesh->getMeshBufferCount())
                        std::cout << "Error, mesh buffer range" << std::endl;
                    if (wt->vertex_id >= AnimatedMesh->getMeshBuffer(wt->buffer_id)->getVertexCount() )
                        std::cout << "Error, vertex ID range" << std::endl;

                }


                /*irr::core::stringc vertexGroupName = "vertexGroupName = ";
                for (int tmpStr = 0; tmpStr < data2[id_1].size(); ++tmpStr)
                    vertexGroupName.append(data2[id_1][tmpStr]);

                */

            }
        }
    }
}



void IO_MeshLoader_W2ENT::skeleton(io::IReadFile* file)
{
    make_bone();                            // std::cout << "make_bone" <<std::endl;
    make_bone_parent();                     // std::cout << "make_bone_parent" <<std::endl;
    make_bone_position();                   // std::cout << "make_bone_position" <<std::endl;
    make_localMatrix_from_global();         // std::cout << "make_localMatrix_from_global" <<std::endl;
}

void IO_MeshLoader_W2ENT::make_bone()
{
    for (u32 i = 0; i < bones_data.size(); ++i)
    {
        ISkinnedMesh::SJoint* joint = AnimatedMesh->addJoint();
        joint->Name = bones_data[i].name;

        log->addLineAndFlush(joint->Name);
    }
}

void IO_MeshLoader_W2ENT::make_bone_parent()
{
    for (u32 i = 0; i < bones_data.size(); ++i)
    {
        bone_data data = bones_data[i];
        core::stringc parentName = "pelvis";
        core::stringc boneName = data.name;

        parentName = searchParent(boneName);
        if (parentName.size() > 0)
        {
            //std::cout << "bone : " << parentName.c_str() << " -> " << boneName.c_str() << std::endl;

            ISkinnedMesh::SJoint* joint = AnimatedMesh->getAllJoints()[AnimatedMesh->getJointNumber(boneName.c_str())];
            if (AnimatedMesh->getJointNumber(parentName.c_str()) != -1)
            {
                ISkinnedMesh::SJoint* jointParent = AnimatedMesh->getAllJoints()[AnimatedMesh->getJointNumber(parentName.c_str())];
                if (jointParent)
                    jointParent->Children.push_back(joint);
            }

        }
        else
            ; //std::cout << "Root bone : " << parentName.c_str() << " from " << boneName.c_str() << std::endl;
    }

    /*
    newarm.makeEditable()
    for bone_id in range(len(bones_data)):
        bonedata = bones_data[bone_id]
        parentname = 'pelvis'
        bonename = bonedata[1]

        parentname = searchParent(bonename)
        if len(parentname)>0:
            bone = newarm.bones[bonename]
            boneparent = newarm.bones[parentname]
            bone.parent = boneparent
    newarm.update()
    */
}

void IO_MeshLoader_W2ENT::addVectorToLog(irr::core::vector3df vec)
{
    log->addLineAndFlush(formatString("Vector : %f %f %f", vec.X, vec.Y, vec.Z));
}

void IO_MeshLoader_W2ENT::addMatrixToLog(irr::core::matrix4 mat)
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

    addVectorToLog(mat.getTranslation());
    addVectorToLog(mat.getRotationDegrees());

    logContent = "End matrix4\n\n";

    log->addAndFlush(logContent);
}

void IO_MeshLoader_W2ENT::make_bone_position()
{
    for (u32 i = 0; i < bones_data.size(); ++i)
    {
        bone_data data = bones_data[i];
        core::stringc boneName = data.name;
        irr::core::matrix4 matr = data.matr;

        ISkinnedMesh::SJoint* joint = AnimatedMesh->getAllJoints()[AnimatedMesh->getJointNumber(boneName.c_str())];

        core::vector3df position = matr.getTranslation();
        core::matrix4 invRot;
        matr.getInverse(invRot);

        /*
        Because we switched Y/Z axis we're supposed to add a (90, 0,  180) rotation
        core::matrix4 axisMatrix;
        axisMatrix.setInverseRotationDegrees(core::vector3df(90, 0,  180));
        axisMatrix.rotateVect(position);
        */

        core::vector3df rotation = invRot.getRotationDegrees();
        position = -position;
        core::vector3df scale = matr.getScale();

        if (joint)
        {

            if (log->isEnabled())
            {
                log->addLine(formatString("Joint %s", joint->Name.c_str()));
                log->addLine(formatString("Position : X=%f, Y=%f, Z=%f", position.X, position.Y, position.Z));
                log->addLine(formatString("Rotation : X=%f, Y=%f, Z=%f", rotation.X, rotation.Y, rotation.Z));
                log->addLine(formatString("Scale : X=%f, Y=%f, Z=%f", scale.X, scale.Y, scale.Z));
                log->flush();
            }

            //Build GlobalMatrix:
            core::matrix4 positionMatrix;
            positionMatrix.setTranslation(position);
            core::matrix4 rotationMatrix;
            rotationMatrix.setRotationDegrees(rotation);
            core::matrix4 scaleMatrix;
            scaleMatrix.setScale(scale);

            //printVector(axisMatrix.getRotationDegrees());

            joint->GlobalMatrix = scaleMatrix * rotationMatrix * positionMatrix;
        }
    }
    /*
    for bone_id in range(len(bones_data)):
        newarm.makeEditable()
        bonedata = bones_data[bone_id]
        bonename = bonedata[1]
        bonematrix = bonedata[0]
        bone = newarm.bones[bonename]
        pos = bonematrix.translationPart()
        rot = bonematrix.rotationPart().invert()
        pos = pos*rot
        bone.head = Vector(pos.negate())
        bvec = bone.tail- bone.head
        bvec.normalize()
        bone.tail = bone.head + 0.1 * bvec
        newarm.update()
    */

}

void IO_MeshLoader_W2ENT::computeLocal(ISkinnedMesh::SJoint* joint)
{
    // Get parent
    const core::stringc parentName = searchParent(joint->Name.c_str());
    scene::ISkinnedMesh::SJoint* jointParent = nullptr;
    if (AnimatedMesh->getJointNumber(parentName.c_str()) != -1)
    {
        jointParent = AnimatedMesh->getAllJoints()[AnimatedMesh->getJointNumber(parentName.c_str())];
    }

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

void IO_MeshLoader_W2ENT::make_localMatrix_from_global()
{
    for (u32 i = 0; i < bones_data.size(); ++i)
    {
        bone_data data = bones_data[i];
        core::stringc boneName = data.name;

        ISkinnedMesh::SJoint* joint = AnimatedMesh->getAllJoints()[AnimatedMesh->getJointNumber(boneName.c_str())];
        computeLocal(joint);


        /*
        logContent += "Joint ";
        logContent += joint->Name;
        logContent += "\n";

        logContent += "Globale lue : \n";
        addMatrixToLog(joint->GlobalMatrix);
        logContent += "\nGlobale from locale : \n";
        */

        //addMatrixToLog(globalComputed);
        //logContent += "\n\n\n\n";

        irr::core::matrix4 localMatrix = joint->LocalMatrix;

        irr::core::matrix4 invRot;
        localMatrix.getInverse(invRot);

        irr::core::vector3df rotatedVect = joint->LocalMatrix.getTranslation();

        irr::core::matrix4 translationMat;
        translationMat.setTranslation(rotatedVect);
        irr::core::matrix4 rotationMat;
        rotationMat.setRotationDegrees(joint->LocalMatrix.getRotationDegrees());
        //joint->LocalMatrix.getInverse(rotationMat);
        //printVector(invRot.getRotationDegrees());

        //joint->LocalMatrix = rotationMat * translationMat;

        joint->Animatedposition = rotatedVect;
        joint->Animatedscale = joint->LocalMatrix.getScale();
        joint->Animatedrotation = core::vector3df(0, 0, 0);
    }
}

bool IO_MeshLoader_W2ENT::load(io::IReadFile* file)
{
    int back = file->getPos();

    readString(file, 4); // CR2W

    RedEngineFileHeader header;
    loadTW2FileHeader(file, header, false);
    Version = header.Version;
    Strings = header.Strings;
    Files = header.Files;

    log->addLineAndFlush("Table 1 & 2 OK");

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

    int nMat = 0, nModel = 0;
    core::array <core::stringc> chunks;

    // Ok, now we can read the materials and meshes
    file->seek(back + headerData[4]);
    for(int i = 0; i < headerData[5]; ++i) // Now, the list of all the components of the files
    {
        // Read data
        u16 dataTypeId = readU16(file) - 1;    // Data type
        core::stringc dataTypeName = Strings[dataTypeId];
        log->addLineAndFlush(formatString("dataTypeName=%s", dataTypeName.c_str()));

        core::array<s32> data2 = readDataArray<s32>(file, 5);             // Data info (adress...)

        DataInfos infos;
        infos.size = data2[1];
        infos.adress = data2[2];

        core::stringc mesh_source;                              // mesh source ? useless

        if (data2[0] == 0)
        {
            u8 size = readU8(file) - 128;

            unsigned char offset;
            file->read(&offset, 1);
            file->seek(-1, true);

            // decalage of 1 octet in this case
            if (offset == 1)
                file->seek(1, true);

            mesh_source = readString(file, size);
        }
        else
        {
            file->seek(1, true);    //readUnsignedChars(file, 1)[0]-128;
        }

        if (!find(chunks, dataTypeName))    //check if 'name' is already in 'chuncks'. If this is not the case, name is added in chunk
            chunks.push_back(dataTypeName);

        // Now we check the type of the data, and if we have a CMaterialInstance or CMesh we read it
        //std::cout << name.c_str() << std::endl;


        // now all stuff readed
        const int back2 = file->getPos();

        if (dataTypeName == "CMaterialInstance")
        {
            log->addAndFlush("\nCMaterialInstance\n");

            CMaterialInstance(file, infos, nMat);
            MeshesToLoad[MeshesToLoad.size()-1].nMat.push_back(nMat);
            nMat++;

            log->addLineAndFlush("CMaterialInstance OK");
        }
        else if (dataTypeName == "CMesh")
        {
            //std::cout << "CMesh" << std::endl;            
            log->addAndFlush("\nCMesh\n");

            // Just get the filename from the filepath
            int index = mesh_source.findLast('\\');
            core::stringc mesh_name = mesh_source.subString(index, mesh_source.size() - index);

            // if there is no filepath ? we set a default name
            if(mesh_name.size() == 0)
                mesh_name = "model";

            Meshdata m_data;
            m_data.nModel = nModel;             // The index of the mesh (useful if there are many meshes)
            m_data.infos = infos;
            MeshesToLoad.push_back(m_data);

            nModel++;

            log->addLineAndFlush("CMesh OK");
        }
        else if (dataTypeName == "CSkeleton")
        {
            //std::cout << "CSkeleton" << std::endl;
            CSkeleton(file, infos);
        }
        else if (dataTypeName == "CSkeletalAnimation")
        {
            //std::cout << "CSkeletalAnimation" << std::endl;
        }
        else if (dataTypeName == "CLayer")
        {
            //std::cout << "CLayer" << std::endl;
        }
        file->seek(back2);
    }
    log->addLineAndFlush("Textures and mesh data OK");

    // When we have loaded all the materials and all the 'mesh headers', we load the meshes
    for (unsigned int i = 0; i < MeshesToLoad.size(); i++)
    {
        // Create mesh
        CMesh(file, MeshesToLoad[i]);
    }

    log->addLineAndFlush("All is loaded");
	return true;
}

bool checkIfBoneDontExist(core::stringc name, irr::core::array<bone_data> datas)
{
    for (u32 i = 0; i < datas.size(); i++)
    {
        if (datas[i].name == name)
            return false;
    }
    return true;
}

void IO_MeshLoader_W2ENT::CSkeleton(io::IReadFile* file, DataInfos infos)
{
    file->seek(infos.adress);

    //std::cout << "begin at " << infos.adress << " and end at " << infos.adress + infos.size << ", so size = " << infos.size << std::endl;
    while(1)
    {
        W2_PropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
            break;


        log->addLineAndFlush(propHeader.toString());
        file->seek(propHeader.endPos);
    }
    file->seek(-4, true);
}

void IO_MeshLoader_W2ENT::CMesh(io::IReadFile* file, Meshdata tmp)
{
    log->addLineAndFlush("Load a mesh...");

    core::array<int> mats = tmp.nMat;

    // ?
    //for (unsigned int i = 0; i < tmp.nMat.size(); i++)
    //    mats.push_back(tmp.nMat[i]);

    // we go to the adress of the data
    file->seek(tmp.infos.adress);
    int nModel = tmp.nModel;    // we get the mesh index

    // Read all the properties of the mesh
    while(1)
    {
        W2_PropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
            break;

        file->seek(propHeader.endPos);
    }
    file->seek(-4, true);

    // Read the LODS data ?
    vert_format(file);

    u8 nbBones = readU8(file);

    int back = file->getPos();

    /*
    if  (magic == 1)
        readU8(file);
    else
        rigged = 1;
    */

    u8 magicData = readU8(file);
    if (magicData != 1)
        file->seek(-1, true);

    if (nbBones == 128)
    {
        file->seek(1, true); //readUnsignedChars(file, 1);
        static_meshes(file, mats);
    }
    else // If the mesh isn't static
    {
        log->addLineAndFlush("Loop");

        bonenames.clear();
        bones_data.clear();

        for (int i = 0; i < nbBones; i++)
        {
            bone_data tmpData;
            // read matrix
            for (u32 j = 0; j < 16; ++j)
            {
                float value;
                file->read(&value, 4);
                tmpData.matr[j] = value;
            }

            // bone name
            u16 nameId = readU16(file) - 1;

            core::stringc name = "";
            if (nameId < Strings.size())
            {
                name = Strings[nameId];
            }
            else
            {
                log->addLineAndFlush("error_with_bones OK");
                name = "bone-";
                name += i;
            }

            bool ok = true;
            for (u32 j = 0; j < AnimatedMesh->getJointCount(); j++)
            {
                if (AnimatedMesh->getAllJoints()[j]->Name == name)
                    ok = false;
            }
            bonenames.push_back(name);
            if (ok)
            {
                tmpData.name = name;
                bones_data.push_back(tmpData);
            }

            file->seek(4, true); //float data12 = readFloats(file, 1)[0];
        }
        nbBones = readU8(file);
        back = file->getPos();
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

        s32 seek = file->getPos() + readS32(file);
        back = file->getPos();
        file->seek(seek);
        core::array<s32> data = readDataArray<s32>(file, 6);
        u8 nbSubMesh = readU8(file);

        core::array<Submesh_data> subMeshesData;
        for (u8 i = 0; i < nbSubMesh; i++)
        {
            Submesh_data s_data;
            s_data.vertexType = readU8(file);
            s_data.dataI = readDataArray<s32>(file, 4);
            s_data.dataH = readDataArray<u16>(file, readU8(file)+2);
            subMeshesData.push_back(s_data);

            /*
            if ((s_data.dataI[2] < 0 || s_data.dataI[2] > 100000 || error_with_bones ) && magic == false) // Can crash here
            {
                // We reload with 'the magic button'
                GEOMETRY(file, tmp, true);
                return;
            }
            */

            log->addLineAndFlush(formatString("submesh : vertEnd = %d, vertype = %d", s_data.dataI[2], s_data.vertexType));
        }

        file->seek(back);

        // Skeleton called before drawmesh to avoid crash
        skeleton(file);


        drawmesh(file, data, subMeshesData, mats);

    }

    log->addLineAndFlush("Mesh loaded");
}


void IO_MeshLoader_W2ENT::static_meshes(io::IReadFile* file, core::array<int> mats)
{
    // We read submeshes infos
    long back = file->getPos();
    int subMeshesInfosOffset = readS32(file);
    file->seek(back + subMeshesInfosOffset);

    file->seek(4, true); // readUnsignedShorts(file, 2);
    file->seek(4, true); // int

    core::array<s32> meshData = readDataArray<s32>(file, 4); // Indices adress + unks
    u8 nbSubMesh = readU8(file);
    u8 vertexType = readU8(file);

    core::array<Submesh_data> subMeshesData;
    for (u8 i = 0; i < nbSubMesh; i++)
    {
        Submesh_data s_data;
        s_data.vertexType = vertexType;                 // The type of vertice determine the size of a vertices (it depend of the number of informations stored in the vertice)
        s_data.dataI = readDataArray<s32>(file, 5);
        s_data.dataH = readDataArray<u16>(file, 1);
        subMeshesData.push_back(s_data);
    }

    file->seek(back+4);
    drawmesh_static(file, meshData, subMeshesData, mats);
}


void IO_MeshLoader_W2ENT::drawmesh_static(io::IReadFile* file, core::array<int> meshData, core::array<Submesh_data> subMeshesData, core::array<int> mats)
{
    log->addLineAndFlush("Drawmesh_static");

    long back = file->getPos();
    const video::SColor defaultColor(255, 255, 255, 255);

    for (u32 i = 0; i < subMeshesData.size(); i++)
    {
        Submesh_data submesh = subMeshesData[i];
        if (i >= IdLOD[0][0])
            continue;


        int vertexSize = 0;
        bool hasSecondUVLayer = false;
        switch (submesh.vertexType) {
        case 0:
            vertexSize = 36;
            break;

        case 6:
            vertexSize = 44;
            hasSecondUVLayer = true;
            break;

        case 9:
        case 5:
            vertexSize = 60;
            break;

        default:
            log->addLineAndFlush(formatString("Unknown vertexType: %d", submesh.vertexType));
            break;
        }
        int verticesStart = submesh.dataI[0];
        int verticesCount = submesh.dataI[2];

        log->addLineAndFlush(formatString("submesh (vertype: %d, vertsize: %d, vertStart = %d)", submesh.vertexType, vertexSize, file->getPos()));
        file->seek(back + verticesStart * vertexSize);

        SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
        if (hasSecondUVLayer)
        {
            buffer->VertexType = video::EVT_2TCOORDS;
            buffer->Vertices_2TCoords.reallocate(verticesCount);
        }
        else
        {
            buffer->VertexType = video::EVT_STANDARD;
            buffer->Vertices_Standard.reallocate(verticesCount);
        }

        for (int i = 0; i < verticesCount; i++)
        {
            long vertexAdress = file->getPos();
            core::array<f32> position = readDataArray<f32>(file, 3);
            file->seek(8, true);
            core::array<f32> uv = readDataArray<f32>(file, 2);

            if (hasSecondUVLayer)
            {
                core::array<f32> uv2 = readDataArray<f32>(file, 2);
                video::S3DVertex2TCoords vertex;
                vertex.Pos = core::vector3df(position[0], position[1], position[2]);
                vertex.TCoords = core::vector2df(uv[0], uv[1]);
                vertex.TCoords2 = core::vector2df(uv2[0], uv2[1]);
                vertex.Color = defaultColor;
                buffer->Vertices_2TCoords.push_back(vertex);
            }
            else
            {
                video::S3DVertex vertex;
                vertex.Pos = core::vector3df(position[0], position[1], position[2]);
                vertex.TCoords = core::vector2df(uv[0], uv[1]);
                vertex.Color = defaultColor;
                buffer->Vertices_Standard.push_back(vertex);
            }

            file->seek(vertexAdress + vertexSize);
        }
        int indicesStart = submesh.dataI[1];
        int indicesCount = submesh.dataI[3];

        file->seek(back + meshData[0] + indicesStart * 2);

        buffer->Indices.set_used(indicesCount);
        file->read(buffer->Indices.pointer(), indicesCount * 2);


        int result = 0;
        if (i < mats.size())
            if ((unsigned int)mats[i] < Materials.size())
                result = mats[i];
        //std::cout << "MaterialSize= " << Materials.size() << std::endl;
        //std::cout << "Result : " << result << ", mat id : " << Materials[result].id << ", mat[n]" << mats[n] << std::endl;

        buffer->Material = Materials[result].material;

        SceneManager->getMeshManipulator()->recalculateNormals(buffer);
        buffer->recalculateBoundingBox();
    }
    AnimatedMesh->setDirty();

    log->addLineAndFlush("Drawmesh_static OK");
}


void IO_MeshLoader_W2ENT::drawmesh(io::IReadFile* file, core::array<int> meshData, core::array<Submesh_data> subMeshesData, core::array<int> mats)
{
    log->addLineAndFlush("Drawmesh");

    int back = file->getPos();
    const video::SColor defaultColor(255, 255, 255, 255);

    for (u32 i = 0; i < subMeshesData.size(); i++)
    {
        Submesh_data submesh = subMeshesData[i];
        if (i >= IdLOD[0][0])
            continue;


        int vertexSize = 0;
        switch (submesh.vertexType) {
        case 1:
            vertexSize = 44;
            break;

        case 11:
            vertexSize = 44;
            break;

        default:
            vertexSize = 52;
            log->addLineAndFlush(formatString("Unknown vertexType: %d", submesh.vertexType));
            break;
        }

        int verticesStart = submesh.dataI[0];
        int verticesCount = submesh.dataI[2];

        log->addLineAndFlush(formatString("submesh (vertype: %d, vertsize: %d, vertStart = %d)", submesh.vertexType, vertexSize, file->getPos()));
        file->seek(back + verticesStart * vertexSize);

        SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
        buffer->Vertices_Standard.reallocate(verticesCount);
        core::array<core::array<u8> > weighting;
        weighting.reallocate(verticesCount);
        for (int i = 0; i < verticesCount; i++)
        {
            int vertexAdress = file->getPos();
            video::S3DVertex vertex;
            core::array<f32> position = readDataArray<f32>(file, 3);
            vertex.Pos = core::vector3df(position[0], position[1], position[2]);

            weighting.push_back(readDataArray<u8>(file, 8));

            file->seek(8, true);
            core::array<f32> uv = readDataArray<f32>(file, 2);
            vertex.TCoords = core::vector2df(uv[0], uv[1]);
            vertex.Color = defaultColor;
            buffer->Vertices_Standard.push_back(vertex);

            file->seek(vertexAdress + vertexSize);
        }

        int indicesStart = submesh.dataI[1];
        int indicesCount = submesh.dataI[3];

        // Faces
        file->seek(back + meshData[2] + indicesStart * 2);
        buffer->Indices.set_used(indicesCount);
        file->read(buffer->Indices.pointer(), indicesCount * 2);


        int result = 0;
        if (i < mats.size())
            if ((unsigned int)mats[i] < Materials.size())
                result = mats[i];

        /*std::cout << "MaterialSize= " << Materials.size() << std::endl;
        std::cout << "Result : " << result << ", mat id : " << Materials[result].id << ", mat[n]" << mats[n] << std::endl;*/

        buffer->Material = Materials[result].material;

        make_vertex_group(SubMeshData[n], weighting);

        SceneManager->getMeshManipulator()->recalculateNormals(buffer);
        buffer->recalculateBoundingBox();
    }

    log->addLineAndFlush("Drawmesh OK");
}

video::ITexture* IO_MeshLoader_W2ENT::getTexture(core::stringc filename)
{
    video::ITexture* texture = nullptr;

    if (core::hasFileExtension(filename.c_str(), "xbm"))
    {
        io::path ddsfile;
        core::cutFilenameExtension(ddsfile, filename);
        ddsfile += ".dds";

        //ddsfile = GamePath + ddsfile;

        if (FileSystem->existFile(ddsfile))
            texture = SceneManager->getVideoDriver()->getTexture(ddsfile.c_str());

        if (!texture)
        {
            // Make a DDS file from the XBM file
            convertXBMToDDS(filename);
            texture = SceneManager->getVideoDriver()->getTexture(ddsfile.c_str());
        }
    }
    else
    {
        texture = SceneManager->getVideoDriver()->getTexture(filename.c_str());
    }

    return texture;
}

bool IO_MeshLoader_W2ENT::ReadPropertyHeader(io::IReadFile* file, W2_PropertyHeader& propHeader)
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

void IO_MeshLoader_W2ENT::CMaterialInstance(io::IReadFile* file, DataInfos infos, int nMats)
{
    int back = file->getPos();
    file->seek(infos.adress);

    log->addLineAndFlush("Read material...");

    video::SMaterial material;
    material.MaterialType = video::EMT_SOLID;

    while (1)
    {
        W2_PropertyHeader propHeader;
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
                back = file->getPos();
                int seek = readS32(file);

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
                        core::stringc texturePath = ConfigGamePath + Files[255-imageID];
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
                file->seek(back + seek);
            }
        }
        file->seek(propHeader.endPos);
    }
    Mat w2Mat;
    w2Mat.id = nMats;
    w2Mat.material = material;

    Materials.push_back(w2Mat);
    //std::cout << "Texture : " << FilesTable[255-readUnsignedChars(1)[0]] << std::endl;
}

void IO_MeshLoader_W2ENT::convertXBMToDDS(core::stringc xbm_file)
{
    log->addLineAndFlush("XBM to DDS");

    // Make the name of the DDS file
    io::path ddsfile;
    core::cutFilenameExtension(ddsfile, xbm_file);
    ddsfile += ".dds";

    // Open the XBM file
    io::IReadFile* fileXBM = FileSystem->createAndOpenFile((xbm_file).c_str());
    if (!fileXBM)
    {
        SceneManager->getParameters()->setAttribute("TW_FEEDBACK", "Some textures havn't been found, check your 'Base directory'.");
        log->addAndFlush(core::stringc("Error : the file ") + xbm_file + core::stringc(" can't be opened.\n"));
        return;
    }

    log->addLineAndFlush("XBM file opened");


    /* This format works like the w2ent format
    The first character (4octets) are the ID of the file type, and next there are the adress and the size of the differents sections of the file :
    - The string list : begin at data[2] and data[3] size
    - The data : begin at data[4] and data[5] size
    */
    fileXBM->seek(0);
    readString(fileXBM, 4);
    core::array<s32> data = readDataArray<s32>(fileXBM, 10);

    //string list
    fileXBM->seek(data[2]);
    core::array<core::stringc> stringsXBM;//Strings = []

    for (int i = 0; i < data[3]; i++)
        stringsXBM.push_back(readString(fileXBM, readU8(fileXBM)-128));

    log->addLineAndFlush("List ok");


    // data
    fileXBM->seek(data[4]);
    for (int i = 0; i < data[5]; i++)
    {
        // The type of the data (cf stringsXBM)
        unsigned short var = readU16(fileXBM);
        // Others informations
        core::array<s32> dataInfos = readDataArray<s32>(fileXBM, 5);

        int back1 = fileXBM->getPos();
        core::array<u8> data1 = readDataArray<u8>(fileXBM, 2);
        fileXBM->seek(back1);

        if (dataInfos[0]==0)
        {
            unsigned char size;
            fileXBM->read(&size, 1);
            size -= 128;

            if (data1[1]==1)
                fileXBM->seek(1, true);

            const core::stringc mesh_source = readString(fileXBM, size);
        }
        else
            fileXBM->seek(1, true); // readUnsignedChars(file2, 1)[0]-128;

        int back3 = fileXBM->getPos();

        // If the data is a CBitmapTexture, we read the data
        if (stringsXBM[var-1] == "CBitmapTexture")
            TEXTURE(fileXBM, ddsfile, dataInfos, stringsXBM);

        fileXBM->seek(back3);
    }
    fileXBM->drop();

    log->addLineAndFlush("XBM to DDS OK");
}

void IO_MeshLoader_W2ENT::TEXTURE(io::IReadFile* fileXBM, core::stringc filenameDDS, core::array<int> data, core::array<core::stringc> stringsXBM)
{
    // int back = file->getPos();
    log->addLineAndFlush("CBitmapTexture");

    fileXBM->seek(data[2]);

    char ddsheader[] = "\x44\x44\x53\x20\x7C\x00\x00\x00\x07\x10\x0A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x05\x00\x00\x00\x44\x58\x54\x31\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x10\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

    core::stringc dxt;
    int height1, width1;

    // Read the header infos
    while(1)
    {
        log->addLineAndFlush("Read header data...");

        // name of the element
        core::stringc propertyName = stringsXBM[readU16(fileXBM)-1];
        // type of the element
        core::stringc propertyType = stringsXBM[readU16(fileXBM)-1];

        fileXBM->seek(2, true);  //readUnsignedChars(fileXBM, 2);
        int back2 = fileXBM->getPos();

        s32 seek = readS32(fileXBM); // size of the property
        // The dimensions of the textures
        if (propertyName == "width" && propertyType == "Uint")
        {
            width1 = readS32(fileXBM);
        }
        else if (propertyName == "height" && propertyType == "Uint")
        {
            height1 = readS32(fileXBM);
        }
        // Compression format
        else if (propertyType == "ETextureCompression")
        {
            dxt = stringsXBM[readU16(fileXBM)-1];

            if  (dxt == "TCM_DXTNoAlpha")
                dxt = "\x44\x58\x54\x31";
            else if (dxt == "TCM_DXTAlpha")
                dxt = "\x44\x58\x54\x35";
            else if (dxt == "TCM_NormalsHigh")
                dxt = "\x44\x58\x54\x35";
            else if (dxt == "TCM_Normals")
                dxt = "\x44\x58\x54\x31";
        }

        fileXBM->seek(back2+seek);
        if (propertyName == "importFile")
            break;
    }
    log->addLineAndFlush("Read header ok");

    fileXBM->seek(27, true); //readUnsignedChars(fileXBM, 27);

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
        const long sizeToCopy = fileXBM->getSize() - fileXBM->getPos();


        char* buffer = new char[sizeToCopy];
        fileXBM->read(buffer, sizeToCopy);

        log->addLineAndFlush("Read XBM OK");

        fileDDS->write(buffer, sizeToCopy);
        delete[] buffer;

        log->addLineAndFlush("Write DDS OK");

        fileDDS->drop();
    }
    else
    {
        // Case of an empty file
        // TODO
        /*  new = Blender.Image.New(file.split('.')[0]+'.dds',height1,width1,24)

            for (int m = 0 ; m < height1; m++)//m in range(height1)
                for (int n = 0 ; n < width1; n++)//for n in range(width1)
                {
                    pix = readUnsignedChars(file, 4);

                    fileDDS.setPixelI(m,n,[pix[0],pix[1],pix[2],pix[3]]);
                }
        */
    }

}

void IO_MeshLoader_W2ENT::vert_format(io::IReadFile* file)
{
    // clear the vector
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




//this part of scripts thanks bm1 from xentax
core::stringc IO_MeshLoader_W2ENT::searchParent(core::stringc bonename)
{
        irr::core::stringc parentname = "torso";
        if (bonename == "pelvis")
            return "";
        if (bonename == "torso2")
            return "torso";
        if (bonename == "torso")
            return "pelvis";
        if (bonename == "neck")
             return "torso2";
        if (bonename == "head")
             return "neck";

        if (bonename == "l_thigh")
            return "pelvis";
        if (bonename == "l_shin")
             return "l_thigh";
        if (bonename == "l_foot")
             return "l_shin";
        if (bonename == "l_toe")
             return "l_foot";

        if (bonename == "l_legRoll")
            return "torso";
        if (bonename == "l_legRoll2")
             return "l_thigh";
        if (bonename == "l_kneeRoll")
             return "l_shin";


        if (bonename == "l_shoulder")
            return "torso2";
        if (bonename == "l_shoulderRoll")
             return "l_shoulder";
        if (bonename == "l_bicep")
             return "l_shoulder";
        if (bonename == "l_bicep2")
             return "l_bicep";

        if (bonename == "l_elbowRoll")
             return "l_bicep";
        if (bonename == "l_forearmRoll1")
             return "l_bicep";
        if (bonename == "l_forearmRoll2")
             return "l_forearmRoll1";
        if (bonename == "l_forearm") // addition
             return "l_elbowRoll";
        if (bonename == "l_handRoll")
             return "l_hand";
        if (bonename == "l_hand")
             return "l_forearmRoll2";

        if (bonename == "l_thumb1")
             return "l_hand";
        if (bonename == "l_index1")
             return "l_hand";
        if (bonename == "l_middle1")
             return "l_hand";
        if (bonename == "l_ring1")
             return "l_hand";
        if (bonename == "l_pinky1")
             return "l_hand";

        if (bonename == "l_thumb2")
             return "l_thumb1";
        if (bonename == "l_index2")
             return "l_index1";
        if (bonename == "l_middle2")
             return "l_middle1";
        if (bonename == "l_ring2")
             return "l_ring1";
        if (bonename == "l_pinky2")
             return "l_pinky1";

        if (bonename == "l_thumb3")
             return "l_thumb2";
        if (bonename == "l_index3")
             return "l_index2";
        if (bonename == "l_middle3")
             return "l_middle2";
        if (bonename == "l_ring3")
             return "l_ring2";
        if (bonename == "l_pinky3")
             return "l_pinky2";


        if (bonename == "l_thumb4")
             return "l_thumb3";
        if (bonename == "l_index4")
             return "l_index3";
        if (bonename == "l_middle4")
             return "l_middle3";
        if (bonename == "l_ring4")
             return "l_ring3";
        if (bonename == "l_pinky4")
             return "l_pinky3";

        if (bonename == "r_thigh")
            return "pelvis";
        if (bonename == "r_shin")
             return "r_thigh";
        if (bonename == "r_foot")
             return "r_shin";
        if (bonename == "r_toe")
             return "r_foot";

        if (bonename == "r_legRoll")
            return "torso";
        if (bonename == "r_legRoll2")
             return "r_thigh";
        if (bonename == "r_kneeRoll")
             return "r_shin";


        if (bonename == "r_shoulder")
            return "torso2";
        if (bonename == "r_shoulderRoll")
             return "r_shoulder";
        if (bonename == "r_bicep")
             return "r_shoulder";
        if (bonename == "r_bicep2")
             return "r_bicep";

        if (bonename == "r_elbowRoll")
             return "r_bicep";
        if (bonename == "r_forearmRoll1")
             return "r_bicep";
        if (bonename == "r_forearm")
             return "r_elbowRoll";
        if (bonename == "r_forearmRoll2")
             //return "r_forearm";         //# r_forearmRoll1 missing !
             return "r_forearmRoll1";
        if (bonename == "r_handRoll")
             return "r_hand";
        if (bonename == "r_hand")
             return "r_forearmRoll2";

        if (bonename == "r_thumb1")
             return "r_hand";
        if (bonename == "r_index1")
             return "r_hand";
        if (bonename == "r_middle1")
             return "r_hand";
        if (bonename == "r_ring1")
             return "r_hand";
        if (bonename == "r_pinky1")
             return "r_hand";

        if (bonename == "r_thumb2")
             return "r_thumb1";
        if (bonename == "r_index2")
             return "r_index1";
        if (bonename == "r_middle2")
             return "r_middle1";
        if (bonename == "r_ring2")
             return "r_ring1";
        if (bonename == "r_pinky2")
             return "r_pinky1";

        if (bonename == "r_thumb3")
             return "r_thumb2";
        if (bonename == "r_index3")
             return "r_index2";
        if (bonename == "r_middle3")
             return "r_middle2";
        if (bonename == "r_ring3")
             return "r_ring2";
        if (bonename == "r_pinky3")
             return "r_pinky2";

        if (bonename == "r_thumb4")
             return "r_thumb3";
        if (bonename == "r_index4")
             return "r_index3";
        if (bonename == "r_middle4")
             return "r_middle3";
        if (bonename == "r_ring4")
             return "r_ring3";
        if (bonename == "r_pinky4")
             return "r_pinky3";

        if (bonename == "Hair_R_01")
             return "head";
        if (bonename == "Hair_R_02")
             return "Hair_R_01";
        if (bonename == "Hair_R_03")
             return "Hair_R_02";

        if (bonename == "Hair_L_01")
             return "head";
        if (bonename == "Hair_L_02")
             return "Hair_L_01";
        if (bonename == "Hair_L_03")
             return "Hair_L_02";


        if (bonename == "jaw")
             return "head_face";
        if (bonename == "head_face")
             return "head";

        if (bonename == "lowwer_lip")
             return "jaw";
        if (bonename == "lowwer_right_lip")
             return "jaw";
        if (bonename == "lowwer_left_lip")
             return "jaw";
        if (bonename == "right_mouth3")
             return "jaw";
        if (bonename == "left_mouth3")
             return "jaw";
        if (bonename == "tongue")
             return "jaw";

        if (bonename == "right_corner_lip")
             return "head_face";
        if (bonename == "left_corner_lip")
             return "head_face";

        if (bonename == "lowwer_right_eyelid")
             return "head_face";
        if (bonename == "upper_right_eyelid")
             return "head_face";
        if (bonename == "right_eye")
             return "head_face";

        if (bonename == "lowwer_left_eyelid")
             return "head_face";
        if (bonename == "upper_left_eyelid")
             return "head_face";
        if (bonename == "left_eye")
             return "head_face";

        if (bonename == "right_chick3")
             return "head_face";
        if (bonename == "right_chick2")
             return "head_face";
        if (bonename == "left_chick3")
             return "head_face";
        if (bonename == "left_chick2")
             return "head_face";
        if (bonename == "left_chick1")
             return "head_face";
        if (bonename == "right_chick1")
             return "head_face";

        if (bonename == "eyebrow_left")
             return "head_face";
        if (bonename == "eyebrow_right")
             return "head_face";
        if (bonename == "eyebrow2_left")
             return "head_face";
        if (bonename == "eyebrow2_right")
             return "head_face";
        if (bonename == "left_mouth1")
             return "head_face";
        if (bonename == "right_mouth1")
             return "head_face";
        if (bonename == "right_nose")
             return "head_face";
        if (bonename == "left_nose")
             return "head_face";


        if (bonename == "right_mouth2")
             return "head_face";
        if (bonename == "left_mouth2")
             return "head_face";
        if (bonename == "upper_left_lip")
             return "head_face";
        if (bonename == "upper_lip")
             return "head_face";
        if (bonename == "upper_right_lip")
             return "head_face";

        return parentname;

}


bool IO_MeshLoader_W2ENT::find (core::array<core::stringc> stringVect, core::stringc name)
{
    for (unsigned int i = 0; i < stringVect.size(); ++i)
    {
        if (stringVect[i] == name)
            return true;
    }
    return false;
}

} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_W2ENT_LOADER_

