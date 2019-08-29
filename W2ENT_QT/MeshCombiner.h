#ifndef MESHCOMBINER
#define MESHCOMBINER

#include <ISceneManager.h>

using namespace irr;

void combineMeshes(scene::ISkinnedMesh* newMesh, scene::IMesh *addition, bool preserveBones);
scene::ISkinnedMesh* copySkinnedMesh(scene::ISceneManager* smgr, scene::IMesh* meshToCopy, bool preserveBones);


#endif // MESHCOMBINER

