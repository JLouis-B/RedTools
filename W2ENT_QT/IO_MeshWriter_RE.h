#ifndef IO_MESHWRITER_RE_H
#define IO_MESHWRITER_RE_H

#include <IMeshWriter.h>
#include <ISkinnedMesh.h>

#include <vector>

namespace irr
{
namespace io
{
    class IFileSystem;
} // end namespace io
namespace scene
{
    struct Weight
    {
        scene::ISkinnedMesh::SWeight w;
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

        bool writeAnimatedMesh(io::IWriteFile* file, scene::IMesh* mesh, bool skinned = false, s32 flags=EMWF_NONE);

        void setLOD1(scene::IMesh* lod1);
        void setLOD2(scene::IMesh* lod2);
        void setCollisionMesh(scene::IMesh* mesh);

        void clearLODS();

	protected:
        void writeHeaderChunk(io::IWriteFile* file);
        void writeMeshChunk(io::IWriteFile* file, core::stringc lodName, IMesh* lodMesh, bool skinned);
        void writeCollisionMeshChunk(io::IWriteFile* file);


        scene::ISceneManager* SceneManager;
        io::IFileSystem* FileSystem;

        scene::IMesh* MeshLOD1;
        scene::IMesh* MeshLOD2;
        scene::IMesh* CollisionMesh;

        void createWeightsTable(scene::ISkinnedMesh* mesh);

        // TODO: use core::array instead (but boring to do because core::array doesn't call the default contructor of the objects)
        std::vector<std::vector<std::vector<Weight> > > WeightsTable;

        core::array<u32> ChunksAdress;
        core::array<u32> ChunksSize;
        core::array<u32> ChunksAdressToWrite;
	};

} // end namespace
} // end namespace

#endif

