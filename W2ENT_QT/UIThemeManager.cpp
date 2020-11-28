#include "UIThemeManager.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

QMap<QString, Theme> UIThemeManager::_themes;

UIThemeManager::UIThemeManager()
{

}

QString getWindowsStyleName()
{
    // Just verify that windowsvista is available
    QStringList styles = QStyleFactory::keys();
    if (styles.contains("windowsvista"))
        return "windowsvista";
    else
        return "windows";
}

void UIThemeManager::Init()
{
    // Build the theme list

    // Default
    Theme defaultTheme;
    defaultTheme._name = "Default";
    defaultTheme._styleName = getWindowsStyleName();
    defaultTheme._palette = qApp->palette();
    _themes.insert(defaultTheme._name, defaultTheme);

    // Dark
    Theme darkTheme;
    darkTheme._name = "Dark";
    darkTheme._styleName = "fusion";
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(15,15,15));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);

    darkPalette.setColor(QPalette::Highlight, QColor(142,45,197).lighter());
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    darkTheme._palette = darkPalette;
    _themes.insert(darkTheme._name, darkTheme);
}

QList<QString> UIThemeManager::GetAvailableThemes()
{
    return _themes.keys();
}

void UIThemeManager::SetTheme(QString themeName)
{
    if (!_themes.contains(themeName))
        return;

    Theme theme = _themes[themeName];
    qApp->setStyle(QStyleFactory::create(theme._styleName));
    qApp->setPalette(theme._palette);
}
