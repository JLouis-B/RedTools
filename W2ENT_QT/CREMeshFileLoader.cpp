// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#define _IRR_COMPILE_WITH_RE_LOADER_
#ifdef _IRR_COMPILE_WITH_RE_LOADER_

#include "CREMeshFileLoader.h"

#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "IReadFile.h"
#include "IWriteFile.h"

#include <sstream>

#include "LoadersUtils.h"

//#define _DEBUG
#ifdef _DEBUG
#define _REREADER_DEBUG
#endif

namespace irr
{
namespace scene
{

//! Constructor
CREMeshFileLoader::CREMeshFileLoader(scene::ISceneManager* smgr, io::IFileSystem* fs)
: SceneManager(smgr), FileSystem(fs)
{
	#ifdef _DEBUG
	setDebugName("CREMeshFileLoader");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CREMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "re" );
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CREMeshFileLoader::createMesh(io::IReadFile* f)
{
	if (!f)
		return 0;

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
	}
	else
	{
		AnimatedMesh->drop();
		AnimatedMesh = 0;
	}

    log->addLineAndFlush("Loaded");

	return AnimatedMesh;
}





bool CREMeshFileLoader::load(io::IReadFile* f)
{
    _lod1 = 0;
    _lod2 = 0;

    u32 nbLOD = readU32(f);
    nbLOD--;

    f->seek(28);
    s32 adress = readS32(f);
    f->seek(adress);

    #ifdef COMPILE_WITH_LODS_SUPPORT
        // Read all the LODS
        for (u32 i = 0; i < nbLOD; ++i)
            readLOD(f);
    #else
        // Read Only the LOD0
        readLOD(f);
    #endif // COMPILE_WITH_LODS_SUPPORT


    return true;
}

void CREMeshFileLoader::readLOD(io::IReadFile* f)
{

    core::array<WeightT> weightTable;


    s32 adress = f->getPos();

    u32 sizeLODname;
    core::stringc LODname;

    f->read(&sizeLODname, 4);
    LODname = readString(f, sizeLODname);

    //f->seek(8, true);
    s32 nbMeshBuffer = readS32(f);
    s32 nbBones = readS32(f);


    f->seek(adress + 52);
    irr::core::vector3df meshCenter;
    f->read(&meshCenter.X, 4);
    f->read(&meshCenter.Z, 4);
    f->read(&meshCenter.Y, 4);


    //f->seek(adress + 64);

    for (int n = 0; n < nbMeshBuffer; n++)
    {
        //SMeshBufferTangents* buf = new SMeshBufferTangents();
        SSkinMeshBuffer* buf = AnimatedMesh->addMeshBuffer();
        s32 sizeMatName = readS32(f);
        core::stringc matName = readString(f, sizeMatName);

        /*
        std::cout << "matname" << std::endl;
        std::cout << sizeMatName << std::endl;
        std::cout << matName.c_str() << std::endl;
        */

        s32 sizeMatDiff = readS32(f);
        core::stringc matDiff = readString(f, sizeMatDiff);

        video::ITexture *tex = SceneManager->getVideoDriver()->getTexture(matDiff);
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

        s32 sizeMatBle = readS32(f);
        core::stringc matBle = readString(f, sizeMatBle);


        s32 nbVertices = readS32(f);
        s32 nbFaces = readS32(f);

        /*
        std::cout << sizeMatBle << std::endl;
        std::cout << matBle.c_str() << std::endl;
        */

        if (log->isEnabled())
        {
            log->addLine(formatString("nbVertices : %d", nbVertices));
            log->addLine(formatString("nbFaces : %d", nbFaces));
            log->addLine(formatString("pos : %d", f->getPos()));
            log->flush();
        }

        buf->Vertices_Standard.reallocate(nbVertices);
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
                    tmp.meshBufferID = n;
                    tmp.vertexID = i;
                    tmp.strenght = strenghts[ns];
                    tmp.boneID = readF32(f);
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

            buf->Vertices_Standard[i].Pos += meshCenter;

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

            buf->Vertices_Standard[i].Color = irr::video::SColor(255,255,255,255);

            if (log->isEnabled())
                log->addLine(formatString("Vertice : x=%f, y=%f, z=%f, UV : u=%f, v=%f", x, y, z, u, v));

            //std::cout << "Vertice :  x = " << x << "  y = "<< y << "  z = "<< z << ", UV = " << u << " et " << v << std::endl;
        }
        log->flush();

        buf->Indices.reallocate(nbFaces * 3);
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

            log->addLine(formatString("Indice : idx1 = %d, idx2 = %d, idx3 = %d", idx1, idx2, idx3));
        }
        log->flush();
        buf->recalculateBoundingBox();
    }

    // Read bones
    for (int i = 0; i < nbBones; i++)
    {
        ISkinnedMesh::SJoint* root;
        if (i == 0)
        {
            /*
            root = AnimatedMesh->addJoint();

            irr::core::vector3df pos(0.0f, 0.0f, 0.0f);
            irr::core::vector3df scale(1.0f, 1.0f, 1.0f);
            irr::core::matrix4 posM;
            posM.setTranslation(pos);
            irr::core::matrix4 rotM;
            rotM.setRotationDegrees(pos);
            irr::core::matrix4 scaleM;
            scaleM.setRotationDegrees(scale);

            root->Animatedposition = pos;
            root->Animatedrotation = pos;
            root->Animatedscale = scale;

            root->GlobalMatrix = posM * rotM * scaleM;
            root->LocalMatrix = posM * rotM * scaleM;
            */
        }


        u32 sizeJointName = readS32(f);
        core::stringc jointName = readString(f, sizeJointName);

        ISkinnedMesh::SJoint* joint = AnimatedMesh->addJoint();
        joint->Name = jointName;

        core::array<f32> jointData = readDataArray<f32>(f, 12);
        core::matrix4 mat (core::matrix4::EM4CONST_IDENTITY);

        std::cout << joint->Name.c_str() << " : ";
        int m = 0;
        for (int n = 0; n < 12; n++)
        {
            if (m == 3 || m == 7 || m == 11)
                m++;

            std::cout << jointData[n] << ", ";
            mat[m] = jointData[n];
            m++;
        }
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
        std::cout << std::endl;
        joint->GlobalMatrix = mat;
        joint->LocalMatrix = mat;

        for (u32 w = 0; w < weightTable.size(); ++w)
        {
            if (weightTable[w].boneID == i)
            {
                ISkinnedMesh::SWeight* wt = AnimatedMesh->addWeight(joint);
                wt->buffer_id = weightTable[w].meshBufferID;
                wt->strength = weightTable[w].strenght;
                wt->vertex_id = weightTable[w].vertexID;
            }
        }
    }

}

} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_RE_LOADER_

