#include "GUI_MainWindow.h"
#include <QApplication>
#include "UIThemeManager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Load setting.xml
    Settings::loadFromXML(QCoreApplication::applicationDirPath() + "/config.xml");
    UIThemeManager::Init();
    UIThemeManager::SetTheme(Settings::_theme);

    GUI_MainWindow w;
    w.show();
    w.initIrrlicht();

    if (QCoreApplication::arguments().size() > 1)
        w.loadMesh(QCoreApplication::arguments().at(1));

    return a.exec();
}
