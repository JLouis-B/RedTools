#ifndef SETTINGS_H
#define SETTINGS_H

#include "CompileConfig.h"
#include <QString>
#include <QColor>
#include <vector3d.h>




#define DEFAULT_CAM_ROT_SPEED 500
#define DEFAULT_CAM_SPEED 500

enum Unit
{
    Unit_m,
    Unit_cm
};

enum Export_Mode
{
    Export_BaseDir,
    Export_Custom
};

struct WindowState
{
    bool _initialised;

    QByteArray _geometry;

    WindowState()
    {
        _initialised = false;
    }
};

struct SearchSettings
{
    bool _searchRedMeshes;
    bool _searchRedRigs;
    bool _searchRedAnimations;
    QString _additionnalExtensions;
};

class Settings
{
private:
    static QString _appVersion;

public:
    static double _camSpeed;
    static double _camRotSpeed;

    static QColor _backgroundColor;

    static Export_Mode _mode;
    static QString _exportDest;

    static bool _copyTextures;
    static bool _nm;
    static bool _sm;

    static bool _convertTextures;
    static QString _texFormat;

    static bool _debugLog;

    static QString _baseDir;

    static QString _language;

    static QString _TW3TexPath;
    static bool _TW3LoadSkel;
    static bool _TW3LoadBestLOD;

    static QString _formats;

    static bool _firstUse;

    static QString _exporter;
    static QString _selectedFilter;

    static Unit _unit;

    static SearchSettings _searchSettings;

    static WindowState _windowState;

    static QString getAppVersion();
    static QString getExportFolder();

    static void loadFromXML(QString filename);
    static void saveToXML(QString filename);

    static QString getFilters();

};

class MeshSize
{
public:
    static float _scaleFactor;
};

#endif // SETTINGS_H
