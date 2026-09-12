#include "Utils_Qt_Irr.h"

#include <Keycodes.h>

QString irrPathToQString(const irr::io::path &irrPath)
{
    #ifdef _IRR_WCHAR_FILESYSTEM
        return QString::fromWCharArray(irrPath.c_str());
    #else
        return QString(irrPath.c_str());
    #endif
}

irr::io::path qStringToIrrPath(const QString& qString)
{
    #ifdef _IRR_WCHAR_FILESYSTEM
        return qString.toStdWString().c_str();
    #else
        return qString.toStdString().c_str();
    #endif
}

QString irrStringToQString(const irr::core::stringc& irrString)
{
    return QString(irrString.c_str());
}

irr::core::stringc qStringToIrrString(const QString& qString)
{
    return qString.toStdString().c_str();
}

int qKeyToIrrKey(int qKey)
{
    switch(qKey)
    {
    case Qt::Key_Left:
        return irr::KEY_LEFT;
    case Qt::Key_Right:
        return irr::KEY_RIGHT;
    case Qt::Key_Up:
        return irr::KEY_UP;
    case Qt::Key_Down:
        return irr::KEY_DOWN;

    default:
        return qKey;
    }
}

irr::video::SColor qColorToIrrColor(const QColor &qColor)
{
    return irr::video::SColor(qColor.alpha(), qColor.red(), qColor.green(), qColor.blue());
}
