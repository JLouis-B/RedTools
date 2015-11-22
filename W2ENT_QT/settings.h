#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QtXml>
#include <QMessageBox>

#include "resize.h"

#define DEFAULT_CAM_ROT_SPEED 500
#define DEFAULT_CAM_SPEED 500

enum Export_Mode
{
    Export_Pack0,
    Export_Custom
};

class Settings
{
public:
    static double _camSpeed;
    static double _camRotSpeed;

    static int _r;
    static int _g;
    static int _b;

    static Export_Mode _mode;
    static QString _exportDest;

    static bool _moveTexture;
    static bool _nm;
    static bool _sm;

    static bool _convertTextures;
    static QString _texFormat;

    static bool _debugLog;

    static QString _pack0;

    static QString _language;

    static QString _TW3TexPath;
    static bool _TW3LoadSkel;
    static QString _formats;

    static bool _firstUse;

    static QString getExportFolder();

    static void loadFromXML(QString filename);
    static void saveToXML(QString filename);

};

#endif // SETTINGS_H
