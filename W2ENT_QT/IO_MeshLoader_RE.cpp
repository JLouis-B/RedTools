#include "IO_MeshLoader_RE.h"

#include <ISceneManager.h>
#include <IVideoDriver.h>
#include <IFileSystem.h>
#include <IReadFile.h>

#include "Utils_Loaders_Irr.h"

namespace irr
{
namespace scene
{

//! Constructor
IO_MeshLoader_RE::IO_MeshLoader_RE(scene::ISceneManager* smgr, io::IFileSystem* fs)
: Lod1Mesh(nullptr),
  Lod2Mesh(nullptr),
  CollisionMesh(nullptr),
  SceneManager(smgr),
  FileSystem(fs)
{
	#ifdef _DEBUG
	setDebugName("CREMeshFileLoader");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool IO_MeshLoader_RE::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "re" );
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* IO_MeshLoader_RE::createMesh(io::IReadFile* f)
{
	if (!f)
        return nullptr;

    log = Log::Instance();

    if (log->isEnabled())
    {
        log->addLine("");
        log->addLine(formatString("-> File : %s", f->getFileName().c_str()));
        log->add("_________________________________________________________\n\n\n");
        log->addLine("Start loading");
        log->flush();
    }

    AnimatedMesh = SceneManager->createSkinnedMesh();

	if (load(f))
	{
	    //AnimatedMesh->recalculateBoundingBox();
		AnimatedMesh->finalize();
		//SceneManager->getMeshManipulator()->recalculateNormals(AnimatedMesh);

        if (Lod1Mesh)
            Lod1Mesh->finalize();
        if (Lod2Mesh)
            Lod2Mesh->finalize();
        if (CollisionMesh)
            CollisionMesh->finalize();
	}
	else
	{
		AnimatedMesh->drop();
        AnimatedMesh = nullptr;
	}

    log->addLineAndFlush("Loaded");

	return AnimatedMesh;
}





bool IO_MeshLoader_RE::load(io::IReadFile* f)
{
    if (Lod1Mesh)
    {
        Lod1Mesh->drop();
        Lod1Mesh = nullptr;
    }
    if (Lod2Mesh)
    {
        Lod2Mesh->drop();
        Lod2Mesh = nullptr;
    }
    if (CollisionMesh)
    {
        CollisionMesh->drop();
        CollisionMesh = nullptr;
    }

    u32 nbChunks = readU32(f);
    core::array<ChunkInfos> chunks;
    for (u32 i = 0; i < nbChunks; ++i)
    {
        ChunkInfos c;

        core::stringc chunkTypeString = readString(f, 4);
        if (chunkTypeString == "rdeh")
            c.Type = Chunk_Header;
        if (chunkTypeString == "hsem")
            c.Type = Chunk_Mesh;
        if (chunkTypeString == "00lc")
            c.Type = Chunk_Collision;

        f->read(&c.Id, 4);
        f->read(&c.Adress, 4);
        f->read(&c.Size, 4);

        chunks.push_back(c);
    }

    for (u32 i = 0; i < chunks.size(); ++i)
    {
        const ChunkInfos chunk = chunks[i];
        if (chunk.Type == Chunk_Header)
        {
            f->seek(chunk.Adress);
            readHeaderChunk(f);
        }
        else if (chunk.Type == Chunk_Mesh)
        {
            #ifdef COMPILE_WITH_LODS_SUPPORT
                f->seek(chunk._adress);
                readMeshChunk(f, chunk._id);
            #else
                // load only the first LOD
                if (chunk.Id == 0)
                {
                    f->seek(chunk.Adress);
                    readMeshChunk(f, 0);
                }
            #endif
        }
        else if (chunk.Type == Chunk_Collision)
        {
            #ifdef COMPILE_WITH_LODS_SUPPORT
                f->seek(chunk.Adress);
                readCollisionMeshChunk(f);
            #endif
        }
    }

    return true;
}

void IO_MeshLoader_RE::readMeshChunk(io::IReadFile* f, u32 id)
{
    log->addLineAndFlush("Read MESH chunk");

    // set the "currentLODMesh"
    scene::ISkinnedMesh* currentLODMesh = nullptr;
    if (id == 0)
        currentLODMesh = AnimatedMesh;
    else if (id == 1)
    {
        Lod1Mesh = SceneManager->createSkinnedMesh();
        currentLODMesh = Lod1Mesh;
    }
    else if (id == 2)
    {
        Lod2Mesh = SceneManager->createSkinnedMesh();
        currentLODMesh = Lod2Mesh;
    }
    if (!currentLODMesh)
        return;


    u32 sizeLODname = readU32(f);
    core::stringc LODname = readString(f, sizeLODname);
    log->addLineAndFlush(formatString("Read LOD : %s", LODname.c_str()));


    core::array<WeightT> weightTable;

    s32 nbMeshBuffer = readS32(f);
    s32 nbBones = readS32(f);
    log->addLine(formatString("nbMeshBuffer : %d", nbMeshBuffer));
    log->addLineAndFlush(formatString("nbBones : %d", nbBones));

    f->seek(36, true); // 3 axis

    core::vector3df meshCenter;
    f->read(&meshCenter.X, 4);
    f->read(&meshCenter.Z, 4);
    f->read(&meshCenter.Y, 4);

    for (int n = 0; n < nbMeshBuffer; n++)
    {
        SSkinMeshBuffer* buf = currentLODMesh->addMeshBuffer();
        s32 sizeMatName = readS32(f);
        core::stringc matName = readString(f, sizeMatName);
        log->addLineAndFlush(formatString("matname : %s", matName.c_str()));

        s32 sizeMatDiff = readS32(f);
        core::stringc matDiff = readString(f, sizeMatDiff);
        log->addLineAndFlush(formatString("diffuse texture : %s", matName.c_str()));

        video::ITexture* tex = SceneManager->getVideoDriver()->getTexture(matDiff);
        if (tex)
            buf->Material.setTexture(0, tex);
        else
        {
            tex = SceneManager->getVideoDriver()->getTexture(FileSystem->getFileDir(f->getFileName()) + "/" + matDiff);
            if (tex)
                buf->Material.setTexture(0, tex);
        }


        s32 sizeMatNor = readS32(f);
        core::stringc matNor = readString(f, sizeMatNor);
        log->addLineAndFlush(formatString("normals texture : %s", matName.c_str()));

        s32 sizeMatBle = readS32(f);
        core::stringc matBle = readString(f, sizeMatBle);
        log->addLineAndFlush(formatString("ble texture : %s", matName.c_str()));


        s32 nbVertices = readS32(f);
        s32 nbFaces = readS32(f);
        if (log->isEnabled())
        {
            log->addLine(formatString("nbVertices : %d", nbVertices));
            log->addLine(formatString("nbFaces : %d", nbFaces));
            log->addLine(formatString("pos : %d", f->getPos()));
            log->flush();
        }

        buf->Vertices_Standard.set_used(nbVertices);
        for (s32 i = 0; i < nbVertices; i++)
        {
            f32 x, y, z, u, v, nx, ny, nz, bx, by, bz, tx, ty, tz;
            f->read(&x, 4);
            f->read(&z, 4);
            f->read(&y, 4);

            // Vertex normal
            f->read(&nx, 4);
            f->read(&nz, 4);
            f->read(&ny, 4);


            // binormal ?
            f->read(&bx, 4);
            f->read(&bz, 4);
            f->read(&by, 4);

            // tangent ?
            f->read(&tx, 4);
            f->read(&tz, 4);
            f->read(&ty, 4);


            f->seek(16, true);
            core::array<f32> strenghts = readDataArray<f32>(f, 4);
            for (u32 ns = 0; ns < strenghts.size(); ++ns)
            {
                if (strenghts[ns] != 0.0f)
                {
                    WeightT tmp;
                    tmp.MeshBufferID = n;
                    tmp.VertexID = i;
                    tmp.Strenght = strenghts[ns];
                    tmp.BoneID = readF32(f);
                    weightTable.push_back(tmp);
                }
                else
                    readF32(f);
            }
            // UV
            //f->seek(48, true);

            f->read(&u, 4);
            f->read(&v, 4);
            f->seek(8, true);


            buf->Vertices_Standard[i].Pos.X = x;
            buf->Vertices_Standard[i].Pos.Y = y;
            buf->Vertices_Standard[i].Pos.Z = z;

            buf->Vertices_Standard[i].TCoords.X = u;
            buf->Vertices_Standard[i].TCoords.Y = v;

            buf->Vertices_Standard[i].Normal.X = nx;
            buf->Vertices_Standard[i].Normal.Y = ny;
            buf->Vertices_Standard[i].Normal.Z = nz;

            /*
            buf->Vertices[i].Binormal.X = bx;
            buf->Vertices[i].Binormal.Y = by;
            buf->Vertices[i].Binormal.Z = bz;

            buf->Vertices[i].Tangent.X = tx;
            buf->Vertices[i].Tangent.Y = ty;
            buf->Vertices[i].Tangent.Z = tz;
            */

            buf->Vertices_Standard[i].Color = video::SColor(255, 255, 255, 255);

            //if (log->isEnabled())
            //    log->addLine(formatString("Vertice : x=%f, y=%f, z=%f, UV : u=%f, v=%f", x, y, z, u, v));
        }
        //log->flush();

        buf->Indices.set_used(nbFaces * 3);
        for (s32 i = 0; i < nbFaces; i++)
        {
            s32 idx1, idx2, idx3;

            f->read(&idx1, 4);
            f->read(&idx2, 4);
            f->read(&idx3, 4);
            // The 3 indices, and a 0.
            f->seek(4, true);


            buf->Indices[3*i] = idx1;
            buf->Indices[3*i + 1] = idx3;
            buf->Indices[3*i + 2] = idx2;

            //log->addLine(formatString("Indice : idx1 = %d, idx2 = %d, idx3 = %d", idx1, idx2, idx3));
        }
        log->flush();
        buf->recalculateBoundingBox();
    }

    // Read bones
    for (int i = 0; i < nbBones; i++)
    {
        // why this code ?
        /*
        if (i == 0)
        {
            scene::ISkinnedMesh::SJoint* root
            root = currentLODMesh->addJoint();

            core::vector3df pos(0.0f, 0.0f, 0.0f);
            core::vector3df scale(1.0f, 1.0f, 1.0f);
            core::matrix4 posM;
            posM.setTranslation(pos);
            core::matrix4 rotM;
            rotM.setRotationDegrees(pos);
            core::matrix4 scaleM;
            scaleM.setRotationDegrees(scale);

            root->Animatedposition = pos;
            root->Animatedrotation = pos;
            root->Animatedscale = scale;

            root->GlobalMatrix = posM * rotM * scaleM;
            root->LocalMatrix = posM * rotM * scaleM;
        }
        */


        u32 sizeJointName = readS32(f);
        core::stringc jointName = readString(f, sizeJointName);

        scene::ISkinnedMesh::SJoint* joint = currentLODMesh->addJoint();
        joint->Name = jointName;

        core::array<f32> jointData = readDataArray<f32>(f, 12);
        core::matrix4 mat(core::matrix4::EM4CONST_IDENTITY);

        log->add(formatString("%s : ", joint->Name.c_str()));
        int m = 0;
        for (int n = 0; n < 12; n++)
        {
            if (m == 3 || m == 7 || m == 11)
                m++;

            log->add(formatString("%f, ", jointData[n]));
            mat[m] = jointData[n];
            m++;
        }
        log->flush();
        //std::cout << "Translation : X="<< mat.getTranslation().X << ", Y="<< mat.getTranslation().Y << ", Z="<< mat.getTranslation().Z << std::endl;
        //std::cout << "Rotation : X="<< mat.getRotationDegrees().X << ", Y="<< mat.getRotationDegrees().Y << ", Z="<< mat.getRotationDegrees().Z << std::endl;
        //std::cout << "Scale : X="<<mat.getScale().X << ", Y="<< mat.getScale().Y << ", Z="<< mat.getScale().Z << std::endl;

        core::matrix4 posMat;
        core::vector3df pos = mat.getTranslation();

        core::matrix4 invRot;
        mat.getInverse(invRot);
        invRot.rotateVect(pos);

        double tmp = pos.Y;
        pos.Y = pos.Z;
        pos.Z = tmp;
        pos += meshCenter;
        posMat.setTranslation(pos);

        core::matrix4 rotMat;
        core::vector3df rot = mat.getRotationDegrees();
        tmp = rot.Y;
        rot.Y = rot.Z;
        rot.Z = tmp;
        rot = core::vector3df(0.0f, 0.0f, 0.0f);
        rotMat.setRotationDegrees(rot);

        core::matrix4 scaleMat;
        core::vector3df scale = mat.getScale();
        tmp = scale.Y;
        scale.Y = scale.Z;
        scale.Z = tmp;
        scaleMat.setScale(scale);

        mat = posMat * rotMat * scaleMat;

        joint->Animatedposition = pos;
        joint->Animatedrotation = rot;
        joint->Animatedscale = scale;
        log->addLineAndFlush("");
        joint->GlobalMatrix = mat;
        joint->LocalMatrix = mat;

        for (u32 w = 0; w < weightTable.size(); ++w)
        {
            if (weightTable[w].BoneID == i)
            {
                ISkinnedMesh::SWeight* wt = currentLODMesh->addWeight(joint);
                wt->buffer_id = weightTable[w].MeshBufferID;
                wt->strength = weightTable[w].Strenght;
                wt->vertex_id = weightTable[w].VertexID;
            }
        }
    }
}

void IO_MeshLoader_RE::readCollisionMeshChunk(io::IReadFile* f)
{
    log->addLineAndFlush("Read COLLISION chunk");
    scene::ISkinnedMesh* currentLODMesh = CollisionMesh;

    u32 sizeCollisionName = readU32(f);
    core::stringc collisionName = readString(f, sizeCollisionName);
    log->addLineAndFlush(formatString("Read collision mesh name : %s", collisionName.c_str()));

    u32 nbVertices = readU32(f);
    u32 nbTriangles = readU32(f);
    u32 nbU = readU32(f);

    SSkinMeshBuffer* buf = currentLODMesh->addMeshBuffer();
    buf->Vertices_Standard.set_used(nbVertices);

    for (u32 i = 0; i < nbVertices; ++i)
    {
        buf->Vertices_Standard[i].Pos.X = readF32(f);
        buf->Vertices_Standard[i].Pos.Z = readF32(f);
        buf->Vertices_Standard[i].Pos.Y = readF32(f);

        buf->Vertices_Standard[i].Color = video::SColor(255, 255, 255, 255);
    }

    buf->Indices.set_used(nbTriangles * 3);
    for (u32 i = 0; i < nbTriangles; ++i)
    {
        buf->Indices[3*i] = readU32(f);
        buf->Indices[3*i + 1] = readU32(f);
        buf->Indices[3*i + 2] = readU32(f);
        f->seek(4, true); // smoothing group ?
    }

    u32 strSize = readU32(f);
    f->seek(strSize, true); // skip str

    f->seek(36, true); // 3 axis

    core::vector3df meshCenter;
    meshCenter.X = readF32(f);
    meshCenter.Z = readF32(f);
    meshCenter.Y = readF32(f);

    // add the offset
    for (u32 i = 0; i < nbVertices; ++i)
        buf->Vertices_Standard[i].Pos += meshCenter;
}

void IO_MeshLoader_RE::readHeaderChunk(io::IReadFile* f)
{
    log->addLineAndFlush("Read HEADER chunk");
    u32 userSize = readU32(f);
    core::stringc user = readString(f, userSize);
    log->addLineAndFlush(formatString("User : %s", user.c_str()));

    u32 pathSize = readU32(f);
    core::stringc path = readString(f, pathSize);
    log->addLineAndFlush(formatString("Path : %s\n", path.c_str()));
}

} // end namespace scene
} // end namespace irr

