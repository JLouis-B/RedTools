#include "Utils_Qt_Irr.h"

QString PATH_TO_QSTRING(irr::io::path path)
{
    #ifdef _IRR_WCHAR_FILESYSTEM
        return QString::fromWCharArray(path.c_str());
    #else
        return QString(path.c_str());
    #endif
}


irr::io::path QSTRING_TO_PATH(QString str)
{
    #ifdef _IRR_WCHAR_FILESYSTEM
        return str.toStdWString().c_str();
    #else
        return str.toStdString().c_str();
    #endif
}
