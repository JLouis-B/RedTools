#include "IO_MeshWriter_RE.h"

#include <IMesh.h>
#include <IMeshBuffer.h>
#include <IMeshManipulator.h>
#include <ISceneManager.h>
#include <IWriteFile.h>
#include <IFileSystem.h>
#include <ITexture.h>

#include <iostream>

using namespace std;

namespace irr
{
namespace scene
{

IO_MeshWriter_RE::IO_MeshWriter_RE(scene::ISceneManager* smgr, io::IFileSystem* fs)
	: SceneManager(smgr), FileSystem(fs)
{
	#ifdef _DEBUG
	setDebugName("CREMeshWriter");
	#endif

	if (SceneManager)
		SceneManager->grab();

	if (FileSystem)
		FileSystem->grab();

    clearLODS();
}


IO_MeshWriter_RE::~IO_MeshWriter_RE()
{
	if (SceneManager)
		SceneManager->drop();

	if (FileSystem)
		FileSystem->drop();
}


//! Returns the type of the mesh writer
EMESH_WRITER_TYPE IO_MeshWriter_RE::getType() const
{
    return EMWT_OBJ;
}

core::vector3df IrrToRedVector(core::vector3df irrVector)
{
    return core::vector3df(irrVector.X, irrVector.Z, irrVector.Y);
}

void IO_MeshWriter_RE::createWeightsTable(scene::ISkinnedMesh* mesh)
{
    WeightsTable.clear();
    WeightsTable.resize(mesh->getMeshBufferCount());
    for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
        WeightsTable[i].resize(mesh->getMeshBuffer(i)->getVertexCount());

    const u32 nbJoints = mesh->getJointCount();
    for (u32 i = 0; i < nbJoints; ++i)
    {
        const ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i];
        const u32 nbWeights = joint->Weights.size();
        for (u32 n = 0; n < nbWeights; ++n)
        {
            scene::ISkinnedMesh::SWeight w = joint->Weights[n];
            Weight tmp;
            tmp.boneID = i;
            tmp.w = w;
            WeightsTable[w.buffer_id][w.vertex_id].push_back(tmp);
        }
    }
}

core::array<Weight> getWeightForVertex(scene::ISkinnedMesh* mesh, u32 meshBufferID, u32 vertexID)
{
    core::array<Weight> result;
    const u32 nbJoints = mesh->getJointCount();
    for (u32 i = 0; i < nbJoints; ++i)
    {
        const ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i];
        const u32 nbWeights = joint->Weights.size();
        for (u32 n = 0; n < nbWeights; ++n)
        {
            scene::ISkinnedMesh::SWeight w = joint->Weights[n];
            if (w.buffer_id == meshBufferID && w.vertex_id == vertexID)
            {
                Weight wd;
                wd.w = w;
                wd.boneID = i;
                result.push_back(wd);
            }
        }
    }
    //std::cout << result.size() << std::endl;
    return result;
}

