// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_W3ENT_MESH_FILE_LOADER_H_INCLUDED__
#define __C_W3ENT_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "irrString.h"
#include "SMesh.h"
#include "SAnimatedMesh.h"
#include "IMeshManipulator.h"
#include "ISkinnedMesh.h"

#include "CSkeleton.h"

#include "meshcombiner.h"

#include "log.h"


using namespace irr;


enum EMeshVertexType
{
    EMVT_STATIC,
    EMVT_SKINNED
};


// Information to load a mesh from the buffer
struct SMeshInfos
{
    u32 numVertices;
    u32 numIndices;
    u32 numBonesPerVertex;

    u32 firstVertex;
    u32 firstIndice;

    EMeshVertexType vertexType;

    u32 materialID;
};

// Informations about the .buffer file
struct SVertexBufferInfos
{
    u32 verticesCoordsOffset;
    u32 uvOffset;
    u32 normalsOffset;

    unsigned short nbVertices;
};

struct SBufferInfos
{
    u32 verticesBufferOffset;
    u32 verticesBufferSize;

    u32 indicesBufferOffset;
    u32 indicesBufferSize;

    core::vector3df quantizationScale;
    core::vector3df quantizationOffset;

    core::array<SVertexBufferInfos> verticesBuffer;
};


struct W3_DataInfos
{
    int adress;
    int size;
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

//! Meshloader capable of loading w2ent meshes.
class CW3ENTMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
    CW3ENTMeshFileLoader(scene::ISceneManager* smgr, io::IFileSystem* fs);

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IReferenceCounted::drop() for more information.
	virtual IAnimatedMesh* createMesh(io::IReadFile* file);


    core::array<video::SMaterial> Materials;
    CSkeleton Skeleton;

private:

    // Attributes
    scene::ISceneManager* SceneManager;
    io::IFileSystem* FileSystem;
    scene::ISkinnedMesh* AnimatedMesh;


    // load the different types of data
    bool W3_load(io::IReadFile* file);
    void W3_CMesh(io::IReadFile* file, W3_DataInfos infos);
    video::SMaterial W3_CMaterialInstance(io::IReadFile* file, W3_DataInfos infos);
    void W3_CMeshComponent(io::IReadFile* file, W3_DataInfos infos);
    void W3_CEntityTemplate(io::IReadFile* file, W3_DataInfos infos);   // Not handled yet
    void W3_CEntity(io::IReadFile* file, W3_DataInfos infos);           // Not handled yet
    CSkeleton W3_CSkeleton(io::IReadFile* file, W3_DataInfos infos);

    // load a mesh buffer from the buffer file
    void W3_ReadBuffer(io::IReadFile* file, SBufferInfos bufferInfos, SMeshInfos meshInfos);


    // Main function
	bool load(io::IReadFile* file);

    video::ITexture* getTexture(io::path filename);

    // Read functions
    core::stringc readStringUntilNull(io::IReadFile* file);

    core::stringc readWord(io::IReadFile* f, int nbLetter);
    core::array<int> readInts (io::IReadFile* f, int nbInt);
    core::array<unsigned short> readUnsignedShorts (io::IReadFile* f, int nbShorts);
    core::array<unsigned char> readUnsignedChars (io::IReadFile* f, int nbChar);
    core::array<float> readFloats (io::IReadFile* f, int nbInt);


    // Strings table
    core::array<core::stringc> Strings;
    // Files table
    core::array<core::stringc> Files;

    u32 nbBonesPos;

    int getTextureLayerFromTextureType(core::stringc textureType);
    core::stringc searchParent(core::stringc bonename);


    SMeshInfos createSMeshInfos();
    SBufferInfos createSBufferInfo();

    // To read the properties
    SBufferInfos ReadSMeshCookedDataProperty(io::IReadFile* file);
    core::array<SMeshInfos> ReadSMeshChunkPackedProperty(io::IReadFile* file);

    u32 ReadUInt32Property(io::IReadFile* file);
    char ReadUInt8Property(io::IReadFile* file);
    float ReadFloatProperty(io::IReadFile* file);
    bool ReadBoolProperty(io::IReadFile* file);
    core::vector3df ReadVector3Property(io::IReadFile* file);
    void ReadUnknowProperty(io::IReadFile* file);
    EMeshVertexType ReadEMVTProperty(io::IReadFile* file);
    void ReadRenderChunksProperty(io::IReadFile* file, SBufferInfos* buffer);
    void ReadMaterialsProperty(io::IReadFile* file);
    video::SMaterial ReadIMaterialProperty(io::IReadFile* file);
    core::array<core::vector3df> ReadBonesPosition(io::IReadFile* file);

    // read external files
    video::SMaterial ReadW2MIFile(core::stringc filename);
    ISkinnedMesh* ReadW2MESHFile(core::stringc filename);

    void computeLocal(ISkinnedMesh::SJoint* joint);

    // debug log
    Log* log;

    io::path GameTexturesPath;
    io::path GamePath;

    core::array<scene::ISkinnedMesh*> Meshes;

};

} // end namespace scene
} // end namespace irr

#endif
