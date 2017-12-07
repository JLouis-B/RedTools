#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "QIrrlichtWidget.h"
#include "GUI_Search.h"
#include "GUI_Options.h"
#include "GUI_Resize.h"
#include "GUI_CleanTexturesPath.h"
#include "GUI_Extractor_TW1_BIF.h"
#include "GUI_Extractor_TW2_DZIP.h"
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
    void selectFile();
    void convertir();
    void translate();
    void changeLanguage();
    void selectFolder();
    void changeWireframe(bool enable);
    void changeRigging(bool enable);
    void openWebpage();
    void options();
    void search();
    void matExplorer();
    void loadFile(QString path);
    void changeOptions();
    void re_size();
    void changeLOD();
    void clearLOD();
    void clearAllLODs();
    void checkConvertButton();
    void cleanTexturesPath();
    void extFiles();
    void loadRig();
    void loadAnimations();
    void addMesh();
    void bifExtractor();
    void dzipExtractor();
    void changeBaseDir(QString newDir);


private:
    void updateWindowTitle();
    void addToUILog(QString log);

    Ui::GUI_MainWindow *_ui;
    QIrrlichtWidget *_irrWidget;
    QString _formats;


    bool _firstSelection;
    LOD _currentLOD;

signals:
    void languageChanged();

};

#endif // MAINWINDOW_H