//! writes a mesh
bool IO_MeshWriter_RE::writeAnimatedMesh(io::IWriteFile* file, scene::IMesh* mesh, bool skinned, s32 flags)
{
	if (!file)
		return false;

    ChunksAdress.clear();
    ChunksSize.clear();
    ChunksAdressToWrite.clear();


    u32 nbLOD = 1;
    if (MeshLOD1)
        nbLOD++;
    if (MeshLOD2)
        nbLOD++;
    if (CollisionMesh)
        nbLOD+=CollisionMesh->getMeshBufferCount();



	// Write the header
    u32 nbChunks = nbLOD + 1;
    file->write(&nbChunks, 4);

    file->write("rdeh\x00\x00\x00\x00", 8);

    // updated later
    u32 adress = 0, size = 0;
    ChunksAdressToWrite.push_back(file->getPos());
    file->write(&adress, 4);
    file->write(&size, 4);



	file->write("hsem", 4);
    u32 LODid = 0;
    file->write(&LODid, 4);


    ChunksAdressToWrite.push_back(file->getPos());
    file->write(&adress, 4);
    file->write(&size, 4);


    // LOD1
    if (MeshLOD1)
    {
        file->write("hsem", 4);
        LODid = 1;
        file->write(&LODid, 4);

        ChunksAdressToWrite.push_back(file->getPos());
        file->write(&adress, 4);
        file->write(&size, 4);
    }

    // LOD2
    if (MeshLOD2)
    {
        file->write("hsem", 4);
        LODid = 2;
        file->write(&LODid, 4);

        ChunksAdressToWrite.push_back(file->getPos());
        file->write(&adress, 4);
        file->write(&size, 4);
    }
    if(CollisionMesh)
    {
        // TODO : test if this really works with multiple mesh buffer ?
        for(u32 i = 0; i < CollisionMesh->getMeshBufferCount(); ++i)
        {
            file->write("00lc", 4);
            LODid = i;
            file->write(&LODid, 4);

            ChunksAdressToWrite.push_back(file->getPos());
            file->write(&adress, 4);
            file->write(&size, 4);
        }
    }


    writeHeaderChunk(file);
    writeMeshChunk(file, "LOD0", mesh, skinned);
    if (MeshLOD1)
        writeMeshChunk(file, "LOD1", MeshLOD1, skinned);
    if (MeshLOD2)
        writeMeshChunk(file, "LOD2", MeshLOD2, skinned);
    if(CollisionMesh)
        writeCollisionMeshChunk(file);


    // update the chunks adress/size in the header
    for (u32 i = 0; i < ChunksAdressToWrite.size(); ++i)
    {
        file->seek(ChunksAdressToWrite[i]);
        file->write(&ChunksAdress[i], 4);
        file->write(&ChunksSize[i], 4);
    }

	return true;
}

void IO_MeshWriter_RE::writeHeaderChunk(io::IWriteFile* file)
{
    ChunksAdress.push_back(file->getPos());
    u32 chunkStart = file->getPos();

    core::stringc user = "anonymous_user";
    core::stringc path = FileSystem->getAbsolutePath(file->getFileName());

    // size of the username
    u32 userSize = user.size();
    file->write(&userSize, 4);
    // and the username
    file->write(user.c_str(), user.size());

    // size of the path
    u32 pathSize = path.size();
    file->write(&pathSize, 4);
    // and the path
    file->write(path.c_str(), pathSize);

    ChunksSize.push_back(file->getPos() - chunkStart);
}

void IO_MeshWriter_RE::writeCollisionMeshChunk(io::IWriteFile* file)
{
    ChunksAdress.push_back(file->getPos());
    u32 chunkStart = file->getPos();

    const core::vector3df meshCenter = CollisionMesh->getBoundingBox().getCenter();
    //meshCenter.Y = 0.0f; // not sure why I have forced it to 0 previously

    for (u32 i = 0; i < CollisionMesh->getMeshBufferCount(); ++i)
    {
        const scene::IMeshBuffer* buffer = CollisionMesh->getMeshBuffer(i);

        const core::vector3df meshBufferCenter = buffer->getBoundingBox().getCenter();
        //meshBufferCenter.Y = 0.0f; // not sure why I have forced it to 0 previously

        const core::vector3df center = meshBufferCenter - meshCenter;

        core::stringc objectName = "collisionMesh";
        objectName += i;
        objectName += "_COLL";
        s32 objectNameSize = objectName.size();
        file->write(&objectNameSize, 4);
        // And the path
        file->write(objectName.c_str(), objectNameSize);

        const u32 nbVertices = buffer->getVertexCount();
        const u32 nbTriangles = buffer->getIndexCount()/3;
        const u32 nbU = 1;

        file->write(&nbVertices, 4);
        file->write(&nbTriangles, 4);
        file->write(&nbU, 4);

        for (u32 n = 0; n < nbVertices; ++n)
        {
            const core::vector3df vertexPos = IrrToRedVector(buffer->getPosition(n) - center);

            file->write(&vertexPos.X, 4);
            file->write(&vertexPos.Y, 4);
            file->write(&vertexPos.Z, 4);
        }
        for (u32 n = 0; n < nbTriangles; ++n)
        {
            u32 index1 = buffer->getIndices()[n*3];
            u32 index2 = buffer->getIndices()[n*3 + 1];
            u32 index3 = buffer->getIndices()[n*3 + 2];

            file->write(&index1, 4);
            file->write(&index2, 4);
            file->write(&index3, 4);
            file->write("\x00\x00\x00\x00", 4); // smoothing group ?
        }
        core::stringc str = "dafault";
        s32 strSize = str.size();
        file->write(&strSize, 4);
        // And the path
        file->write(str.c_str(), strSize);

        // maybe the 3 Axis
        file->write("\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?", 36);

        // probably the mesh center
        const core::vector3df redCenter = IrrToRedVector(center);
        file->write(&redCenter.X, 4);
        file->write(&redCenter.Y, 4);
        file->write(&redCenter.Z, 4);
    }

    ChunksSize.push_back(file->getPos() - chunkStart);
}

