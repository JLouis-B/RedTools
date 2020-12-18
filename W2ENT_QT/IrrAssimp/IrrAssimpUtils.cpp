#include "IrrAssimpUtils.h"

aiString irrToAssimpPath(const irr::io::path& path)
{
#ifdef _IRR_WCHAR_FILESYSTEM
    irr::core::stringc rPath = path;
    return aiString(rPath.c_str());
#else
    return aiString(path.c_str());
#endif
}
