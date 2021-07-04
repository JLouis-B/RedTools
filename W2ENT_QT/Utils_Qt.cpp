#include "Utils_Qt.h"

void deleteInReleaseBuild(QWidget* widget)
{
#ifndef IS_A_DEVELOPMENT_BUILD
    widget->deleteLater();
#endif
}

void deleteInReleaseBuild(QAction* action)
{
#ifndef IS_A_DEVELOPMENT_BUILD
    action->deleteLater();
#endif
}

QString cleanPath(QString path)
{
    if (path.size() > 0 && (path[path.size() - 1] != '/' || path[path.size() - 1] != '\\'))
        path.push_back('/');

    return path;
}
