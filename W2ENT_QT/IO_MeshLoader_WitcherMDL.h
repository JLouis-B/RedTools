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
#include "Log.h"
#include "IO_SpeedTreeLoader.h"

#include <vector>
#include <map>

// it's the right solution to keep a good hirerachy for animations, but it currently poses problems with the exporters
//#define TW1_ATTACH_MESHES_TO_NODES

// Based on the loader and spec of the Xoreos engine
// https://github.com/xoreos/xoreos/blob/master/src/graphics/aurora/model_witcher.cpp

using namespace irr;


enum ConfigNodeType
{
    ConfigNodeTrimesh = 1,
    ConfigNodeSkin = 2,
    ConfigNodeTexturePaint = 4
};



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

struct StaticControllersData
{
    // transform
    core::vector3df position;
    core::quaternion rotation;
    core::vector3df scale;
    core::matrix4 localTransform;
    core::matrix4 globalTransform;

    // others
    f32 alpha;
    video::SColor selphIllumColor;

    StaticControllersData():
        position(core::vector3df(0.f, 0.f, 0.f)),
        rotation(core::quaternion(0.f, 0.f, 0.f, 1.f)),
        scale(core::vector3df(1.f, 1.f, 1.f)),
        alpha(1.f)
        {}

    void computeLocalTransform()
    {
        core::matrix4 qPos, qRot, qScale;
        qPos.setTranslation(position);
        core::vector3df euler;
        rotation.toEuler(euler);
        qRot.setRotationRadians(euler);
        qScale.setScale(scale);

        localTransform = qPos * qRot * qScale;
    }
};

struct ControllersData
{
    core::array<f32> positionTime;
    core::array<core::vector3df> position;

    core::array<f32> rotationTime;
    core::array<core::quaternion> rotation;

    core::array<f32> scaleTime;
    core::array<core::vector3df> scale;

    // not supported for the animations
    core::array<f32> alphaTime;
    core::array<f32> alpha;

    core::array<f32> selphIllumColorTime;
    core::array<video::SColor> selphIllumColor;
};


class TW1_MaterialParser
{
public :
    explicit TW1_MaterialParser(io::IFileSystem *fs);
    bool loadFile(core::stringc filename);
    bool loadFromString(core::stringc content);
    bool hasMaterial();
    void debugPrint();

    core::stringc getShader();
    core::stringc getTexture(u32 slot);
    video::E_MATERIAL_TYPE getMaterialTypeFromShader();

private:
    Log* _log;
    io::IFileSystem* FileSystem;
    core::stringc _shader;
    std::map<core::stringc, core::stringc> _textures;
    std::map<core::stringc, core::stringc> _bumpmaps;
    std::map<core::stringc, core::stringc> _strings;
    // TODO: vector4 ?
    std::map<core::stringc, core::vector3df> _vectors;
    std::map<core::stringc, f32> _floats;
};

struct SkinMeshToLoadEntry
{
    long Seek;
    StaticControllersData ControllersData;
    scene::ISkinnedMesh::SJoint* Joint;
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

    scene::ISkinnedMesh* meshToAnimate;

private:
    std::map<u32, core::stringc> NodeTypeNames;

    bool load(io::IReadFile* file);
    void loadNode(io::IReadFile* file, scene::ISkinnedMesh::SJoint *parentJoint, core::matrix4 parentMatrix);
    void readMeshNode(io::IReadFile* file, StaticControllersData controllers);
    void readTexturePaintNode(io::IReadFile* file, StaticControllersData controllers);
    void readSkinNode(io::IReadFile* file, StaticControllersData controllers);
    void readSpeedtreeNode(io::IReadFile* file, StaticControllersData controllers);
    TW1_MaterialParser readTextures(io::IReadFile *file);

    // Animations
    void readAnimations(io::IReadFile* file);
    void loadAnimationNode(io::IReadFile* file, f32 timeOffset);
    ControllersData readNodeControllers(io::IReadFile* file, ArrayDef key, ArrayDef data);

    template <class T> core::array<T> readArray(io::IReadFile* file, ArrayDef def);

    StaticControllersData getStaticNodeControllers(io::IReadFile* file, ArrayDef key, ArrayDef data);
    void transformVertices(core::matrix4 mat);

    bool hasTexture(core::stringc texPath);
    video::ITexture* getTexture(core::stringc texPath);


    scene::ISceneManager* SceneManager;
    io::IFileSystem* FileSystem;
    scene::ISkinnedMesh* AnimatedMesh;

    ModelData ModelInfos;
    
    core::stringc GameTexturesPath;

    core::array<SkinMeshToLoadEntry> SkinMeshToLoad;

    Log* _log;
    u32 _depth;
};

#endif // CWITCHERMDLMESHFILELOADER_H
