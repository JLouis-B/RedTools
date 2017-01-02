#ifndef EXTFILES_H
#define EXTFILES_H

#include <QDialog>
#include <QFileDialog>
#include <IReadFile.h>

#include "utils.h"
#include "TW_Utils.h"

using namespace irr;

class QIrrlichtWidget;



namespace Ui {
class ExtFiles;
}


class ExtFiles : public QDialog
{
    Q_OBJECT

public:
    explicit ExtFiles(QIrrlichtWidget* irrlicht, QWidget *parent = 0);
    ~ExtFiles();

    void read(QString filename);

public slots :
    void selectFile();

    // w2mi stuff
    void checkW2MI();
    void changeSelection(QString newSelectedText);
    void back();

private:
    QIrrlichtWidget* _irrlicht;
    Ui::ExtFiles* _ui;

    core::array<core::stringc> readTW3File(io::IReadFile *file);
    core::array<core::stringc> readTW2File(io::IReadFile *file);

    QString _back;
};

#endif // EXTFILES_H
