#ifndef RESIZE_H
#define RESIZE_H

#include <QDialog>
#include <irrlicht.h>


namespace Ui {
class ReSize;
}

enum Unit
{
    Unit_m,
    Unit_cm
};

class ReSize : public QDialog
{
    Q_OBJECT

public:
    explicit ReSize(QWidget *parent = 0);
    ~ReSize();

    static irr::core::vector3df _originalDimensions;    // original size in cm
    static irr::core::vector3df _dimensions;            // size in cm
    static Unit _unit;


public slots:
    void changeX();
    void changeY();
    void changeZ();
    void changeUnit(QString unit);
    void cancel();

    void destroyWindow();



private:
    Ui::ReSize *ui;
    float _rapportY;
    float _rapportZ;

    irr::core::vector3df _initialDimensions;
    Unit _initialUnit;

    void setNewSize();
};

#endif // RESIZE_H
