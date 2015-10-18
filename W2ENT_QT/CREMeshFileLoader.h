// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_RE_MESH_FILE_LOADER_H_INCLUDED__
#define __C_RE_MESH_FILE_LOADER_H_INCLUDED__

// #define COMPILE_WITH_LODS_SUPPORT

#include "IMeshLoader.h"
#include "irrString.h"
#include "SMesh.h"
#include "SAnimatedMesh.h"
#include "IMeshManipulator.h"
#include "ISkinnedMesh.h"

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


	struct WeightT
	{
	    u32 vertexID;
	    u32 meshBufferID;
	    f32 strenght;
	    u32 boneID;
	};

//! Meshloader capable of loading re meshes.
class CREMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	CREMeshFileLoader(scene::ISceneManager* smgr, io::IFileSystem* fs);

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IReferenceCounted::drop() for more information.
	virtual IAnimatedMesh* createMesh(io::IReadFile* file);

	IAnimatedMesh* _lod1;
	IAnimatedMesh* _lod2;


private:

    // Main function
	bool load(io::IReadFile* file);

    void readLOD(io::IReadFile* f);


	void writeLog();


    // Read functions
    core::stringc readWord(io::IReadFile* f, s32 nbLetter);
    core::array<s32> readInts (io::IReadFile* f, s32 nbInt);
    core::array<u16> readUnsignedShorts (io::IReadFile* f, s32 nbShorts);
    core::array<u8> readUnsignedChars (io::IReadFile* f, s32 nbChar);
    core::array<f32> readFloats (io::IReadFile* f, s32 nbInt);

    // Atrributes
    scene::ISceneManager* SceneManager;
    io::IFileSystem* FileSystem;
    scene::ISkinnedMesh* AnimatedMesh;
    //scene::SMesh* Mesh;
    //CSkinnedMesh* AnimatedMesh;


    //DEBUG
    core::stringc logContent;


};

} // end namespace scene
} // end namespace irr

#endif
