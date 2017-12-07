#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>
#include <QColorDialog>
#include <QFileDialog>
#include <QProcess>

#include "Translator.h"
#include "GUI_ExtFilesExplorer.h"
#include "Settings.h"

namespace Ui {
class GUI_Options;
}


class GUI_Options : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Options(QWidget *parent = 0, QString loadedFile = QString(), QIrrlichtWidget *irr = 0);
    ~GUI_Options();

public slots:
    void reset();
    void ok();
    void selectColor();
    void changeExport();
    void selectDir();
    void selectTW3TexDir();

private:
    QColor _col;
    QString _filename;
    Ui::GUI_Options *_ui;

    QIrrlichtWidget *_irr;

signals:
    void optionsValidation();
};

#endif // OPTIONS_H
