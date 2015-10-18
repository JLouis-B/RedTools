#ifndef IRRASSIMPEXPORT_H
#define IRRASSIMPEXPORT_H

#include <irrlicht.h>
#include "IrrAssimpExport.h"

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
};

#endif // IRRASSIMPEXPORT_H
