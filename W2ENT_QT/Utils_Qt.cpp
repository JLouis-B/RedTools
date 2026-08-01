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

QString ensureTrailingSlash(QString path)
{
    if (!path.isEmpty() && !path.endsWith('/') && !path.endsWith('\\'))
        path.push_back('/');

    return path;
}
