#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qirrlichtwidget.h"
#include "search.h"
#include "options.h"
#include "resize.h"
#include "cleantexturespath.h"
#include "tw1bifextractorui.h"
#include <iostream>
#include <QDesktopServices>
#include <QtConcurrent>

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
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
    void changeBaseDir(QString newDir);


private:
    void updateWindowTitle();

    Ui::MainWindow *_ui;
    QIrrlichtWidget *_irrWidget;
    QString _formats;


    bool _firstSelection;
    LOD _currentLOD;

signals:
    void languageChanged();

};

#endif // MAINWINDOW_H
