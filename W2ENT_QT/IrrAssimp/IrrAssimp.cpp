#include "IrrAssimp.h"

#include <ISceneManager.h>

using namespace irr;

IrrAssimp::IrrAssimp(irr::scene::ISceneManager* smgr)
    : m_sceneManager(smgr),
      m_meshCache(smgr->getMeshCache()),
      m_fileSystem(smgr->getFileSystem()),
      m_importer(smgr),
      m_exporter()
{

}

IrrAssimp::~IrrAssimp()
{

}

void IrrAssimp::exportMesh(irr::scene::IMesh* mesh, irr::core::stringc format, irr::core::stringc path)
{
    m_exporter.writeFile(mesh, format, path);
}

irr::scene::IAnimatedMesh* IrrAssimp::getMesh(const io::path& path)
{
    scene::IAnimatedMesh* mesh = m_meshCache->getMeshByName(path);
    if (mesh)
        return mesh;

    io::IReadFile* file = m_fileSystem->createAndOpenFile(path);
	if (!file)
	{
		//os::Printer::log("Could not load mesh, because file could not be opened: ", path, ELL_ERROR);
		return 0;
	}

	if (isLoadable(path))
    {
        mesh = m_importer.createMesh(file);

        if (mesh)
        {
            m_meshCache->addMesh(path, mesh);
            mesh->drop();
        }
    }

	file->drop();

    /*
        if (!msh)
            os::Printer::log("Could not load mesh, file format seems to be unsupported", filename, ELL_ERROR);
        else
            os::Printer::log("Loaded mesh", filename, ELL_INFORMATION);
    */

    return mesh;
}

irr::core::stringc IrrAssimp::getError()
{
    return m_importer.error;
}


core::array<ExportFormat> IrrAssimp::getExportFormats()
{
    core::array<ExportFormat> formats;

    Assimp::Exporter exporter;
    for (size_t i = 0; i < exporter.GetExportFormatCount(); ++i)
    {
         const aiExportFormatDesc* formatDesc = exporter.GetExportFormatDescription(i);
         formats.push_back(ExportFormat(formatDesc->fileExtension, formatDesc->id, formatDesc->description));
    }

    return formats;
}


bool IrrAssimp::isLoadable(irr::io::path path)
{
    return m_importer.isALoadableFileExtension(path);
}
