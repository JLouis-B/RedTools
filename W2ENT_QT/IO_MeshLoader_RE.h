// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef IO_MESHLOADER_RE_H
#define IO_MESHLOADER_RE_H

//#define COMPILE_WITH_LODS_SUPPORT

#include "IMeshLoader.h"
#include "ISkinnedMesh.h"
#include "Log.h"

enum ChunkType
{
    Chunk_Header,
    Chunk_Mesh,
    Chunk_Collision
};

struct ChunkInfos
{
    ChunkType Type;
    u32 Id;
    u32 Adress;
    u32 Size;
};

struct WeightT
{
    u32 VertexID;
    u32 MeshBufferID;
    f32 Strenght;
    u32 BoneID;
};

namespace irr
{


namespace io
{
	class IFileSystem;
	class IReadFile;
} // end namespace io
namespace scene
{
class IMeshManipulator;

//! Meshloader capable of loading re meshes.
class IO_MeshLoader_RE : public IMeshLoader
{
public:

	//! Constructor
    IO_MeshLoader_RE(scene::ISceneManager* smgr, io::IFileSystem* fs);

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IReferenceCounted::drop() for more information.
	virtual IAnimatedMesh* createMesh(io::IReadFile* file);

    scene::ISkinnedMesh* Lod1Mesh;
    scene::ISkinnedMesh* Lod2Mesh;
    scene::IMesh* CollisionMesh;


private:

	bool load(io::IReadFile* file);

    void readCollisionMeshChunk(io::IReadFile* f);
    void readMeshChunk(io::IReadFile* f, u32 id);
    void readHeaderChunk(io::IReadFile* f);

    scene::ISceneManager* SceneManager;
    io::IFileSystem* FileSystem;
    scene::ISkinnedMesh* AnimatedMesh;

    Log* log;
};

} // end namespace scene
} // end namespace irr

#endif
