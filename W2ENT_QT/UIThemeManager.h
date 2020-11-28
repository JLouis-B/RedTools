#ifndef UITHEMEMANAGER_H
#define UITHEMEMANAGER_H

#include <QPalette>
#include <QStyle>

class Theme
{
public:
    QString _name;
    QString _styleName;
    QPalette _palette;
};

class UIThemeManager
{
public:
    UIThemeManager();

    static void Init();
    static QList<QString> GetAvailableThemes();
    static void SetTheme(QString themeName);

private:
    static QMap<QString, Theme> _themes;
};

#endif // UITHEMEMANAGER_H
