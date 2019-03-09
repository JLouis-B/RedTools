// Copyright (C) 2008-2012 Christian Stehno
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_RE_MESH_WRITER_H_INCLUDED__
#define __IRR_RE_MESH_WRITER_H_INCLUDED__

#include "IMeshWriter.h"
#include "S3DVertex.h"
#include "irrString.h"
#include "ISkinnedMesh.h"

#include <vector>

namespace irr
{
namespace io
{
	class IFileSystem;
} // end namespace io
namespace scene
{
	class IMeshBuffer;
	class ISceneManager;


    struct Weight
    {
        ISkinnedMesh::SWeight w;
        u32 boneID;
    };


	//! class to write meshes, implementing a RE writer
    class IO_MeshWriter_RE : public IMeshWriter
	{
	public:

        IO_MeshWriter_RE(scene::ISceneManager* smgr, io::IFileSystem* fs);
        virtual ~IO_MeshWriter_RE();

		//! Returns the type of the mesh writer
        virtual EMESH_WRITER_TYPE getType() const;

		//! writes a mesh
        virtual bool writeMesh(io::IWriteFile* file, scene::IMesh* mesh, s32 flags=EMWF_NONE);

        bool writeAnimatedMesh(io::IWriteFile* file, scene::IMesh *mesh, bool skinned = false, s32 flags=EMWF_NONE);

        void setLOD1(IMesh* lod1);
        void setLOD2(IMesh* lod2);
        void setCollisionMesh(IMesh* mesh);

        void clearLODS();


	protected:
        void writeHeaderChunk(io::IWriteFile* file);
        void writeMeshChunk(io::IWriteFile* file, core::stringc lodName, IMesh* lodMesh, bool skinned);
        void writeCollisionMeshChunk(io::IWriteFile* file);


        ISceneManager* SceneManager;
        io::IFileSystem* FileSystem;

        scene::IMesh* MeshLOD1;
        scene::IMesh* MeshLOD2;
        scene::IMesh* CollisionMesh;

        void createWeightsTable(scene::ISkinnedMesh* mesh);

        std::vector<std::vector<std::vector<Weight> > > WeightsTable;

        core::array<u32> ChunksAdress;
        core::array<u32> ChunksSize;
        core::array<u32> ChunksAdressToWrite;
	};

} // end namespace
} // end namespace

#endif

