#ifndef IO_MESHLOADER_W3ENT_H
#define IO_MESHLOADER_W3ENT_H

#include <map>

#include <IMeshLoader.h>
#include <ISkinnedMesh.h>

#include "TW3_CSkeleton.h"
#include "TW3_DataCache.h"
#include "Utils_RedEngine.h"
#include "MeshCombiner.h"
#include "Log/LoggerManager.h"


using namespace irr;


enum EMeshVertexType
{
    EMVT_STATIC,
    EMVT_SKINNED
};

enum EAnimTrackType
{
    EATT_POSITION,
    EATT_ORIENTATION,
    EATT_SCALE
};

enum SAnimationBufferOrientationCompressionMethod
{
    ABOCM_PackIn64bitsW,
    ABOCM_PackIn48bitsW,
    ABOCM_PackIn40bitsW,
    ABOCM_AsFloat_XYZW,
    ABOCM_AsFloat_XYZSignedW,
    ABOCM_AsFloat_XYZSignedWInLastBit,
    ABOCM_PackIn48bits,
    ABOCM_PackIn40bits,
    ABOCM_PackIn32bits
};


// Information to load a mesh from the buffer
struct SMeshInfos
{
    SMeshInfos() : numVertices(0), numIndices(0), numBonesPerVertex(4), firstVertex(0), firstIndice(0), vertexType(EMVT_STATIC), materialID(0)
    {
    }

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
    SVertexBufferInfos() : verticesCoordsOffset(0), uvOffset(0), normalsOffset(0), indicesOffset(0), nbVertices(0), nbIndices(0), lod(1)
    {
    }

    u32 verticesCoordsOffset;
    u32 uvOffset;
    u32 normalsOffset;

    u32 indicesOffset;

    u16 nbVertices;
    u32 nbIndices;

    u8 lod;
};

struct SBufferInfos
{
    SBufferInfos() : verticesBufferOffset(0), verticesBufferSize(0), indicesBufferOffset(0), indicesBufferSize(0),
        quantizationScale(core::vector3df(1, 1, 1)), quantizationOffset(core::vector3df(0, 0, 0)), verticesBuffer(core::array<SVertexBufferInfos>())
    {
    }

    u32 verticesBufferOffset;
    u32 verticesBufferSize;

    u32 indicesBufferOffset;
    u32 indicesBufferSize;

    core::vector3df quantizationScale;
    core::vector3df quantizationOffset;

    core::array<SVertexBufferInfos> verticesBuffer;
};

struct SPropertyHeader
{
    core::stringc propName;
    core::stringc propType;
    s32 propSize;
    u32 endPos;
};


struct W3_DataInfos
{
    s32 adress;
    s32 size;
};

struct SAnimationBufferBitwiseCompressedData
{
    SAnimationBufferBitwiseCompressedData() : type(EATT_POSITION), dt(0.f), compression(0), numFrames(0), dataAddr(0), dataAddrFallback(0)
    {

    }

    EAnimTrackType type;
    f32 dt;
    s8 compression;
    u16 numFrames;
    u32 dataAddr;
    u32 dataAddrFallback;
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
class IO_MeshLoader_W3ENT : public IMeshLoader
{
public:

	//! Constructor
    IO_MeshLoader_W3ENT(scene::ISceneManager* smgr, io::IFileSystem* fs);

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IReferenceCounted::drop() for more information.
	virtual IAnimatedMesh* createMesh(io::IReadFile* file);

    std::map<int, video::SMaterial> Materials;
    TW3_CSkeleton Skeleton;
    scene::ISkinnedMesh* meshToAnimate;

private:

    // Attributes
    scene::ISceneManager* SceneManager;
    io::IFileSystem* FileSystem;
    scene::ISkinnedMesh* AnimatedMesh;

    u32 FrameOffset;

