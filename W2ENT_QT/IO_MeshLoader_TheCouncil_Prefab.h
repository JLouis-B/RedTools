#ifndef IO_MESHLOADER_THECOUNCIL_PREFAB_H
#define IO_MESHLOADER_THECOUNCIL_PREFAB_H

#include <IMeshLoader.h>
#include <IMeshManipulator.h>
#include <ISkinnedMesh.h>
#include <IFileSystem.h>

#include "Log/LoggerManager.h"

#include <QString>

using namespace irr;

class IO_MeshLoader_TheCouncil_Prefab : public scene::IMeshLoader
{
public:
    IO_MeshLoader_TheCouncil_Prefab(scene::ISceneManager *smgr, io::IFileSystem *fs);

    //! returns true if the file maybe is able to be loaded by this class
    //! based on the file extension (e.g. ".cob")
    virtual bool isALoadableFileExtension(const io::path& filename) const;

    //! creates/loads an animated mesh from the file.
    //! \return Pointer to the created mesh. Returns 0 if loading failed.
    //! If you no longer need the mesh, you should call IAnimatedMesh::drop().
    //! See IReferenceCounted::drop() for more information.
    virtual scene::IAnimatedMesh* createMesh(io::IReadFile* file);

private:
    video::SMaterial readMaterialFile(QString path);

    scene::ISceneManager* SceneManager;
    io::IFileSystem* FileSystem;
    scene::ISkinnedMesh* AnimatedMesh;

    io::path ConfigGamePath;

    LoggerManager* _log;
};

#endif // IO_MESHLOADER_THECOUNCIL_PREFAB_H
