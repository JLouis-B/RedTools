#include "Utils_Qt_Irr.h"
#include <Keycodes.h>

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
