#ifndef UTILS_QT_IRR_H
#define UTILS_QT_IRR_H

#include <IFileSystem.h>
#include <QString>
#include <QColor>
#include <SColor.h>

QString irrPathToQString(irr::io::path irrPath);
irr::io::path qStringToIrrPath(QString qString);

QString irrStringToQString(irr::core::stringc irrString);
irr::core::stringc qStringToIrrString(QString qString);

int qKeyToIrrKey(int qKey);
irr::video::SColor qColorToIrrColor(QColor qColor);


#endif // UTILS_QT_IRR_H