    // load the different types of data
    bool W3_load(io::IReadFile* file);
    void W3_CMesh(io::IReadFile* file, W3_DataInfos infos);
    video::SMaterial W3_CMaterialInstance(io::IReadFile* file, W3_DataInfos infos);
    void W3_CMeshComponent(io::IReadFile* file, W3_DataInfos infos);
    void W3_CEntityTemplate(io::IReadFile* file, W3_DataInfos infos);   // Not handled yet
    void W3_CEntity(io::IReadFile* file, W3_DataInfos infos);           // Not handled yet
    TW3_CSkeleton W3_CSkeleton(io::IReadFile* file, W3_DataInfos infos);
    void W3_CAnimationBufferBitwiseCompressed(io::IReadFile* file, W3_DataInfos infos);
    void W3_CUnknown(io::IReadFile* file, W3_DataInfos infos);

    // load a mesh buffer from the buffer file
    bool W3_ReadBuffer(io::IReadFile* file, SBufferInfos bufferInfos, SMeshInfos meshInfos);

    SAnimationBufferBitwiseCompressedData ReadSAnimationBufferBitwiseCompressedDataProperty(io::IReadFile* file);
    core::array<core::array<SAnimationBufferBitwiseCompressedData> > ReadSAnimationBufferBitwiseCompressedBoneTrackProperty(io::IReadFile* file);
    void readAnimBuffer(core::array<core::array<SAnimationBufferBitwiseCompressedData> >& inf, io::IReadFile *dataFile, SAnimationBufferOrientationCompressionMethod c);

    void ReadBones(io::IReadFile* file);


    // Main function
	bool load(io::IReadFile* file);

    video::ITexture* getTexture(io::path filename);


    // Strings table
    core::array<core::stringc> Strings;
    // Files table
    core::array<core::stringc> Files;

    u32 NbBonesPos;

    int getTextureLayerFromTextureType(core::stringc textureType);

    // To read the properties
    bool ReadPropertyHeader(io::IReadFile* file, SPropertyHeader& propHeader);

    SBufferInfos ReadSMeshCookedDataProperty(io::IReadFile* file);
    core::array<SMeshInfos> ReadSMeshChunkPackedProperty(io::IReadFile* file);

    u32 ReadUInt32Property(io::IReadFile* file);
    u8 ReadUInt8Property(io::IReadFile* file);
    f32 ReadFloatProperty(io::IReadFile* file);
    bool ReadBoolProperty(io::IReadFile* file);
    core::vector3df ReadVector3Property(io::IReadFile* file);
    void ReadUnknowProperty(io::IReadFile* file);
    EMeshVertexType ReadEMVTProperty(io::IReadFile* file);
    SAnimationBufferOrientationCompressionMethod ReadAnimationBufferOrientationCompressionMethodProperty(io::IReadFile* file);
    void ReadRenderChunksProperty(io::IReadFile* file, SBufferInfos* buffer);
    core::array<video::SMaterial> ReadMaterialsProperty(io::IReadFile* file);
    video::SMaterial ReadIMaterialProperty(io::IReadFile* file);
    core::array<core::vector3df> ReadBonesPosition(io::IReadFile* file);
    void ReadRenderLODSProperty(io::IReadFile* file);

    // read external files
    video::SMaterial ReadMaterialFile(core::stringc filename);
    video::SMaterial ReadW2MIFile(core::stringc filename);
    ISkinnedMesh* ReadW2MESHFile(core::stringc filename);

    bool checkBones(io::IReadFile* file, char nbBones);

    // debug log
    LoggerManager* log;
    void writeLogBoolProperty(core::stringc name, bool value);
    void writeLogHeader(const io::IReadFile *f);

    bool ConfigLoadSkeleton;
    bool ConfigLoadOnlyBestLOD;
    io::path ConfigGameTexturesPath;
    io::path ConfigGamePath;

    core::array<scene::ISkinnedMesh*> Meshes;

};

} // end namespace scene
} // end namespace irr

#endif
