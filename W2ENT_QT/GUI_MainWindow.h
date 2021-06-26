#ifndef GUI_MAINWINDOW_H
#define GUI_MAINWINDOW_H

#include "QIrrlichtWidget.h"

#include <QMainWindow>

namespace Ui {
class GUI_MainWindow;
}


class GUI_MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GUI_MainWindow(QWidget* parent = nullptr);
    ~GUI_MainWindow();
    void initIrrlicht();

public slots :
    void convert();
    void translate();
    void changeLanguage();
    void selectFolder();

    void openAbout();
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


    void bifExtractor();
    void dzipExtractor();
    void bundleExtractor();
    void thecouncilExtractor();
    void dishonoredExtractor();
    void changeBaseDir(QString newDir);


    void selectRigFile();
    void selectAnimationsFile();

    void selectTW1AnimationsFile();

    void selectTheCouncilTemplate();

    void loadFileGeneric(QString path);
    void addFileGeneric(QString path);
    void onLoadMeshClicked();
    void replaceMesh(QString path);
    void loadRig(QString path);
    void loadAnimations(QString path);

    void onAddMeshClicked();
    void addMeshes(QStringList filenames);

    void loadTW1Animations(QString path);

    void loadTheCouncilTemplate(QString path);

    void logLoadingResult(bool result);


private:
    void updateWindowTitle();
    void logToUser(core::stringc log);
    void registerExporters();

    Ui::GUI_MainWindow* _ui;
    QIrrlichtWidget* _irrWidget;

    bool _firstSelection;
    LOD _currentLOD;

    QVector<ExporterInfos> _exporters;

signals:
    void languageChanged();

};

#endif // GUI_MAINWINDOW_H
