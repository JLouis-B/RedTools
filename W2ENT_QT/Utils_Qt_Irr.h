#ifndef UTILS_QT_IRR_H
#define UTILS_QT_IRR_H

#include <IFileSystem.h>
#include <QString>

QString PATH_TO_QSTRING(irr::io::path);
irr::io::path QSTRING_TO_PATH(QString str);
int QKEY_TO_IRRKEY(int qKey);


#endif // UTILS_QT_IRR_H
