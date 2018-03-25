// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

//#include "IrrCompileConfig.h"

// -------------
#define _IRR_COMPILE_WITH_RE_WRITER_

#ifdef _IRR_COMPILE_WITH_RE_WRITER_

#include "IO_MeshWriter_RE.h"
//#include "os.h"
#include "IMesh.h"
#include "IMeshBuffer.h"
#include "IMeshManipulator.h"
#include "IAttributes.h"
#include "ISceneManager.h"
#include "IMeshCache.h"
#include "IWriteFile.h"
#include "IFileSystem.h"
#include "ITexture.h"

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


void IO_MeshWriter_RE::createWeightsTable(ISkinnedMesh* mesh)
{
    _table.clear();
    _table.resize(mesh->getMeshBufferCount());
    for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
        _table[i].resize(mesh->getMeshBuffer(i)->getVertexCount());

    const u32 nbJoints = mesh->getJointCount();
    for (u32 i = 0; i < nbJoints; ++i)
    {
        const ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i];
        const u32 nbWeights = joint->Weights.size();
        for (u32 n = 0; n < nbWeights; ++n)
        {
            irr::scene::ISkinnedMesh::SWeight w = joint->Weights[n];
            Weight tmp;
            tmp.boneID = i;
            tmp.w = w;
            _table[w.buffer_id][w.vertex_id].push_back(tmp);
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
            irr::scene::ISkinnedMesh::SWeight w = joint->Weights[n];
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


int IO_MeshWriter_RE::getJointsSize(ISkinnedMesh* mesh)
{
    // matrix = 4*12 + 4 for the name size
    int size = mesh->getJointCount() * 52;

    for (unsigned int i = 0; i < mesh->getJointCount(); i++)
    {
        size += mesh->getAllJoints()[i]->Name.size();
    }

    return size;
}

int IO_MeshWriter_RE::getMaterialsSize(IMesh* mesh)
{
    int size = 0;
    for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        size += 30; // 22 octets fixes dans un mateirl


        irr::core::stringc matName = "noname";
        irr::core::stringc diffuseTexture = "diff";
        if (mesh->getMeshBuffer(i)->getMaterial().getTexture(0))
        {
            diffuseTexture = FileSystem->getFileBasename(mesh->getMeshBuffer(i)->getMaterial().getTexture(0)->getName().getPath(), true);
            //irr::core::stringc mat = mesh->getMeshBuffer(0)->getMaterial().getTexture(0).getName().getPath();
            matName = FileSystem->getFileBasename(mesh->getMeshBuffer(i)->getMaterial().getTexture(0)->getName().getPath(), false);

            //core::cutFilenameExtension(matName, mat);
        }
        u32 matNameSize = matName.size();
        u32 diffSize = diffuseTexture.size();

        size += matNameSize;
        size += diffSize;
    }

    return size;
}

//! writes a mesh
bool IO_MeshWriter_RE::writeAnimatedMesh(io::IWriteFile* file, scene::IMesh* mesh, bool skinned, s32 flags)
{
	if (!file)
		return false;



    u32 nbLOD = 1;
    if (MeshLOD1)
        nbLOD++;
    if (MeshLOD2)
        nbLOD++;
    if (CollisionMesh)
        nbLOD+=CollisionMesh->getMeshBufferCount();

    // Compute the number of vertices and faces
    s32 nbVertices = 0;
	s32 nbFaces = 0;

	for (u32 i=0; i<mesh->getMeshBufferCount(); ++i)
	{
	    nbVertices += mesh->getMeshBuffer(i)->getVertexCount();
	    nbFaces += mesh->getMeshBuffer(i)->getIndexCount()/3;
	}


	//os::Printer::log("Writing mesh", file->getFileName());

	// write Red Engine MESH header
	io::path name;
	core::cutFilenameExtension(name,file->getFileName()) += ".re";

	core::stringc user = "anonymous_user";
	core::stringc path = FileSystem->getAbsolutePath(file->getFileName());

	// Write the header
    u32 nbLOD2 = nbLOD + 1;
	file->write(&nbLOD2, 4);

    file->write("rdeh\x00\x00\x00\x00", 8);
    u32 headerSize = 20 + nbLOD * 16;
    file->write(&headerSize, 4);



	s32 nb = 8 + user.size() + path.size();
	file->write(&nb, 4);

	file->write("hsem", 4);
    u32 LODid = 0;
    file->write(&LODid, 4);

	// The adress of the LOD0
	s32 adress = 44 + user.size() + path.size();
	if (MeshLOD1)
        adress += 16; // Un LOD prend 16 octets dans le header
	if (MeshLOD2)
        adress += 16; // Un LOD prend 16 octets dans le header
    if (CollisionMesh)
        adress += CollisionMesh->getMeshBufferCount() * 16;
    file->write(&adress, 4);

    // vertex = 112 octets, face = 16 octets, and there is 104 octets between the end of the path and the begin of vertices declaration
	//int dataB = nbVertices * 112 + nbFaces * 16 + 104;
    s32 LODsize = nbVertices * 112 + nbFaces * 16 + 64 + getMaterialsSize(mesh);
    if(skinned)
        LODsize += getJointsSize((ISkinnedMesh*)mesh);
    //LODsize -=4;
    file->write(&LODsize, 4);

    // LOD1
    if (MeshLOD1)
    {
        file->write("hsem", 4);
        LODid = 1;
        file->write(&LODid, 4);
        adress = adress + LODsize;
        file->write(&adress, 4);
        nbVertices = 0;
        nbFaces = 0;
        if (MeshLOD1)
            for (u32 i=0; i<MeshLOD1->getMeshBufferCount(); ++i)
            {
                nbVertices += MeshLOD1->getMeshBuffer(i)->getVertexCount();
                nbFaces += MeshLOD1->getMeshBuffer(i)->getIndexCount()/3;
            }


        LODsize = nbVertices * 112 + nbFaces * 16 + 64 + getMaterialsSize(MeshLOD1);
        if(skinned)
              LODsize += getJointsSize((scene::ISkinnedMesh*)MeshLOD1);
        //LODsize -=4;
        file->write(&LODsize, 4);
    }

    // LOD2
    if (MeshLOD2)
    {
        file->write("hsem", 4);
        LODid = 2;
        file->write(&LODid, 4);
        adress = adress + LODsize;
        file->write(&adress, 4);
        nbVertices = 0;
        nbFaces = 0;
        if (MeshLOD2)
            for (u32 i=0; i<MeshLOD2->getMeshBufferCount(); ++i)
            {
                nbVertices += MeshLOD2->getMeshBuffer(i)->getVertexCount();
                nbFaces += MeshLOD2->getMeshBuffer(i)->getIndexCount()/3;
            }

        LODsize = nbVertices * 12 + nbFaces * 16 + 64 + getMaterialsSize(MeshLOD2);
        if(skinned)
               LODsize += getJointsSize((scene::ISkinnedMesh*)MeshLOD2);
        //LODsize -=4;
        file->write(&LODsize, 4);
    }
    if(CollisionMesh)
    {
        for(u32 i = 0; i < CollisionMesh->getMeshBufferCount(); ++i)
        {
            file->write("00lc", 4);
            LODid = i;
            file->write(&LODid, 4);
            adress = adress + LODsize;
            file->write(&adress, 4);
            nbVertices = CollisionMesh->getMeshBuffer(i)->getVertexCount();
            nbFaces = CollisionMesh->getMeshBuffer(i)->getIndexCount()/3;
            LODsize = nbVertices * 12 + nbFaces * 16 + 81 + 4;
            if (i > 9)
                LODsize++;
            if (i > 99)
                LODsize++;
            if (i > 999)
                LODsize++;
            file->write(&LODsize, 4);
        }
    }

    // Size of the username
    s32 userSize = user.size();
    file->write(&userSize, 4);
    // And the username
	file->write(user.c_str(), user.size());

	// Size of the path
	s32 pathSize = path.size();
	file->write(&pathSize, 4);
	// And the path
    file->write(path.c_str(), pathSize);

    writeLOD(file, "LOD0", mesh, skinned);
    if (MeshLOD1)
        writeLOD(file, "LOD1", MeshLOD1, skinned);
    if (MeshLOD2)
        writeLOD(file, "LOD2", MeshLOD2, skinned);
    if(CollisionMesh)
        writeCollisionMesh(file);


	return true;
}


void IO_MeshWriter_RE::writeCollisionMesh(io::IWriteFile* file)
{
    core::vector3df meshCenter = ((CollisionMesh->getBoundingBox().MaxEdge - CollisionMesh->getBoundingBox().MinEdge)/2) - CollisionMesh->getBoundingBox().MaxEdge;
    meshCenter.Y = 0.0f;




    for (u32 i = 0; i < CollisionMesh->getMeshBufferCount(); ++i)
    {
        core::vector3df meshBufferCenter = ((CollisionMesh->getMeshBuffer(i)->getBoundingBox().MaxEdge - CollisionMesh->getMeshBuffer(i)->getBoundingBox().MinEdge)/2) - CollisionMesh->getBoundingBox().MaxEdge;
        meshBufferCenter.Y = 0.0f;

        core::vector3df center = meshBufferCenter - meshCenter;

        core::stringc objectName = "collisionMesh";
        objectName += i;
        objectName += "_COLL";
        s32 objectNameSize = objectName.size();
        file->write(&objectNameSize, 4);
        // And the path
        file->write(objectName.c_str(), objectNameSize);

        const u32 nbVertices = CollisionMesh->getMeshBuffer(i)->getVertexCount();
        const u32 nbTriangles = CollisionMesh->getMeshBuffer(i)->getIndexCount()/3;
        const u32 nbU = 1;

        file->write(&nbVertices, 4);
        file->write(&nbTriangles, 4);
        file->write(&nbU, 4);

        for (u32 n = 0; n < nbVertices; ++n)
        {
            core::vector3df vertexPos = CollisionMesh->getMeshBuffer(i)->getPosition(n) - center;

            file->write(&vertexPos.X, 4);
            file->write(&vertexPos.Z, 4);
            file->write(&vertexPos.Y, 4);
        }
        for (u32 n = 0; n < nbTriangles; n = n+3)
        {
            file->write(&CollisionMesh->getMeshBuffer(i)->getIndices()[n], 4);
            file->write(&CollisionMesh->getMeshBuffer(i)->getIndices()[n + 1], 4);
            file->write(&CollisionMesh->getMeshBuffer(i)->getIndices()[n + 2], 4);
            file->write("\x00\x00\x00\x00", 4); // smothing group ?
        }
        core::stringc str = "dafault";
        s32 strSize = str.size();
        file->write(&strSize, 4);
        // And the path
        file->write(str.c_str(), strSize);

        // maybe the 3 Axis
        file->write("\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?", 36);

        // TODO : 3 floats.
        //Maybe the mesh center

        float f1 = center.X, f2 = center.Z, f3 = center.Y;
        file->write(&f1, 4);
        file->write(&f2, 4);
        file->write(&f3, 4);
    }

}

void IO_MeshWriter_RE::writeLOD(io::IWriteFile* file, core::stringc lodName, IMesh* lodMesh, bool skinned)
{
    // In the .re format, the mesh need tangents
    scene::IMesh* tangentMesh = lodMesh;
    for (u32 i = 0; i < lodMesh->getMeshBufferCount(); ++i)
    {
        if (lodMesh->getMeshBuffer(i)->getVertexType() != video::EVT_TANGENTS)
        {
            tangentMesh = SceneManager->getMeshManipulator()->createMeshWithTangents(lodMesh);
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

    // the decalage between the center of the mesh and (0, 0, 0)
    core::vector3df meshCenter = ((lodMesh->getBoundingBox().MaxEdge - lodMesh->getBoundingBox().MinEdge)/2) - lodMesh->getBoundingBox().MaxEdge;
    meshCenter.Y = 0.0f;

    f32 Xcenter, Ycenter, Zcenter;
    Xcenter = meshCenter.X;
    Ycenter = meshCenter.Y;
    Zcenter = meshCenter.Z;

    // Write it in the file
    file->write(&Xcenter, 4);
    /*file->write(&Ycenter, 4);
    file->write(&Zcenter, 4);*/
    file->write(&Zcenter, 4);
    file->write(&Ycenter, 4);

    if (skinned)
        createWeightsTable((ISkinnedMesh*)lodMesh);


    // The material
    // file->write("\x06\x00\x00\x00noname\x04\x00\x00\x00\x64iff\x03\x00\x00\x00nor\x03\x00\x00\x00\x62le", 32);
    for (u32 i=0; i<lodMesh->getMeshBufferCount(); ++i)
	{
        core::stringc matName = "noname";
        core::stringc diffuseTexture = "diff";
        if (lodMesh->getMeshBuffer(i)->getMaterial().getTexture(0))
        {
            diffuseTexture = FileSystem->getFileBasename(lodMesh->getMeshBuffer(i)->getMaterial().getTexture(0)->getName().getPath(), true);
            diffuseTexture = diffuseTexture.make_lower();
            matName = FileSystem->getFileBasename(lodMesh->getMeshBuffer(i)->getMaterial().getTexture(0)->getName().getPath(), false);
            matName = matName.make_lower();

            //core::cutFilenameExtension(matName, mat);
        }
        u32 matNameSize = matName.size();
        u32 diffSize = diffuseTexture.size();

        file->write(&matNameSize, 4);
        file->write(matName.c_str(), matNameSize);

        file->write(&diffSize, 4);
        file->write(diffuseTexture.c_str(), diffSize);

        file->write("\x03\x00\x00\x00nor\x03\x00\x00\x00\x62le", 14);

        s32 nbVerticesMeshBuf = lodMesh->getMeshBuffer(i)->getVertexCount();
        s32 nbFacesMeshBuf = lodMesh->getMeshBuffer(i)->getIndexCount()/3;

        file->write(&nbVerticesMeshBuf, 4);
        file->write(&nbFacesMeshBuf, 4);

        //SceneManager->getMeshManipulator()->recalculateNormals(lodMesh);

        // write mesh buffers
        // And now the vertex
        video::S3DVertexTangents* verts = (video::S3DVertexTangents*)(tangentMesh->getMeshBuffer(i)->getVertices());
        for(u32 n=0; n<lodMesh->getMeshBuffer(i)->getVertexCount(); ++n)
        {
            // The vertex positions are relatives to the mesh center
            irr::core::vector3df relativePos = lodMesh->getMeshBuffer(i)->getPosition(n) - meshCenter;

            f32 x = relativePos.X;
            f32 y = relativePos.Y;
            f32 z = relativePos.Z;

            file->write(&x, 4);
            /*file->write(&y, 4);
            file->write(&z, 4);*/
            file->write(&z, 4);
            file->write(&y, 4);
            // Y and Z axis seem don't be the same that the Irrlicht axis


            f32 u = lodMesh->getMeshBuffer(i)->getTCoords(n).X;
            f32 v = lodMesh->getMeshBuffer(i)->getTCoords(n).Y;


            f32 nx = lodMesh->getMeshBuffer(i)->getNormal(n).X;
            f32 ny = lodMesh->getMeshBuffer(i)->getNormal(n).Y;
            f32 nz = lodMesh->getMeshBuffer(i)->getNormal(n).Z;

            file->write(&nx, 4);
            file->write(&nz, 4);
            file->write(&ny, 4);


            /*
            irr::core::vector3df vect2 = mesh->getMeshBuffer(i)->getNormal(n);
            irr::core::vector3df vect3 = mesh->getMeshBuffer(i)->getNormal(n);

            irr::core::matrix4 m;
            m.setRotationDegrees(irr::core::vector3df(0, 0, -90));
            m.rotateVect(vect2);

            m.setRotationDegrees(irr::core::vector3df(-90, 0, 0));
            m.rotateVect(vect3);

            float v2x = vect2.X;
            float v2y = vect2.Y;
            float v2z = vect2.Z;

            float v3x = vect3.X;
            float v3y = vect3.Y;
            float v3z = vect3.Z;
            */



            // Binormals and tangeants ?
            f32 v2x = (verts)[n].Tangent.X;
            f32 v2y = (verts)[n].Tangent.Y;
            f32 v2z = (verts)[n].Tangent.Z;

            f32 v3x = (verts)[n].Binormal.X;
            f32 v3y = (verts)[n].Binormal.Y;
            f32 v3z = (verts)[n].Binormal.Z;

            file->write(&v2x, 4);
            file->write(&v2z, 4);
            file->write(&v2y, 4);

            file->write(&v3x, 4);
            file->write(&v3z, 4);
            file->write(&v3y, 4);

            if (skinned)
            {
                //core::array<Weight> ws =  getWeightForVertex((ISkinnedMesh*)lodMesh, i, n);
                vector<Weight> ws = _table[i][n];


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

            /* Older versions

            file->write("\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 72);

            file->write("\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 84);

            \x00\x00\x80\xbf\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00
            file->write("\x00\x00\x80\xbf\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 84);
            */


            file->write(&u, 4);
            file->write(&v, 4);

            // The second UV layer
            if (lodMesh->getMeshBuffer(i)->getVertexType() != video::EVT_2TCOORDS)
                file->write("\x00\x00\x00\x00\x00\x00\x00\x00", 8);
            else
            {
                // std::cout << "WARNING : 2 TCoords" << std::endl;
                core::vector2df tCoords2 = ((video::S3DVertex2TCoords*)lodMesh->getMeshBuffer(i)->getVertices())[n].TCoords2;
                file->write(&tCoords2.X, 4);
                file->write(&tCoords2.Y, 4);
            }
        }


        // And the indices

        for(u32 n=0; n<lodMesh->getMeshBuffer(i)->getIndexCount()/3; n++)
        {
            /*
            u16 index1 = mesh->getMeshBuffer(i)->getIndices()[n*3];
            u16 index2 = mesh->getMeshBuffer(i)->getIndices()[n*3 + 1];
            u16 index3 = mesh->getMeshBuffer(i)->getIndices()[n*3 + 2];

            file->write(&index1, 2);
            file->write("\x00\x00", 2);
            file->write(&index3, 2);
            file->write("\x00\x00", 2);
            file->write(&index2, 2);
            file->write("\x00\x00", 2);

            file->write("\x00\x00\x00\x00", 4);*/

            // s32 version. The fist implementation

            s32 index1 = lodMesh->getMeshBuffer(i)->getIndices()[n*3];
            s32 index2 = lodMesh->getMeshBuffer(i)->getIndices()[n*3 + 1];
            s32 index3 = lodMesh->getMeshBuffer(i)->getIndices()[n*3 + 2];

            file->write(&index1, 4);
            file->write(&index3, 4);
            file->write(&index2, 4);
            //if (n<(lodMesh->getMeshBuffer(i)->getIndexCount()/3)-1)
            file->write("\x00\x00\x00\x00", 4);
        }
	}


    ISkinnedMesh* skinMesh;
    if (skinned)
        skinMesh = ((ISkinnedMesh*)lodMesh);

    for (u32 i = 0; i < nbJoints; ++i)
    {
        u32 jointSizeName = skinMesh->getAllJoints()[i]->Name.size();
        file->write(&jointSizeName, 4);

        core::stringc jointName = skinMesh->getAllJoints()[i]->Name;
        file->write(jointName.c_str(), jointSizeName);

        core::matrix4 transformations = skinMesh->getAllJoints()[i]->GlobalMatrix;





        core::matrix4 posMat;
        core::vector3df pos = skinMesh->getAllJoints()[i]->Animatedposition;
        pos -= meshCenter;
        double tmp = pos.Y;
        pos.Y = pos.Z;
        pos.Z = tmp;
        posMat.setTranslation(pos);

        core::matrix4 rotMat;
        core::vector3df rot;
        skinMesh->getAllJoints()[i]->Animatedrotation.toEuler(rot);
        tmp = rot.Y;
        rot.Y = rot.Z;
        rot.Z = tmp;
        rotMat.setRotationDegrees(rot);

        core::matrix4 scaleMat;
        core::vector3df scale = skinMesh->getAllJoints()[i]->Animatedscale;
        tmp = scale.Y;
        scale.Y = scale.Z;
        scale.Z = tmp;
        scaleMat.setScale(scale);

        transformations = posMat * rotMat * scaleMat;



        //std::cout << "Translation : X="<< transformations.getTranslation().X << ", Y="<< transformations.getTranslation().Y << ", Z="<< transformations.getTranslation().Z << std::endl;
        //std::cout << "Rotation : X="<< transformations.getRotationDegrees().X << ", Y="<< transformations.getRotationDegrees().Y << ", Z="<< transformations.getRotationDegrees().Z << std::endl;
        //std::cout << "Scale : X="<<transformations.getScale().X << ", Y="<< transformations.getScale().Y << ", Z="<< transformations.getScale().Z << std::endl;
        //std::cout << "Extra data="<<transformations[3] << ", "<< transformations[7] << ", "<< transformations[11] << std::endl;

        float m0, m1, m2, m4, m5, m6, m8, m9, m10, m12, m13, m14;
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
    //file->seek(-4, true);

}



void IO_MeshWriter_RE::setLOD1(IMesh* lod1)
{
    MeshLOD1 = lod1;
}

void IO_MeshWriter_RE::setLOD2(IMesh* lod2)
{
    MeshLOD2 = lod2;
}

void IO_MeshWriter_RE::setCollisionMesh(IMesh *mesh)
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

#endif

