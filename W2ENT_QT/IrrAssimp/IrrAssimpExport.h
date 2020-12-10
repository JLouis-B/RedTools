#ifndef IRRASSIMPEXPORT_H
#define IRRASSIMPEXPORT_H

#include <ISkinnedMesh.h>

#include <assimp/scene.h>          // Output data structure
#include <assimp/postprocess.h>    // Post processing flags
#include <assimp/Exporter.hpp>

#include "IrrAssimpUtils.h"

class IrrAssimpExport
{
    public:
        IrrAssimpExport();
        virtual ~IrrAssimpExport();
        void writeFile(irr::scene::IMesh* mesh, irr::core::stringc format, irr::core::stringc filename);

    protected:
    private:
        aiScene* AssimpScene;

        void createMeshes(const irr::scene::IMesh* mesh);
        void createMaterials(const irr::scene::IMesh* mesh);
        void createAnimations(const irr::scene::ISkinnedMesh* mesh);
        aiNode* createNode(const irr::scene::ISkinnedMesh::SJoint* joint);

        irr::core::array<irr::u16> getMeshesMovedByBone(const irr::scene::ISkinnedMesh::SJoint* joint);
        std::map<irr::u16, irr::core::array<const irr::scene::ISkinnedMesh::SJoint*> > m_bonesPerMesh;
        std::map<std::pair<irr::u16, const irr::scene::ISkinnedMesh::SJoint*>, irr::u32> m_weightsCountPerMeshesAndBones;
};

#endif // IRRASSIMPEXPORT_H
