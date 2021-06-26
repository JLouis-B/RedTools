#include "Utils_Qt_Irr.h"

#include <Keycodes.h>

QString IRRPATH_TO_QSTRING(irr::io::path irrPath)
{
    #ifdef _IRR_WCHAR_FILESYSTEM
        return QString::fromWCharArray(irrPath.c_str());
    #else
        return QString(irrPath.c_str());
    #endif
}


irr::io::path QSTRING_TO_IRRPATH(QString qString)
{
    #ifdef _IRR_WCHAR_FILESYSTEM
        return qString.toStdWString().c_str();
    #else
        return qString.toStdString().c_str();
    #endif
}

QString IRRSTRING_TO_QSTRING(irr::core::stringc irrString)
{
    return QString(irrString.c_str());
}

irr::core::stringc QSTRING_TO_IRRSTRING(QString qString)
{
    return qString.toStdString().c_str();
}

int QKEY_TO_IRRKEY(int qKey)
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

irr::video::SColor QCOLOR_TO_IRRCOLOR(QColor color)
{
    return irr::video::SColor(color.alpha(), color.red(), color.green(), color.blue());
}
