#include "UIThemeManager.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

QMap<QString, Theme> UIThemeManager::_themes;

UIThemeManager::UIThemeManager()
{

}

void UIThemeManager::Init()
{
    QStringList styles = QStyleFactory::keys();
    bool hasFusionStyle = false;
    QString windowsStyleName = "Windows";
    if (styles.contains("windowsvista"))
        windowsStyleName = "windowsvista";

    if (styles.contains("Fusion"))
        hasFusionStyle = true;


    // Build the theme list
    // Default
    Theme defaultTheme;
    defaultTheme._name = "Windows";
    defaultTheme._styleName = windowsStyleName;
    defaultTheme._palette = qApp->palette();
    _themes.insert(defaultTheme._name, defaultTheme);

    if (hasFusionStyle)
    {
        // Fusion
        Theme fusionTheme;
        fusionTheme._name = "Fusion";
        fusionTheme._styleName = "Fusion";
        fusionTheme._palette = qApp->palette();
        _themes.insert(fusionTheme._name, fusionTheme);

        // Dark Fusion
        Theme darkFusionTheme;
        darkFusionTheme._name = "Fusion Dark";
        darkFusionTheme._styleName = "Fusion";
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

        darkFusionTheme._palette = darkPalette;
        _themes.insert(darkFusionTheme._name, darkFusionTheme);
    }
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
