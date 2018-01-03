#ifndef CWITCHERMDLMESHFILELOADER_H
#define CWITCHERMDLMESHFILELOADER_H

#include "IMeshLoader.h"
#include "irrString.h"
#include "SMesh.h"
#include "SAnimatedMesh.h"
#include "IMeshManipulator.h"
#include "ISkinnedMesh.h"
#include "IFileSystem.h"
#include "IReadFile.h"

using namespace irr;

struct ArrayDef
{
    u32 firstElemOffest;
    u32 nbUsedEntries;
    u32 nbAllocatedEntries;
};

struct ModelData
{
    u16 fileVersion;

    u32 offsetModelData;
    u32 sizeModelData;

    u32 offsetRawData;
    u32 sizeRawData;

    u32 offsetTextureInfo;

    u32 offsetTexData;
    u32 sizeTexData;
};


class TW1_MaterialParser
{
public :
    TW1_MaterialParser(io::IFileSystem *fs);
    bool loadFile(core::stringc filename);
    bool loadFromString(core::stringc content);

    core::stringc getShader();
    core::stringc getTexture(u32 slot);

private:
    io::IFileSystem* FileSystem;
    core::stringc _shader;
    core::stringc _textures[_IRR_MATERIAL_MAX_TEXTURES_];
};

class IO_MeshLoader_WitcherMDL : public scene::IMeshLoader
{
public:
    IO_MeshLoader_WitcherMDL(scene::ISceneManager *smgr, io::IFileSystem *fs);

    //! returns true if the file maybe is able to be loaded by this class
    //! based on the file extension (e.g. ".cob")
    virtual bool isALoadableFileExtension(const io::path& filename) const;

    //! creates/loads an animated mesh from the file.
    //! \return Pointer to the created mesh. Returns 0 if loading failed.
    //! If you no longer need the mesh, you should call IAnimatedMesh::drop().
    //! See IReferenceCounted::drop() for more information.
    virtual scene::IAnimatedMesh* createMesh(io::IReadFile* file);

private:
    bool load(io::IReadFile* file);
    void loadNode(io::IReadFile* file);
    void readMesh(io::IReadFile* file, core::matrix4 transform);
    void readTexturePaint(io::IReadFile* file, core::matrix4 transform);
    void readTextures(io::IReadFile *file, core::array<core::stringc> &textures);

    template <class T> core::array<T> readArray(io::IReadFile* file, ArrayDef def);

    core::matrix4 readNodeControllers(io::IReadFile* file, ArrayDef key, ArrayDef data);
    void transformVertices(core::matrix4 mat);

    bool hasTexture(core::stringc texPath);
    video::ITexture* getTexture(core::stringc texPath);


    scene::ISceneManager* SceneManager;
    io::IFileSystem* FileSystem;
    scene::ISkinnedMesh* AnimatedMesh;

    ModelData ModelInfos;
    
    core::stringc GameTexturesPath;
};

#endif // CWITCHERMDLMESHFILELOADER_H
