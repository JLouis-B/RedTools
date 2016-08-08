// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#define _IRR_COMPILE_WITH_W2ENT_LOADER_
#ifdef _IRR_COMPILE_WITH_W2ENT_LOADER_

#include "CW2ENTMeshFileLoader.h"

#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "IReadFile.h"
#include "IWriteFile.h"

#include "LoadersUtils.h"
#include "settings.h"


#include <sstream>

//#define _DEBUG


namespace irr
{
namespace scene
{

//! Constructor
CW2ENTMeshFileLoader::CW2ENTMeshFileLoader(scene::ISceneManager* smgr, io::IFileSystem* fs)
: SceneManager(smgr), FileSystem(fs), AnimatedMesh(0), log(0)
{
	#ifdef _DEBUG
	setDebugName("CW2ENTMeshFileLoader");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CW2ENTMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
    irr::io::IReadFile* file = SceneManager->getFileSystem()->createAndOpenFile(filename);
    if (!file)
        return false;

    file->seek(4);

    int version = 0;
    file->read(&version, 4);

    if (version < 162)
    {
        file->drop();
        return core::hasFileExtension ( filename, "w2ent" ) || core::hasFileExtension ( filename, "w2mesh" );
    }

    file->drop();
    return false;
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CW2ENTMeshFileLoader::createMesh(io::IReadFile* f)
{
	if (!f)
		return 0;

    LogOutput output = LOG_NONE;
    // If we want the log in cout
    //output |= LOG_CONSOLE;

    if (SceneManager->getParameters()->getAttributeAsBool("TW_DEBUG_LOG"))
        output |= LOG_FILE;
    log = new Log(SceneManager, "debug.log", output);

    #ifdef _IRR_WCHAR_FILESYSTEM
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsStringW("TW_GAME_PATH");
    #else
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsString("TW_GAME_PATH");
    #endif

    // log
    log->add("-> Exported with The Witcher Converter ");
    log->add(Settings::_appVersion.toStdString().c_str());
    log->add("\n");

    log->add("-> File : ");
    log->add(f->getFileName().c_str());
    log->add("\n");

    log->add("_________________________________________________________\n\n\n");

    log->add("Start loading\n");
    log->push();

    AnimatedMesh = SceneManager->createSkinnedMesh();

	if (load(f))
	{
		AnimatedMesh->finalize();
		//SceneManager->getMeshManipulator()->recalculateNormals(AnimatedMesh);
	}
	else
	{
		AnimatedMesh->drop();
		AnimatedMesh = 0;
    }

	//Clear up
    FilesTable.clear();
    Strings.clear();
    Materials.clear();
    MeshesToLoad.clear();
    NbSubMesh = 0;


    delete log;
	return AnimatedMesh;
}


void printVector(core::vector3df vect)
{
    std::cout << "Vector : " << vect.X << ", " << vect.Y << ", " << vect.Z << std::endl;
}


void CW2ENTMeshFileLoader::make_vertex_group(Submesh_data dataSubMesh, core::array<core::array<unsigned char> > weighting)
{
    core::array <unsigned short> vertex_groups = dataSubMesh.dataH;

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

                ISkinnedMesh::SJoint* bone = 0;
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

                logContent += vertexGroupName;
                writeLog();*/

            }
        }
    }
}



void CW2ENTMeshFileLoader::skeleton(io::IReadFile* file)
{
    make_bone();                            // std::cout << "make_bone" <<std::endl;
    make_bone_parent();                     // std::cout << "make_bone_parent" <<std::endl;
    make_bone_position();                   // std::cout << "make_bone_position" <<std::endl;
    make_localMatrix_from_global();         // std::cout << "make_localMatrix_from_global" <<std::endl;
}

void CW2ENTMeshFileLoader::make_bone()
{
    for (u32 i = 0; i < bones_data.size(); ++i)
    {
        ISkinnedMesh::SJoint* joint = AnimatedMesh->addJoint();
        joint->Name = bones_data[i].name;

        log->addAndPush(joint->Name + "\n");
    }
}

void CW2ENTMeshFileLoader::make_bone_parent()
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

void CW2ENTMeshFileLoader::addVectorToLog(irr::core::vector3df vec)
{
    core::stringc logContent = "Vector : ";
    logContent += vec.X;
    logContent += "  ";
    logContent += vec.Y;
    logContent += "  ";
    logContent += vec.Z;
    logContent += "\n";

    log->addAndPush(logContent);
}

