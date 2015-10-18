#ifndef EXTFILES_H
#define EXTFILES_H

#include <QDialog>
#include <QFileDialog>
#include <irrlicht.h>

#include "utils.h"

using namespace irr;

class QIrrlichtWidget;

enum WitcherFileType
{
    WFT_WITCHER_2,
    WFT_WITCHER_3,
    WFT_NOT_WITCHER
};

namespace Ui {
class ExtFiles;
}

class ExtFiles : public QDialog
{
    Q_OBJECT

public:
    explicit ExtFiles(QIrrlichtWidget *irrlicht, QWidget *parent = 0);
    ~ExtFiles();

    void read(QString filename);

public slots :
    void close();
    void selectFile();

    // w2mi stuff
    void checkW2MI();
    void changeSelection(QString newSelectedText);
    void back();

private:
    Ui::ExtFiles *ui;

    QIrrlichtWidget *_irrlicht;

    core::array<core::stringc> ReadTW3File(io::path filename);
    core::array<core::stringc> ReadTW2File(io::path filename);

    WitcherFileType getFileType(io::path filename);
    core::array<core::stringc> read(io::path filename);

    QString _back;
};

#endif // EXTFILES_H
