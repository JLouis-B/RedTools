#ifndef IO_MESHLOADER_CEF_H
#define IO_MESHLOADER_CEF_H

#include "IMeshLoader.h"
#include "irrString.h"
#include "SMesh.h"
#include "SAnimatedMesh.h"
#include "IMeshManipulator.h"
#include "ISkinnedMesh.h"
#include "IFileSystem.h"
#include "IReadFile.h"
#include "Log.h"


enum VertexComponent
{
    VERTEX_POSITION,
    VERTEX_BLENDWEIGHT,
    VERTEX_BLENDINDICE,

    VERTEX_NORMAL,
    VERTEX_TANGENT,
    VERTEX_TEXCOORD,

    VERTEX_INDICE,

    VERTEX_ERROR
};

class IO_MeshLoader_CEF : public scene::IMeshLoader
{
public:
    IO_MeshLoader_CEF(scene::ISceneManager *smgr, io::IFileSystem *fs);

    //! returns true if the file maybe is able to be loaded by this class
    //! based on the file extension (e.g. ".cob")
    virtual bool isALoadableFileExtension(const io::path& filename) const;

    //! creates/loads an animated mesh from the file.
    //! \return Pointer to the created mesh. Returns 0 if loading failed.
    //! If you no longer need the mesh, you should call IAnimatedMesh::drop().
    //! See IReferenceCounted::drop() for more information.
    virtual scene::IAnimatedMesh* createMesh(io::IReadFile* file);

    core::array<core::stringc> bufferNames;

private:
    bool load(io::IReadFile* file);

    scene::ISceneManager* SceneManager;
    io::IFileSystem* FileSystem;
    scene::ISkinnedMesh* AnimatedMesh;

    Log* _log;

};

#endif // IO_MESHLOADER_CEF_H
