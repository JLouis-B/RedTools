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
    QByteArray _windowGeometry;
    bool _checkFolderNames;
    bool _searchRedMeshes;
    bool _searchRedRigs;
    bool _searchRedAnimations;
    QString _additionnalExtensions;
};

class Settings
{
public:
    static double _cameraSpeed;
    static double _cameraRotationSpeed;

    static QColor _backgroundColor;

    static Export_Mode _mode;
    static QString _exportDest;

    static bool _copyTexturesEnabled;
    static bool _copyTexturesSlot1;
    static bool _copyTexturesSlot2;

    static bool _convertTexturesEnabled;
    static QString _convertTexturesFormat;

    static bool _debugLog;

    static QString _baseDir;

    static QString _language;

    static bool _TW1LoadStaticMesh;
    static bool _TW1LoadSkinnedMesh;
    static bool _TW1LoadPaintedMesh;

    static QString _TW3TexPath;
    static bool _TW3LoadSkeletonEnabled;
    static bool _TW3LoadBestLODEnabled;

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
