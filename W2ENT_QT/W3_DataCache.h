#ifndef W3_DATACACHE_H
#define W3_DATACACHE_H


#include "ISkinnedMesh.h"
#include "IrrAssimp/IrrAssimpImport.h"
#include <vector>

using namespace irr;

struct BoneEntry
{
    BoneEntry(core::stringc name, core::matrix4 offsetMatrix)
        : _name(name), _offsetMatrix(offsetMatrix)
    {
    }

    core::stringc _name;
    core::matrix4 _offsetMatrix;
};

struct VertexSkinningEntry
{
    VertexSkinningEntry(u32 boneID, u16 meshBufferID, u32 vertexID, f32 strenght)
        : _boneID(boneID), _meshBufferID(meshBufferID), _vertexID(vertexID), _strenght(strenght)
    {
    }

    u32 _boneID;
    u16 _meshBufferID;
    u32 _vertexID;
    f32 _strenght;
};

class SkinnedVertex
{
public:
    SkinnedVertex()
    {
        Moved = false;
        Position = irr::core::vector3df(0, 0, 0);
        Normal = irr::core::vector3df(0, 0, 0);
    }

    bool Moved;
    irr::core::vector3df Position;
    irr::core::vector3df Normal;
};

class W3_DataCache
{
    scene::ISkinnedMesh* _owner;

    core::array<BoneEntry> _bones;
    core::array<VertexSkinningEntry> _vertices;


    std::vector<std::vector<SkinnedVertex> > _skinnedVertex;
    void skinJoint(irr::scene::ISkinnedMesh::SJoint *joint, BoneEntry bone);
    void buildSkinnedVertexArray();
    void applySkinnedVertexArray();

public:
    W3_DataCache();

    static W3_DataCache _instance;

    void setOwner(scene::ISkinnedMesh* owner);
    void addBoneEntry(core::stringc name, core::matrix4 boneOffset);
    void addVertexEntry(u32 boneID, u16 meshBufferID, u32 vertexID, f32 strenght);
    void clear();
    void apply();
    void skin();

    u32 _bufferID;
};

#endif // W3_DATACACHE_H
