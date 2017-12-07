#ifndef EXTFILES_H
#define EXTFILES_H

#include <QDialog>
#include <QFileDialog>
#include <IReadFile.h>

#include "Utils_Qt_Irr.h"
#include "Utils_TW.h"

using namespace irr;

class QIrrlichtWidget;



namespace Ui {
class GUI_ExtFilesExplorer;
}


class GUI_ExtFilesExplorer : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_ExtFilesExplorer(QIrrlichtWidget* irrlicht, QWidget *parent = 0);
    ~GUI_ExtFilesExplorer();

    void read(QString filename);

public slots :
    void selectFile();

    // w2mi stuff
    void checkW2MI();
    void changeSelection(QString newSelectedText);
    void back();

private:
    QIrrlichtWidget* _irrlicht;
    Ui::GUI_ExtFilesExplorer* _ui;

    core::array<core::stringc> readTW2File(io::IReadFile *file, core::array<core::stringc>& files);

    QString _back;
};

#endif // EXTFILES_H
