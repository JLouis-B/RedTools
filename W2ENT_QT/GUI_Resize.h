#ifndef RESIZE_H
#define RESIZE_H

#include <QDialog>

#include "Settings.h"


namespace Ui {
class GUI_Resize;
}


using namespace irr;

class GUI_Resize : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Resize(QWidget *parent = nullptr);
    ~GUI_Resize();


public slots:
    void changeX();
    void changeY();
    void changeZ();
    void changeUnit(QString unit);

    void cancel();


private:
    Ui::GUI_Resize *_ui;
    float _ratioY;
    float _ratioZ;

    core::vector3df _initialDimensions;
    Unit _initialUnit;

    void setNewSize();
};

#endif // RESIZE_H
