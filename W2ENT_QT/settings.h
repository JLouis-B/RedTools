#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <vector3d.h>


#define IS_A_DEVELOPMENT_BUILD

// Assimp is used to import and export many file formats. You can disable it with this define
#define COMPILE_WITH_ASSIMP

// in the case of a release build, COMPILE_WITH_ASSIMP should be enabled
#ifndef IS_A_DEVELOPMENT_BUILD
#define COMPILE_WITH_ASSIMP
#endif

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

class Settings
{
private:
    static QString _appVersion;

public:
    static double _camSpeed;
    static double _camRotSpeed;

    static int _r;
    static int _g;
    static int _b;

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

    static QString getAppVersion();
    static QString getExportFolder();

    static void loadFromXML(QString filename);
    static void saveToXML(QString filename);

    static QString getFilters();

};

class MeshSize
{
public:
    static irr::core::vector3df _originalDimensions;    // original size in cm
    static irr::core::vector3df _dimensions;            // size in cm
};

#endif // SETTINGS_H
