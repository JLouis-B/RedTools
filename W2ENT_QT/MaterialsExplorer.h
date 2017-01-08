#ifndef MATERIALSEXPLORER_H
#define MATERIALSEXPLORER_H

#include <QDialog>
#include "extfiles.h"
#include "qirrlichtwidget.h"

namespace Ui {
class MaterialsExplorer;
}

class MaterialsExplorer : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialsExplorer(QWidget *parent = 0, QIrrlichtWidget* irrlicht = 0);
    ~MaterialsExplorer();

public slots :
    void selectFile();

private:
    QIrrlichtWidget* _irrlicht;
    Ui::MaterialsExplorer *_ui;

    void loadTW3Materials(QString file);
    void read(QString filename);
};

#endif // MATERIALSEXPLORER_H
