#ifndef UTILS_QT_IRR_H
#define UTILS_QT_IRR_H

#include <IFileSystem.h>
#include <QString>
#include <QColor>
#include <SColor.h>

QString IRRPATH_TO_QSTRING(irr::io::path irrPath);
irr::io::path QSTRING_TO_IRRPATH(QString qString);

QString IRRSTRING_TO_QSTRING(irr::core::stringc irrString);
irr::core::stringc QSTRING_TO_IRRSTRING(QString qString);

int QKEY_TO_IRRKEY(int qKey);
irr::video::SColor QCOLOR_TO_IRRCOLOR(QColor qColor);


#endif // UTILS_QT_IRR_H
