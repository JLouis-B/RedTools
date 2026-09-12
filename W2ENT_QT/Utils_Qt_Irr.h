#ifndef UTILS_QT_IRR_H
#define UTILS_QT_IRR_H

// Irrlicht
#include <IFileSystem.h>
#include <SColor.h>

// Qt
#include <QString>
#include <QColor>

namespace QtIrr
{
    QString irrPathToQString(const irr::io::path& irrPath);
    irr::io::path qStringToIrrPath(const QString& qString);

    QString irrStringToQString(const irr::core::stringc& irrString);
    irr::core::stringc qStringToIrrString(const QString& qString);

    int qKeyToIrrKey(int qKey);
    irr::video::SColor qColorToIrrColor(const QColor& qColor);
}


#endif // UTILS_QT_IRR_H
