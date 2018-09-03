#ifndef RESIZE_H
#define RESIZE_H

#include <QDialog>
#include <vector3d.h>


namespace Ui {
class GUI_Resize;
}

enum Unit
{
    Unit_m,
    Unit_cm
};

using namespace irr;

class GUI_Resize : public QDialog
{
    Q_OBJECT

public:
    explicit GUI_Resize(QWidget *parent = nullptr);
    ~GUI_Resize();

    static core::vector3df _originalDimensions;    // original size in cm
    static core::vector3df _dimensions;            // size in cm
    static Unit _unit;


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
