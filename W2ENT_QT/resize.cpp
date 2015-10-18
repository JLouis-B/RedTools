#include "resize.h"
#include "ui_resize.h"

irr::core::vector3df ReSize::_originalDimensions = irr::core::vector3df(0,0,0);
irr::core::vector3df ReSize::_dimensions = irr::core::vector3df(0,0,0);
Unit ReSize::_unit = Unit_m;

//:_originalDimensions = irr::core::vector3df(0, 0, 0);


ReSize::ReSize(QWidget *parent) :
     QDialog(parent),
    ui(new Ui::ReSize)
{
    ui->setupUi(this);

    ui->spinbox_sizeX->setValue(_dimensions.X);
    ui->spinbox_sizeY->setValue(_dimensions.Y);
    ui->spinbox_sizeZ->setValue(_dimensions.Z);

    _rapportY = ui->spinbox_sizeY->value()/ui->spinbox_sizeX->value();
    _rapportZ = ui->spinbox_sizeZ->value()/ui->spinbox_sizeX->value();

    if (_unit == Unit_cm)
    {
        ui->combo_unit->setCurrentText("cm");
        ui->label_unitX->setText("cm");
        ui->label_unitY->setText("cm");
        ui->label_unitZ->setText("cm");
    }

    QObject::connect(ui->spinbox_sizeX, SIGNAL(valueChanged(double)), this, SLOT(changeX()));
    QObject::connect(ui->spinbox_sizeY, SIGNAL(valueChanged(double)), this, SLOT(changeY()));
    QObject::connect(ui->spinbox_sizeZ, SIGNAL(valueChanged(double)), this, SLOT(changeZ()));
    QObject::connect(ui->combo_unit, SIGNAL(currentTextChanged(QString)), this, SLOT(changeUnit()));
    QObject::connect(this, SIGNAL(rejected()), this, SLOT(cancel()));

    _SaveUnit = _unit;
    _SaveOriginalDimensions = _originalDimensions;
    _SaveDimensions = _dimensions;
}

ReSize::~ReSize()
{
    delete ui;
}

void ReSize::cancel()
{
    _unit = _SaveUnit;
    _originalDimensions = _SaveOriginalDimensions;
    _dimensions = _SaveDimensions;
}

void ReSize::changeX()
{
    ui->spinbox_sizeY->setValue(ui->spinbox_sizeX->value() * _rapportY);
    ui->spinbox_sizeZ->setValue(ui->spinbox_sizeX->value() * _rapportZ);

    _dimensions = irr::core::vector3df(ui->spinbox_sizeX->value(), ui->spinbox_sizeY->value(), ui->spinbox_sizeZ->value());

}

void ReSize::changeY()
{
    ui->spinbox_sizeX->setValue(ui->spinbox_sizeY->value() / _rapportY);
    ui->spinbox_sizeZ->setValue(ui->spinbox_sizeX->value() * _rapportZ);

    _dimensions = irr::core::vector3df(ui->spinbox_sizeX->value(), ui->spinbox_sizeY->value(), ui->spinbox_sizeZ->value());

}

void ReSize::changeZ()
{
    ui->spinbox_sizeX->setValue(ui->spinbox_sizeZ->value() / _rapportZ);
    ui->spinbox_sizeY->setValue(ui->spinbox_sizeX->value() * _rapportY);

    _dimensions = irr::core::vector3df(ui->spinbox_sizeX->value(), ui->spinbox_sizeY->value(), ui->spinbox_sizeZ->value());
}

void ReSize::changeUnit()
{
    if (ui->combo_unit->currentText() == "cm")
    {
        ui->label_unitX->setText("cm");
        ui->label_unitY->setText("cm");
        ui->label_unitZ->setText("cm");
        _unit = Unit_cm;
        ui->spinbox_sizeX->setValue(ui->spinbox_sizeX->value() * 100.0f);

    }
    else if (ui->combo_unit->currentText() == "m")
    {
        ui->label_unitX->setText("m");
        ui->label_unitY->setText("m");
        ui->label_unitZ->setText("m");
        _unit = Unit_m;
        ui->spinbox_sizeX->setValue(ui->spinbox_sizeX->value() / 100.0f);
    }
}