void CW2ENTMeshFileLoader::addMatrixToLog(irr::core::matrix4 mat)
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
    log->addAndPush(logContent);

    addVectorToLog(mat.getTranslation());
    addVectorToLog(mat.getRotationDegrees());

    logContent = "End matrix4\n\n";

    log->addAndPush(logContent);
}

void CW2ENTMeshFileLoader::make_bone_position()
{
    for (u32 i = 0; i < bones_data.size(); ++i)
    {
        bone_data data = bones_data[i];
        core::stringc boneName = data.name;
        irr::core::matrix4 matr = data.matr;

        ISkinnedMesh::SJoint* joint = AnimatedMesh->getAllJoints()[AnimatedMesh->getJointNumber(boneName.c_str())];

        irr::core::vector3df position = matr.getTranslation();
        irr::core::matrix4 invRot;
        matr.getInverse(invRot);
        invRot.rotateVect(position);

        core::matrix4 axisMatrix;
        axisMatrix.setInverseRotationDegrees(core::vector3df(90, 0,  180));
        axisMatrix.rotateVect(position);

        core::vector3df rotation = invRot.getRotationDegrees();
        rotation = core::vector3df(0, 0, 0);
        position = - position;
        irr::core::vector3df scale = core::vector3df(1, 1, 1);//invRot.getScale();

        if (joint)
        {

            if (log->isEnabled())
            {
                core::stringc logContent = "Joint ";
                logContent += joint->Name;
                logContent += "\nPosition : X=";
                logContent += position.X;
                logContent += ", Y=";
                logContent += position.Y;
                logContent += ", Z=";
                logContent += position.Z;

                logContent += "\nRotation : X=";
                logContent += rotation.X;
                logContent += ", Y=";
                logContent += rotation.Y;
                logContent += ", Z=";
                logContent += rotation.Z;

                logContent += "\nScale : X=";
                logContent += scale.X;
                logContent += ", Y=";
                logContent += scale.Y;
                logContent += ", Z=";
                logContent += scale.Z;
                logContent += "\n\n";

                log->addAndPush(logContent);
            }

            //Build GlobalMatrix:
            core::matrix4 positionMatrix;
            positionMatrix.setTranslation( position );
            core::matrix4 scaleMatrix;
            scaleMatrix.setScale( scale );
            core::matrix4 rotationMatrix;
            rotationMatrix.setRotationDegrees(rotation);

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

void CW2ENTMeshFileLoader::computeLocal(ISkinnedMesh::SJoint* joint)
{
    // Get parent
    const core::stringc parentName = searchParent(joint->Name.c_str());
    ISkinnedMesh::SJoint* jointParent = 0;
    if (AnimatedMesh->getJointNumber(parentName.c_str()) != -1)
    {
        jointParent = AnimatedMesh->getAllJoints()[AnimatedMesh->getJointNumber(parentName.c_str())];
    }

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

void CW2ENTMeshFileLoader::make_localMatrix_from_global()
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

bool CW2ENTMeshFileLoader::load(io::IReadFile* file)
{
    int back = file->getPos();

    readString(file, 4); // CR2W

    core::array<s32> header = readDataArray<s32>(file, 10);

    const int fileFormatVersion = header[0];

    // Strings list
    file->seek(back + header[2]);
    for (int i = 0; i < header[3]; ++i)
        Strings.push_back(readString(file, readData<u8>(file) -128));

    log->addAndPush("Table 1 OK\n");

    // This list is the list of all the externals files used by the file
    file->seek(back + header[6]);    
    for (int i = 0; i < header[7]; ++i)
    {
        core::array<u8> format_name = readDataArray<u8>(file, 2);
        unsigned char size      = format_name[0];
        unsigned char offset    = format_name[1];

        file->seek(-1, true);

        if (offset == 1)
            file->seek(1, true);

        core::stringc filename = readString(file, size);
        file->seek(4, true);    //int index = readInts(file, 1)-1;
        // Type of the file (Cmesh, CmaterielInstance...)
        // core::stringc file_type = Strings[index];

        FilesTable.push_back(filename);
    }
    log->addAndPush("Table 2 OK\n");

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
    file->seek(back + header[4]);
    for(int i = 0; i < header[5]; ++i) // Now, the list of all the components of the files
    {
        // Read data
        unsigned short dataTypeId = readData<u16>(file) - 1;    // Data type
        core::stringc dataType = Strings[dataTypeId];

        core::array<s32> data2 = readDataArray<s32>(file, 5);             // Data info (adress...)

        DataInfos infos;
        infos.size = data2[1];
        infos.adress = data2[2];

        core::stringc mesh_source;                              // mesh source ? useless

        if (data2[0] == 0)
        {
            unsigned char size = readData<u8>(file) - 128;

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

        if (!find(chunks, dataType))    //check if 'name' is already in 'chuncks'. If this is not the case, name is added in chunk
            chunks.push_back(dataType);

        // Now we check the type of the data, and if we have a CMaterialInstance or CMesh we read it
        //std::cout << name.c_str() << std::endl;


        // now all stuff readed
        const int back2 = file->getPos();

        if (dataType == "CMaterialInstance")
        {
            log->addAndPush("\nCMaterialInstance\n");

            CMaterialInstance(file, infos, nMat);
            MeshesToLoad[MeshesToLoad.size()-1].nMat.push_back(nMat);
            nMat++;

            log->addAndPush("CMaterialInstance OK\n");
        }
        else if (dataType == "CMesh")
        {
            //std::cout << "CMesh" << std::endl;            
            log->addAndPush("\nCMesh\n");

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

            log->addAndPush("CMesh OK\n");
        }
        else if (dataType == "CSkeleton")
        {
            //std::cout << "CSkeleton" << std::endl;
            CSkeleton(file, infos);
        }
        else if (dataType == "CSkeletalAnimation")
        {
            //std::cout << "CSkeletalAnimation" << std::endl;
        }
        else if (dataType == "CLayer")
        {
            //std::cout << "CLayer" << std::endl;
        }
        file->seek(back2);
    }
    log->addAndPush("Textures and mesh data OK\n");

    // When we have loaded all the materials and all the 'mesh headers', we load the meshes
    for (unsigned int i = 0; i < MeshesToLoad.size(); i++)
    {
        // Create mesh
        CMesh(file, MeshesToLoad[i]);
    }

    log->addAndPush("All is loaded\n");
	return true;
}

bool checkIfBoneDontExist(core::stringc name, irr::core::array<bone_data> datas)
{
    for (int i = 0; i < datas.size(); i++)
    {
        if (datas[i].name == name)
            return false;
    }
    return true;
}

void CW2ENTMeshFileLoader::CSkeleton(io::IReadFile* file, DataInfos infos)
{
    file->seek(infos.adress);

    //std::cout << "begin at " << infos.adress << " and end at " << infos.adress + infos.size << ", so size = " << infos.size << std::endl;
    while(1)
    {
        W2_PropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader))
            break;

        file->seek(propHeader.endPos);
    }
    file->seek(-4, true);
}

void CW2ENTMeshFileLoader::CMesh(io::IReadFile* file, Meshdata tmp)
{
    #ifdef _W2ENTREADER_DEBUG
    std::cout << "Load a mesh..." <<std::endl;
    #endif

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

    int nbBones = readData<u8>(file);

    int back = file->getPos();

    /*
    if  (magic == 1)
        readU8(file);
    else
        rigged = 1;
    */

    u8 magicData = readData<u8>(file);
    if (magicData != 1)
        file->seek(-1, true);

    if (nbBones == 128)
    {
        file->seek(1, true); //readUnsignedChars(file, 1);
        static_meshes(file, mats,nModel);
    }
    else // If the mesh isn't static
    {
        log->addAndPush("Loop\n");

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
            u16 nameId = readData<u16>(file) - 1;

            core::stringc name = "";
            if (nameId < Strings.size())
            {
                name = Strings[nameId];
            }
            else
            {
                log->addAndPush("error_with_bones OK\n");
                name = "bone-";
                name += i;
            }

            bool ok = true;
            for (unsigned int i = 0; i < AnimatedMesh->getJointCount(); i++)
            {
                if (AnimatedMesh->getAllJoints()[i]->Name == name)
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
        nbBones = readData<u8>(file);
        back = file->getPos();
        if (readData<s32>(file) > 128 || readData<s32>(file) > 128)
        {
            if (nbBones != 1)
                file->seek(back + 1);
            else
                file->seek(back);
        }
        else
            file->seek(back);

        readDataArray<s32>(file, nbBones);

        s32 seek = file->getPos() + readData<s32>(file);
        back = file->getPos();
        file->seek(seek);
        core::array<s32> data = readDataArray<s32>(file, 6);
        NbSubMesh = readData<u8>(file);
        SubMeshData.clear();
        for (unsigned int i = 0; i < NbSubMesh; i++)
        {
            Submesh_data s_data;
            s_data.vertype = readData<u8>(file);
            s_data.dataI = readDataArray<s32>(file, 4);
            s_data.dataH = readDataArray<u16>(file, readData<u8>(file)+2);
            SubMeshData.push_back(s_data);

            /*
            if ((s_data.dataI[2] < 0 || s_data.dataI[2] > 100000 || error_with_bones ) && magic == false) // Can crash here
            {
                // We reload with 'the magic button'
                GEOMETRY(file, tmp, true);
                return;
            }
            */
            #ifdef _W2ENTREADER_DEBUG
            std::cout << "Submesh : vertEnd = " << s_data.dataI[2] << ", vertype = "<< s_data.vertype  <<std::endl;
            #endif
        }

        file->seek(back);

        // Skeleton called before drawmesh to avoid crash
        skeleton(file);


        drawmesh(file, data, mats, nModel);

    }
    #ifdef _W2ENTREADER_DEBUG
    std::cout << "Mesh loaded" <<std::endl;
    #endif
}


void CW2ENTMeshFileLoader::static_meshes(io::IReadFile* file, core::array<int> mats, int nModel)
{
    // We read submesh infos
    int back = file->getPos();
    int seek = readData<s32>(file);
    file->seek(back + seek);

    file->seek(4, true); //readUnsignedShorts(file, 2);
    readData<s32>(file);

    core::array<s32> data = readDataArray<s32>(file, 4);
    NbSubMesh = readData<u8>(file);
    SubMeshData.clear();
    int Vert_Type = readData<u8>(file);

    for (unsigned int i = 0; i < NbSubMesh; i++)
    {
        Submesh_data s_data;
        s_data.vertype = Vert_Type;                 // The type of vertice determine the size of a vertices (it depend of the number of informations stored in the vertice)
        s_data.dataI = readDataArray<s32>(file, 5);
        s_data.dataH = readDataArray<u16>(file, 1);
        SubMeshData.push_back(s_data);
    }

    file->seek(back+4);
    drawmesh_static(file, data, mats, nModel);
}


void CW2ENTMeshFileLoader::drawmesh_static(io::IReadFile* file, core::array<int> data, core::array<int> mats, int nModel)
{
    log->addAndPush("Drawmesh_static\n");

    int back = file->getPos();
    const video::SColor defaultColor(255, 255, 255, 255);

    for (unsigned int n = 0; n < NbSubMesh; n++)
    {
        if (n >= IdLOD[0][0])
            continue;

        log->addAndPush("submesh\n");
        SSkinMeshBuffer *buf = AnimatedMesh->addMeshBuffer();

        Submesh_data var = SubMeshData[n];
        //std::cout << "var.vertype = " << var.vertype << std::endl;

        int vertsize = 0;
        if (var.vertype == 0)
            vertsize = 36;
        else if (var.vertype == 6)
            vertsize = 44;
        else if (var.vertype == 9 || var.vertype  == 5)
            vertsize = 60;

        int VertStart = var.dataI[0];
        int VertEnd = var.dataI[2];

        file->seek(back+VertStart*vertsize);
        buf->Vertices_Standard.reallocate(VertEnd);
        for (int i = 0; i < VertEnd; i++)
        {
            int back1 = file->getPos();
            video::S3DVertex vertex;
            core::array<f32> position = readDataArray<f32>(file, 3);
            vertex.Pos = core::vector3df(position[0], position[2], position[1]);

            file->seek(8, true);
            core::array<f32> uv = readDataArray<f32>(file, 2);
            vertex.TCoords = core::vector2df(uv[0], uv[1]);
            vertex.Color = defaultColor;
            buf->Vertices_Standard.push_back(vertex);

            file->seek(back1 + vertsize);
        }
        int FacesStart = var.dataI[1];
        int FacesEnd = var.dataI[3];

        file->seek(back+data[0]+FacesStart*2);

        buf->Indices.set_used(FacesEnd);
        file->read(buf->Indices.pointer(), FacesEnd * 2);


        int result = 0;
        if (n < mats.size())
            if ((unsigned int)mats[n] < Materials.size())
                result = mats[n];
        //std::cout << "MaterialSize= " << Materials.size() << std::endl;
        //std::cout << "Result : " << result << ", mat id : " << Materials[result].id << ", mat[n]" << mats[n] << std::endl;

        buf->Material = Materials[result].material;

        SceneManager->getMeshManipulator()->recalculateNormals(buf);
        buf->recalculateBoundingBox();
    }
    AnimatedMesh->setDirty();

    log->addAndPush("Drawmesh_static OK\n");
}


void CW2ENTMeshFileLoader::drawmesh(io::IReadFile* file, core::array<int> data, core::array<int> mats, int nModel)
{
    log->addAndPush("Drawmesh\n");

    int back = file->getPos();
    const video::SColor defaultColor(255, 255, 255, 255);

    for (unsigned int n = 0; n < NbSubMesh; n++)
    {
        if (n >= IdLOD[0][0])
            continue;

        SSkinMeshBuffer* buf = AnimatedMesh->addMeshBuffer();

        core::array<core::array<u8> > weighting;
        // std::cout << "var.vertype = " << SubMeshData[n].vertype << std::endl;
        int vertsize = 0;
        if (SubMeshData[n].vertype==1)
            vertsize=44;
        else if (SubMeshData[n].vertype==11)
            vertsize=44;
        else
            vertsize=52;

        Submesh_data var = SubMeshData[n];
        int VertStart  = var.dataI[0];
        int VertEnd = var.dataI[2];

        file->seek(back+VertStart*vertsize);
        buf->Vertices_Standard.reallocate(VertEnd);
        weighting.reallocate(VertEnd);
        for (int i = 0; i < VertEnd; i++)
        {
            int back1 = file->getPos();
            video::S3DVertex vertex;
            core::array<f32> position = readDataArray<f32>(file, 3);
            vertex.Pos = core::vector3df(position[0], position[2], position[1]);

            weighting.push_back(readDataArray<u8>(file, 8));

            file->seek(8, true);
            core::array<f32> uv = readDataArray<f32>(file, 2);
            vertex.TCoords = core::vector2df(uv[0], uv[1]);
            vertex.Color = defaultColor;
            buf->Vertices_Standard.push_back(vertex);

            file->seek(back1+vertsize);
        }

        int FacesStart = var.dataI[1];
        int FacesEnd = var.dataI[3];

        // Faces
        file->seek(back+data[2]+FacesStart*2);        
        buf->Indices.set_used(FacesEnd);
        file->read(buf->Indices.pointer(), FacesEnd * 2);


        int result = 0;
        if (n < mats.size())
            if ((unsigned int)mats[n] < Materials.size())
                result = mats[n];

        /*std::cout << "MaterialSize= " << Materials.size() << std::endl;
        std::cout << "Result : " << result << ", mat id : " << Materials[result].id << ", mat[n]" << mats[n] << std::endl;*/

        buf->Material = Materials[result].material;

        make_vertex_group(SubMeshData[n], weighting);

        SceneManager->getMeshManipulator()->recalculateNormals(buf);
        buf->recalculateBoundingBox();
    }

    log->addAndPush("Drawmesh OK\n");
}

video::ITexture* CW2ENTMeshFileLoader::getTexture(core::stringc filename)
{
    video::ITexture* texture = 0;

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

bool CW2ENTMeshFileLoader::ReadPropertyHeader(io::IReadFile* file, W2_PropertyHeader& propHeader)
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

void CW2ENTMeshFileLoader::CMaterialInstance(io::IReadFile* file, DataInfos infos, int nMats)
{
    int back = file->getPos();
    file->seek(infos.adress);

    log->addAndPush("Read material...\n");

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
            file->seek(1, true); //FilesTable[255-readData<u8>(file)];
            file->seek(6, true); //readUnsignedChars(file, 6);

            int nMatElement = readData<s32>(file);

            log->addAndPush(core::stringc("nMatElement = ") + core::stringc(nMatElement) + core::stringc("\n"));

            for (int n = 0; n < nMatElement; n++)
            {
                back = file->getPos();
                int seek = readData<s32>(file);

                u16 propertyIndex = readU16(file) - 1;
                u16 propertyTypeIndex = readU16(file) - 1;

                core::stringc propertyName = Strings[propertyIndex];
                core::stringc propertyType = Strings[propertyTypeIndex];

                if (propertyType == "*ITexture")
                {
                    int imageID = readData<u8>(file);
                    if (imageID>0)
                    {
                        // ImageID is the index of the texture file in the FilesTable
                        //std::cout << "Image ID : " << imageID << ", image name : " << FilesTable[255-imageID] << std::endl;
                        core::stringc texturePath = ConfigGamePath + FilesTable[255-imageID];
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
                        if (propertyName == "specular" || propertyName == "tex_Specular" || propertyName == "Specular" || propertyName == "sptTexSpecular")
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

void CW2ENTMeshFileLoader::convertXBMToDDS(core::stringc xbm_file)
{
    log->addAndPush("XBM to DDS\n");

    // Make the name of the DDS file
    io::path ddsfile;
    core::cutFilenameExtension(ddsfile, xbm_file);
    ddsfile += ".dds";

    // Open the XBM file
    io::IReadFile* fileXBM = FileSystem->createAndOpenFile((xbm_file).c_str());
    if (!fileXBM)
    {
        SceneManager->getParameters()->setAttribute("TW_FEEDBACK", "Some textures havn't been found, check your 'Base directory'.");
        log->addAndPush(core::stringc("Error : the file ") + xbm_file + core::stringc(" can't be opened.\n"));
        return;
    }

    log->addAndPush("XBM file opened\n");


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
        stringsXBM.push_back(readString(fileXBM, readData<u8>(fileXBM)-128));

    log->addAndPush("List ok\n");


    // data
    fileXBM->seek(data[4]);
    for (int i = 0; i < data[5]; i++)
    {
        // The type of the data (cf stringsXBM)
        unsigned short var = readData<u16>(fileXBM);
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

    log->addAndPush("XBM to DDS OK\n");
}

void CW2ENTMeshFileLoader::TEXTURE(io::IReadFile* fileXBM, core::stringc filenameDDS, core::array<int> data, core::array<core::stringc> stringsXBM)
{
    // int back = file->getPos();
    log->addAndPush("CBitmapTexture\n");

    fileXBM->seek(data[2]);

    char ddsheader[] = "\x44\x44\x53\x20\x7C\x00\x00\x00\x07\x10\x0A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x05\x00\x00\x00\x44\x58\x54\x31\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x10\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

    core::stringc dxt;
    int height1, width1;

    // Read the header infos
    while(1)
    {
        log->addAndPush("Read header data...\n");

        // name of the element
        core::stringc propertyName = stringsXBM[readData<u16>(fileXBM)-1];
        // type of the element
        core::stringc propertyType = stringsXBM[readData<u16>(fileXBM)-1];

        fileXBM->seek(2, true);  //readUnsignedChars(fileXBM, 2);
        int back2 = fileXBM->getPos();

        s32 seek = readData<s32>(fileXBM); // size of the property
        // The dimensions of the textures
        if (propertyName == "width" && propertyType == "Uint")
        {
            width1 = readData<s32>(fileXBM);
        }
        else if (propertyName == "height" && propertyType == "Uint")
        {
            height1 = readData<s32>(fileXBM);
        }
        // Compression format
        else if (propertyType == "ETextureCompression")
        {
            dxt = stringsXBM[readData<u16>(fileXBM)-1];

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
    log->addAndPush("Read header ok\n");

    fileXBM->seek(27, true); //readUnsignedChars(fileXBM, 27);

    // If a compression method has been found
    if (dxt.size() > 0)
    {
        // Create the DDS file
        io::IWriteFile* fileDDS = FileSystem->createAndWriteFile((filenameDDS).c_str());

        if (!fileDDS)
        {
            log->addAndPush(core::stringc("Error : the file ") + filenameDDS + core::stringc(" can't be created.\n"));
        }
        else
        {
            log->addAndPush(core::stringc("File ") + filenameDDS + core::stringc(" created.\n"));
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
        log->addAndPush("DDS header OK\n");

        // copy the content of the file
        const long sizeToCopy = fileXBM->getSize() - fileXBM->getPos();


        char* buffer = new char[sizeToCopy];
        fileXBM->read(buffer, sizeToCopy);

        log->addAndPush("Read XBM OK\n");

        fileDDS->write(buffer, sizeToCopy);
        delete[] buffer;

        log->addAndPush("Write DDS OK\n");

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

void CW2ENTMeshFileLoader::vert_format(io::IReadFile* file)
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
core::stringc CW2ENTMeshFileLoader::searchParent(core::stringc bonename)
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


bool CW2ENTMeshFileLoader::find (core::array<core::stringc> stringVect, core::stringc name)
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

