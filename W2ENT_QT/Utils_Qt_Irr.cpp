#include "Utils_Qt_Irr.h"

#include <Keycodes.h>

QString irrPathToQString(irr::io::path irrPath)
{
    #ifdef _IRR_WCHAR_FILESYSTEM
        return QString::fromWCharArray(irrPath.c_str());
    #else
        return QString(irrPath.c_str());
    #endif
}

irr::io::path qStringToIrrPath(QString qString)
{
    #ifdef _IRR_WCHAR_FILESYSTEM
        return qString.toStdWString().c_str();
    #else
        return qString.toStdString().c_str();
    #endif
}

QString irrStringToQString(irr::core::stringc irrString)
{
    return QString(irrString.c_str());
}

irr::core::stringc qStringToIrrString(QString qString)
{
    return qString.toStdString().c_str();
}

int qKeyToIrrKey(int qKey)
{
    if (qKey == 16777234)
        return irr::KEY_LEFT;
    if (qKey == 16777235)
        return irr::KEY_UP;
    if (qKey == 16777236)
        return irr::KEY_RIGHT;
    if (qKey == 16777237)
        return irr::KEY_DOWN;

    return qKey;
}

irr::video::SColor qColorToIrrColor(QColor qColor)
{
    return irr::video::SColor(qColor.alpha(), qColor.red(), qColor.green(), qColor.blue());
}