void IO_MeshWriter_RE::writeMeshChunk(io::IWriteFile* file, core::stringc lodName, IMesh* lodMesh, bool skinned)
{
    ChunksAdress.push_back(file->getPos());
    u32 chunkStart = file->getPos();

    // In the .re format, the mesh needs tangents
    scene::IMesh* tangentMesh = lodMesh;
    bool useSeparateTangentsMesh = false;

    for (u32 i = 0; i < lodMesh->getMeshBufferCount(); ++i)
    {
        if (lodMesh->getMeshBuffer(i)->getVertexType() != video::EVT_TANGENTS)
        {
            tangentMesh = SceneManager->getMeshManipulator()->createMeshWithTangents(lodMesh);
            useSeparateTangentsMesh = true;
            break;
        }
    }

    // name of the LOD
    u32 lodNameSize = lodName.size();
	file->write(&lodNameSize, 4);
	file->write(lodName.c_str(), lodNameSize);

    // nb mesh buffer
    u32 nbMeshBuffer = lodMesh->getMeshBufferCount();
    file->write(&nbMeshBuffer, 4);

    // nb joints
    u32 nbJoints = 0;
    if (skinned)
        nbJoints = ((ISkinnedMesh*)lodMesh)->getJointCount();

    file->write(&nbJoints, 4);

    // maybe the 3 Axis
    file->write("\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?", 36);

    // the offset between the center of the mesh and (0, 0, 0)
    const core::vector3df redMeshCenter = IrrToRedVector(lodMesh->getBoundingBox().getCenter());
    //meshCenter.Y = 0.0f; // not sure why I have forced it to 0 previously
    file->write(&redMeshCenter.X, 4);
    file->write(&redMeshCenter.Y, 4);
    file->write(&redMeshCenter.Z, 4);

    if (skinned)
        createWeightsTable((scene::ISkinnedMesh*)lodMesh);


    for (u32 i=0; i<lodMesh->getMeshBufferCount(); ++i)
	{
        const scene::IMeshBuffer* buffer = lodMesh->getMeshBuffer(i);
        const scene::IMeshBuffer* tangentBuffer = tangentMesh->getMeshBuffer(i); // Potentially not the same as 'buffer' if the mesh has no tangents
        const video::SMaterial material = buffer->getMaterial();

        core::stringc matName = "noname";
        core::stringc diffuseTextureName = "diff";
        const video::ITexture* diffuseTexture = material.getTexture(0);
        if (diffuseTexture)
        {
            diffuseTextureName = FileSystem->getFileBasename(diffuseTexture->getName().getPath(), true);
            diffuseTextureName = diffuseTextureName.make_lower();
            matName = FileSystem->getFileBasename(diffuseTexture->getName().getPath(), false);
            matName = matName.make_lower();
        }
        u32 matNameSize = matName.size();
        u32 diffuseTextureSize = diffuseTextureName.size();

        file->write(&matNameSize, 4);
        file->write(matName.c_str(), matNameSize);

        file->write(&diffuseTextureSize, 4);
        file->write(diffuseTextureName.c_str(), diffuseTextureSize);

        file->write("\x03\x00\x00\x00nor\x03\x00\x00\x00\x62le", 14);

        u32 nbVerticesMeshBuf = buffer->getVertexCount();
        s32 nbFacesMeshBuf = buffer->getIndexCount()/3;

        file->write(&nbVerticesMeshBuf, 4);
        file->write(&nbFacesMeshBuf, 4);


        video::S3DVertexTangents* vertices = (video::S3DVertexTangents*)(tangentBuffer->getVertices());
        for(u32 n=0; n<buffer->getVertexCount(); ++n)
        {
            // The vertex positions are relatives to the mesh center
            const core::vector3df redRelativePos = IrrToRedVector(buffer->getPosition(n));
            file->write(&redRelativePos.X, 4);
            file->write(&redRelativePos.Y, 4);
            file->write(&redRelativePos.Z, 4);

            const core::vector3df redNormal = IrrToRedVector(buffer->getNormal(n));
            file->write(&redNormal.X, 4);
            file->write(&redNormal.Y, 4);
            file->write(&redNormal.Z, 4);


            /*
            core::vector3df vect2 = buffer->getNormal(n);
            core::vector3df vect3 = buffer->getNormal(n);

            core::matrix4 m;
            m.setRotationDegrees(core::vector3df(0, 0, -90));
            m.rotateVect(vect2);

            m.setRotationDegrees(core::vector3df(-90, 0, 0));
            m.rotateVect(vect3);
            */
            // Binormals and tangents ?
            const core::vector3df redTangent = IrrToRedVector(vertices[n].Tangent);
            file->write(&redTangent.X, 4);
            file->write(&redTangent.Y, 4);
            file->write(&redTangent.Z, 4);

            const core::vector3df redBinormal = IrrToRedVector(vertices[n].Binormal);
            file->write(&redBinormal.X, 4);
            file->write(&redBinormal.Y, 4);
            file->write(&redBinormal.Z, 4);

            if (skinned)
            {
                vector<Weight> ws = WeightsTable[i][n];

                file->write("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);

                for(u32 m = 0; m < ws.size(); ++m)
                {
                    if (m == 4)
                    {
                        std::cout << "more than 4" << std::endl;
                        break;
                    }
                    file->write(&(ws[m].w.strength), 4);

                }
                if (ws.size() < 4)
                    for(u32 m = 0; m < 4 - ws.size(); ++m)
                    {
                        file->write("\x00\x00\x00\x00", 4);
                    }
                for(u32 m = 0; m < ws.size(); ++m)
                {
                    if (m == 4)
                    {
                        std::cout << "more than 4" << std::endl;
                        break;
                    }
                    float boneID = ws[m].boneID;
                    file->write(&boneID, 4);

                }
                if (ws.size() < 4)
                    for(u32 m = 0; m < 4 - ws.size(); ++m)
                    {
                        file->write("\x00\x00\x00\x00", 4);
                    }
            }
            else
                file->write("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 48);


            const core::vector2df uv = buffer->getTCoords(n);
            file->write(&uv.X, 4);
            file->write(&uv.Y, 4);

            // The second UV layer
            if (buffer->getVertexType() != video::EVT_2TCOORDS)
                file->write("\x00\x00\x00\x00\x00\x00\x00\x00", 8);
            else
            {
                // std::cout << "WARNING : 2 TCoords" << std::endl;
                const core::vector2df tCoords2 = ((video::S3DVertex2TCoords*)buffer->getVertices())[n].TCoords2;
                file->write(&tCoords2.X, 4);
                file->write(&tCoords2.Y, 4);
            }
        }

        for(u32 n=0; n<buffer->getIndexCount()/3; n++)
        {
            const u32 index1 = buffer->getIndices()[n*3];
            const u32 index2 = buffer->getIndices()[n*3 + 1];
            const u32 index3 = buffer->getIndices()[n*3 + 2];

            file->write(&index1, 4);
            file->write(&index3, 4);
            file->write(&index2, 4);
            file->write("\x00\x00\x00\x00", 4);
        }
	}

    // This part isn't well tested
    if (skinned)
    {
        scene::ISkinnedMesh* skinMesh = ((scene::ISkinnedMesh*)lodMesh);

        for (u32 i = 0; i < nbJoints; ++i)
        {
            const scene::ISkinnedMesh::SJoint* joint = skinMesh->getAllJoints()[i];

            u32 jointSizeName = joint->Name.size();
            file->write(&jointSizeName, 4);

            core::stringc jointName = joint->Name;
            file->write(jointName.c_str(), jointSizeName);


            core::matrix4 transformations = joint->GlobalMatrix;

            core::matrix4 posMat;
            core::vector3df redBonePos = IrrToRedVector(joint->Animatedposition) - redMeshCenter;
            posMat.setTranslation(redBonePos);

            core::matrix4 rotMat;
            core::vector3df redBoneRot;
            joint->Animatedrotation.toEuler(redBoneRot);
            redBoneRot = IrrToRedVector(redBoneRot);
            rotMat.setRotationDegrees(redBoneRot);

            core::matrix4 scaleMat;
            core::vector3df redBoneScale = IrrToRedVector(joint->Animatedscale);
            scaleMat.setScale(redBoneScale);

            transformations = posMat * rotMat * scaleMat;


            //std::cout << "Translation : X="<< transformations.getTranslation().X << ", Y="<< transformations.getTranslation().Y << ", Z="<< transformations.getTranslation().Z << std::endl;
            //std::cout << "Rotation : X="<< transformations.getRotationDegrees().X << ", Y="<< transformations.getRotationDegrees().Y << ", Z="<< transformations.getRotationDegrees().Z << std::endl;
            //std::cout << "Scale : X="<<transformations.getScale().X << ", Y="<< transformations.getScale().Y << ", Z="<< transformations.getScale().Z << std::endl;
            //std::cout << "Extra data="<<transformations[3] << ", "<< transformations[7] << ", "<< transformations[11] << std::endl;

            f32 m0, m1, m2, m4, m5, m6, m8, m9, m10, m12, m13, m14;
            m0 = transformations[0];
            m1 = transformations[1];
            m2 = transformations[2];

            m4 = transformations[4];
            m5 = transformations[5];
            m6 = transformations[6];

            m8 = transformations[8];
            m9 = transformations[9];
            m10 = transformations[10];

            m12 = transformations[12];
            m13 = transformations[13];
            m14 = transformations[14];


            file->write(&m0, 4);
            file->write(&m1, 4);
            file->write(&m2, 4);

            file->write(&m4, 4);
            file->write(&m5, 4);
            file->write(&m6, 4);

            file->write(&m8, 4);
            file->write(&m9, 4);
            file->write(&m10, 4);

            file->write(&m12, 4);
            file->write(&m13, 4);
            file->write(&m14, 4);

        }
    }

    if (useSeparateTangentsMesh)
        tangentMesh->drop();

    ChunksSize.push_back(file->getPos() - chunkStart);
}



void IO_MeshWriter_RE::setLOD1(scene::IMesh* lod1)
{
    MeshLOD1 = lod1;
}

void IO_MeshWriter_RE::setLOD2(scene::IMesh* lod2)
{
    MeshLOD2 = lod2;
}

void IO_MeshWriter_RE::setCollisionMesh(scene::IMesh *mesh)
{
    CollisionMesh = mesh;
}

void IO_MeshWriter_RE::clearLODS()
{
    MeshLOD1 = nullptr;
    MeshLOD2 = nullptr;
    CollisionMesh = nullptr;
}

bool IO_MeshWriter_RE::writeMesh(io::IWriteFile* file, scene::IMesh* mesh, s32 flags)
{
    // Call writeAnimatedMesh.
    bool skinned = false;
    if (mesh->getMeshType() == EAMT_SKINNED)
        skinned = true;

    return writeAnimatedMesh(file, (scene::ISkinnedMesh*)mesh, skinned, flags);
}


} // end namespace
} // end namespace

