#include <QApplication>

#include "GUI_MainWindow.h"
#include "UIThemeManager.h"
#include "Log/ConsoleLogger.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifdef IS_A_DEVELOPMENT_BUILD
    ConsoleLogger consoleLogger;
    LoggerManager::Instance()->registerLogger(&consoleLogger, Logger_Dev);
#endif

    // Load setting.xml
    Settings::loadFromXML(QCoreApplication::applicationDirPath() + "/config.xml");
    UIThemeManager::Init();
    UIThemeManager::SetTheme(Settings::_theme);

    GUI_MainWindow w;
    w.show();
    w.initIrrlicht();

    const QStringList arguments = QCoreApplication::arguments();
    for (int i = 1; i < arguments.size(); ++i)
    {
        w.addFileGeneric(arguments.at(i));
    }

    return a.exec();
}
