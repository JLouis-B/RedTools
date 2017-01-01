#ifndef EXTFILES_H
#define EXTFILES_H

#include <QDialog>
#include <QFileDialog>
#include <IReadFile.h>

#include "utils.h"

using namespace irr;

class QIrrlichtWidget;

enum WitcherFileType
{
    WFT_WITCHER_2,
    WFT_WITCHER_3,
    WFT_NOT_WITCHER
};

WitcherFileType getFileType(QIrrlichtWidget* irrlicht, io::path filename);

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

    core::array<core::stringc> readTW3File(io::path filename);
    core::array<core::stringc> readTW2File(io::path filename);


    core::array<core::stringc> read(io::path filename);

    QString _back;
};

#endif // EXTFILES_H
