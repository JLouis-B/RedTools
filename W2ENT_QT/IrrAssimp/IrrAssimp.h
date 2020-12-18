#ifndef IRRASSIMP_H
#define IRRASSIMP_H

#include "IrrAssimpExport.h"
#include "IrrAssimpImport.h"

#include <IMeshCache.h>

struct ExportFormat
{
    irr::core::stringc fileExtension;
    irr::core::stringc id;
    irr::core::stringc description;

    ExportFormat(irr::core::stringc t_fileExtension, irr::core::stringc t_id, irr::core::stringc t_description)
    : fileExtension(t_fileExtension), id(t_id), description(t_description) {}
};

class IrrAssimp
{
    public:
        explicit IrrAssimp(irr::scene::ISceneManager* smgr);
        virtual ~IrrAssimp();


        /*  Get a mesh with Assimp.
            Like ISceneManager::getMesh, check if the mesh is already in the MeshCache, and if it's not the case, Assimp load it.
        */
        irr::scene::IAnimatedMesh* getMesh(const irr::io::path& path);


        /*  Export a mesh.
            The "format" parameter correspond to the Assimp format ID.
        */
        void exportMesh(irr::scene::IMesh* mesh, irr::core::stringc format, irr::core::stringc path);

        // Return the error of the last loading. Return an empty string if no error.
        irr::core::stringc getError();


        // Check if the file has a loadable extension
        bool isLoadable(irr::io::path path);

        // Return the list of available export formats
        static irr::core::array<ExportFormat> getExportFormats();

    private:
        irr::scene::ISceneManager* m_sceneManager;
        irr::scene::IMeshCache* m_meshCache;
        irr::io::IFileSystem* m_fileSystem;

        IrrAssimpImport m_importer;
        IrrAssimpExport m_exporter;
};

#endif // IRRASSIMP_H
