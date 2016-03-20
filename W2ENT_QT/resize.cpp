#include "resize.h"
#include "ui_resize.h"

irr::core::vector3df ReSize::_originalDimensions = irr::core::vector3df(0, 0, 0);
irr::core::vector3df ReSize::_dimensions = irr::core::vector3df(0, 0, 0);
Unit ReSize::_unit = Unit_m;

ReSize::ReSize(QWidget *parent) :
     QDialog(parent),
    ui(new Ui::ReSize)
{
    ui->setupUi(this);

    irr::core::vector3df size = _dimensions;

    if (_unit == Unit_m)
    {
        ui->combo_unit->setCurrentText("m");
        ui->label_unitX->setText("m");
        ui->label_unitY->setText("m");
        ui->label_unitZ->setText("m");
        size /= 100.f;
    }

    ui->spinbox_sizeX->setValue(size.X);
    ui->spinbox_sizeY->setValue(size.Y);
    ui->spinbox_sizeZ->setValue(size.Z);

    _rapportY = ui->spinbox_sizeY->value()/ui->spinbox_sizeX->value();
    _rapportZ = ui->spinbox_sizeZ->value()/ui->spinbox_sizeX->value();

    QObject::connect(ui->spinbox_sizeX, SIGNAL(valueChanged(double)), this, SLOT(changeX()));
    QObject::connect(ui->spinbox_sizeY, SIGNAL(valueChanged(double)), this, SLOT(changeY()));
    QObject::connect(ui->spinbox_sizeZ, SIGNAL(valueChanged(double)), this, SLOT(changeZ()));
    QObject::connect(ui->combo_unit, SIGNAL(currentTextChanged(QString)), this, SLOT(changeUnit(QString)));

    QObject::connect(this, SIGNAL(rejected()), this, SLOT(cancel()));
    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));

    _initialUnit = _unit;
    _initialDimensions = _dimensions;
}

ReSize::~ReSize()
{
    delete ui;
}

void ReSize::destroyWindow()
{
    delete this;
}

void ReSize::cancel()
{
    _unit = _initialUnit;
    _dimensions = _initialDimensions;
}

void ReSize::setNewSize()
{
    _dimensions = irr::core::vector3df(ui->spinbox_sizeX->value(), ui->spinbox_sizeY->value(), ui->spinbox_sizeZ->value());
    if (_unit == Unit_m)
        _dimensions *= 100.f; // m to cm
}

void ReSize::changeX()
{
    ui->spinbox_sizeY->setValue(ui->spinbox_sizeX->value() * _rapportY);
    ui->spinbox_sizeZ->setValue(ui->spinbox_sizeX->value() * _rapportZ);
    setNewSize();
}

void ReSize::changeY()
{
    ui->spinbox_sizeX->setValue(ui->spinbox_sizeY->value() / _rapportY);
    ui->spinbox_sizeZ->setValue(ui->spinbox_sizeX->value() * _rapportZ);
    setNewSize();
}

void ReSize::changeZ()
{
    ui->spinbox_sizeX->setValue(ui->spinbox_sizeZ->value() / _rapportZ);
    ui->spinbox_sizeY->setValue(ui->spinbox_sizeX->value() * _rapportY);
    setNewSize();
}

void ReSize::changeUnit(QString unit)
{
    if (unit == "cm")
    {
        ui->label_unitX->setText("cm");
        ui->label_unitY->setText("cm");
        ui->label_unitZ->setText("cm");
        _unit = Unit_cm;
        ui->spinbox_sizeX->setValue(ui->spinbox_sizeX->value() * 100.f);
    }
    else if (unit == "m")
    {
        ui->label_unitX->setText("m");
        ui->label_unitY->setText("m");
        ui->label_unitZ->setText("m");
        _unit = Unit_m;
        ui->spinbox_sizeX->setValue(ui->spinbox_sizeX->value() / 100.f);
    }
}
