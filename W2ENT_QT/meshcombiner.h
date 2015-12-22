#ifndef MESHCOMBINER
#define MESHCOMBINER

#include <irrlicht.h>

using namespace irr;

void combineMeshes(scene::ISkinnedMesh* source, scene::ISkinnedMesh* addition);
scene::ISkinnedMesh* copySkinnedMesh(scene::ISceneManager* Smgr, scene::ISkinnedMesh* meshToCopy);


#endif // MESHCOMBINER

