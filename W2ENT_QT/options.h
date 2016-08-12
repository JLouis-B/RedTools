#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>
#include <QColorDialog>
#include <QFileDialog>
#include <QProcess>

#include "translator.h"
#include "extfiles.h"
#include "settings.h"

namespace Ui {
class Options;
}


class Options : public QDialog
{
    Q_OBJECT

public:
    explicit Options(QWidget *parent = 0, QString loadedFile = QString(), QIrrlichtWidget *irr = 0);
    ~Options();

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
    Ui::Options *_ui;

    QIrrlichtWidget *_irr;

signals:
    void optionsValidation();
};

#endif // OPTIONS_H
