#ifndef UTILS_QT_H
#define UTILS_QT_H

#include <QWidget>
#include <QAction>
#include "CompileConfig.h"

void keepOnlyInDevBuild(QWidget* widget);
void keepOnlyInDevBuild(QAction* action);

#endif // UTILS_QT_H
