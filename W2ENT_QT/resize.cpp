#include "resize.h"
#include "ui_resize.h"

core::vector3df ReSize::_originalDimensions = irr::core::vector3df(0, 0, 0);
core::vector3df ReSize::_dimensions = irr::core::vector3df(0, 0, 0);
Unit ReSize::_unit = Unit_m;

ReSize::ReSize(QWidget *parent) :
     QDialog(parent),
    _ui(new Ui::ReSize)
{
    _ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    core::vector3df size = _dimensions;

    if (_unit == Unit_m)
    {
        _ui->combo_unit->setCurrentText("m");
        _ui->label_unitX->setText("m");
        _ui->label_unitY->setText("m");
        _ui->label_unitZ->setText("m");
        size /= 100.f;
    }

    _ui->spinbox_sizeX->setValue(size.X);
    _ui->spinbox_sizeY->setValue(size.Y);
    _ui->spinbox_sizeZ->setValue(size.Z);

    _ratioY = _ui->spinbox_sizeY->value() / _ui->spinbox_sizeX->value();
    _ratioZ = _ui->spinbox_sizeZ->value() / _ui->spinbox_sizeX->value();

    QObject::connect(_ui->spinbox_sizeX, SIGNAL(valueChanged(double)), this, SLOT(changeX()));
    QObject::connect(_ui->spinbox_sizeY, SIGNAL(valueChanged(double)), this, SLOT(changeY()));
    QObject::connect(_ui->spinbox_sizeZ, SIGNAL(valueChanged(double)), this, SLOT(changeZ()));
    QObject::connect(_ui->combo_unit, SIGNAL(currentTextChanged(QString)), this, SLOT(changeUnit(QString)));

    QObject::connect(this, SIGNAL(rejected()), this, SLOT(cancel()));

    _initialUnit = _unit;
    _initialDimensions = _dimensions;
}

ReSize::~ReSize()
{
    delete _ui;
}

void ReSize::cancel()
{
    _unit = _initialUnit;
    _dimensions = _initialDimensions;
}

void ReSize::setNewSize()
{
    _dimensions = core::vector3df(_ui->spinbox_sizeX->value(), _ui->spinbox_sizeY->value(), _ui->spinbox_sizeZ->value());
    if (_unit == Unit_m)
        _dimensions *= 100.f; // m to cm
}

void ReSize::changeX()
{
    _ui->spinbox_sizeY->setValue(_ui->spinbox_sizeX->value() * _ratioY);
    _ui->spinbox_sizeZ->setValue(_ui->spinbox_sizeX->value() * _ratioZ);
    setNewSize();
}

void ReSize::changeY()
{
    _ui->spinbox_sizeX->setValue(_ui->spinbox_sizeY->value() / _ratioY);
    _ui->spinbox_sizeZ->setValue(_ui->spinbox_sizeX->value() * _ratioZ);
    setNewSize();
}

void ReSize::changeZ()
{
    _ui->spinbox_sizeX->setValue(_ui->spinbox_sizeZ->value() / _ratioZ);
    _ui->spinbox_sizeY->setValue(_ui->spinbox_sizeX->value() * _ratioY);
    setNewSize();
}

void ReSize::changeUnit(QString unit)
{
    if (unit == "cm")
    {
        _ui->label_unitX->setText("cm");
        _ui->label_unitY->setText("cm");
        _ui->label_unitZ->setText("cm");
        _unit = Unit_cm;
        _ui->spinbox_sizeX->setValue(_ui->spinbox_sizeX->value() * 100.f);
    }
    else if (unit == "m")
    {
        _ui->label_unitX->setText("m");
        _ui->label_unitY->setText("m");
        _ui->label_unitZ->setText("m");
        _unit = Unit_m;
        _ui->spinbox_sizeX->setValue(_ui->spinbox_sizeX->value() / 100.f);
    }
}
