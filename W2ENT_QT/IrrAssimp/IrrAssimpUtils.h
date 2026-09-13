#ifndef IRRASSIMPUTILS
#define IRRASSIMPUTILS

#include <IFileSystem.h>
#include <assimp/Logger.hpp>

namespace IrrAssimp
{

aiString irrToAssimpPath(const irr::io::path& path);

}

#endif // IRRASSIMPUTILS
