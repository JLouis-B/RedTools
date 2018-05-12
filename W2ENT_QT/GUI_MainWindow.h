#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "QIrrlichtWidget.h"
#include "GUI_Search.h"
#include "GUI_Options.h"
#include "GUI_Resize.h"
#include "GUI_CleanTexturesPath.h"
#include "GUI_Extractor_TW1_BIF.h"
#include "GUI_Extractor_TW2_DZIP.h"
#include "GUI_Extractor_TW3_CACHE.h"
#include "GUI_Extractor_TW3_BUNDLE.h"
#include "GUI_MaterialsExplorer.h"

#include <iostream>
#include <QDesktopServices>

namespace Ui {
class GUI_MainWindow;
}


class GUI_MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GUI_MainWindow(QWidget *parent = 0);
    ~GUI_MainWindow();
    void initIrrlicht();

public slots :

    void convert();
    void translate();
    void changeLanguage();
    void selectFolder();
    void changeWireframe(bool enable);
    void changeRigging(bool enable);
    void openWebpage();
    void options();
    void search();
    void matExplorer();

    void changeOptions();
    void re_size();
    void changeLOD();
    void clearLOD();
    void clearAllLODs();
    void checkConvertButton();
    void cleanTexturesPath();
    void extFiles();


    void addMesh();
    void bifExtractor();
    void dzipExtractor();
    void bundleExtractor();
    void changeBaseDir(QString newDir);


    void selectMeshFile();
    void selectRigFile();
    void selectAnimationsFile();

    void loadFileGeneric(QString path);
    void loadMesh(QString path);
    void loadRig(QString path);
    void loadAnimations(QString path);


private:
    void updateWindowTitle();
    void addToUILog(QString log);
    void fillComboBoxFormats();

    Ui::GUI_MainWindow *_ui;
    QIrrlichtWidget *_irrWidget;

    bool _firstSelection;
    LOD _currentLOD;

    QVector<ExporterInfos> _exporters;

signals:
    void languageChanged();

};

#endif // MAINWINDOW_H
