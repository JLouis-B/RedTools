#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>
#include <QColorDialog>
#include <QFileDialog>
#include <QProcess>

#include "translator.h"
#include "extfiles.h"

#define DEFAULT_CAM_ROT_SPEED 500
#define DEFAULT_CAM_SPEED 500

namespace Ui {
class Options;
}

enum Export_Mode
{
    Export_Pack0,
    Export_Custom
};

class OptionsData
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

    static QString getExportFolder();

};


class Options : public QDialog
{
    Q_OBJECT

public:
    explicit Options(QWidget *parent = 0, QString language = QString(), QString loadedFile = QString(), QIrrlichtWidget *irr = 0);
    ~Options();

public slots:
    void reset();
    void ok();
    void selectColor();
    void changeExport();
    void changeDebug();
    void selectDir();

private:
    QString _language;
    QString _filename;
    Ui::Options *_ui;

    QIrrlichtWidget *_irr;

signals:
    void optionsValidation();
};

#endif // OPTIONS_H
