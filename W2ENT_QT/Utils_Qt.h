#ifndef UTILS_QT_H
#define UTILS_QT_H

#include "CompileConfig.h"

#include <QWidget>
#include <QAction>

void keepOnlyInDevBuild(QWidget* widget);
void keepOnlyInDevBuild(QAction* action);
QString cleanPath(QString path);

#endif // UTILS_QT_H
