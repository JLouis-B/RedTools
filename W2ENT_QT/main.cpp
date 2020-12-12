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

    for (int i = 1; i < QCoreApplication::arguments().size(); ++i)
    {
        if (i == 1)
        {
            w.replaceMesh(QCoreApplication::arguments().at(1));
        }
        else
        {
            w.addMesh(QStringList(QCoreApplication::arguments().at(i)));
        }
    }

    return a.exec();
}
