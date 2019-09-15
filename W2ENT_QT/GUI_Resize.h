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
    explicit GUI_Resize(QWidget* parent = nullptr);
    ~GUI_Resize();
    void setMeshOriginalDimensions(core::vector3df originalDimensions);


public slots:
    void changeX(double newValue);
    void changeY(double newValue);
    void changeZ(double newValue);
    void changeUnit(QString unit);

    // validate window
    void validateNewSize();

private:
    Ui::GUI_Resize* _ui;

    float _ratioYX;
    float _ratioZX;
    float _ratioXY;
    float _ratioZY;
    float _ratioXZ;
    float _ratioYZ;

    core::vector3df _originalDimensions;
    Unit _unit;

    void enableEvents(bool enabled);
};

#endif // RESIZE_H
