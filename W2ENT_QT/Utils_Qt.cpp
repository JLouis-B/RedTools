#include "Utils_Qt.h"

void keepOnlyInDevBuild(QWidget* widget)
{
#ifndef IS_A_DEVELOPMENT_BUILD
    widget->deleteLater();
#endif
}

void keepOnlyInDevBuild(QAction* action)
{
#ifndef IS_A_DEVELOPMENT_BUILD
    action->deleteLater();
#endif
}
