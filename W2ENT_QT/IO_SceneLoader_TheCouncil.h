#ifndef IO_SCENELOADER_THECOUNCILL_H
#define IO_SCENELOADER_THECOUNCILL_H


#include "ISceneLoader.h"
#include "irrString.h"
#include "SMesh.h"
#include "SAnimatedMesh.h"
#include "IMeshManipulator.h"
#include "ISkinnedMesh.h"
#include "IFileSystem.h"
#include "IReadFile.h"
#include "Log/LoggerManager.h"

#include <QString>

using namespace irr;

class IO_SceneLoader_TheCouncil : public scene::ISceneLoader
{
public:
    IO_SceneLoader_TheCouncil(scene::ISceneManager *smgr, io::IFileSystem *fs);

    //! returns true if the file maybe is able to be loaded by this class
    //! based on the file extension (e.g. ".cob")
    virtual bool isALoadableFileExtension(const io::path& filename) const;

    virtual bool isALoadableFileFormat(io::IReadFile *file) const;

    //! creates/loads an animated mesh from the file.
    //! \return Pointer to the created mesh. Returns 0 if loading failed.
    //! If you no longer need the mesh, you should call IAnimatedMesh::drop().
    //! See IReferenceCounted::drop() for more information.
    virtual bool loadScene(io::IReadFile* file, scene::ISceneUserDataSerializer* userDataSerializer=0, scene::ISceneNode* rootNode=0);

private:
    scene::ISceneManager* SceneManager;
    io::IFileSystem* FileSystem;

    scene::ISceneNode* RootNode;

    io::path ConfigGamePath;

    LoggerManager* _log;
};

#endif // IO_SCENELOADER_THECOUNCILL_H
