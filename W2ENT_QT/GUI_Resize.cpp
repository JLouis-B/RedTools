#include "GUI_Resize.h"
#include "ui_GUI_Resize.h"


GUI_Resize::GUI_Resize(QWidget *parent) :
     QDialog(parent),
    _ui(new Ui::GUI_Resize)
{
    _ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    _unit = Settings::_unit;
    if (_unit == Unit_m)
    {
        _ui->combo_unit->setCurrentText("m");
        _ui->label_unitX->setText("m");
        _ui->label_unitY->setText("m");
        _ui->label_unitZ->setText("m");
    }

    QObject::connect(_ui->spinbox_sizeX, SIGNAL(valueChanged(double)), this, SLOT(changeX(double)));
    QObject::connect(_ui->spinbox_sizeY, SIGNAL(valueChanged(double)), this, SLOT(changeY(double)));
    QObject::connect(_ui->spinbox_sizeZ, SIGNAL(valueChanged(double)), this, SLOT(changeZ(double)));
    QObject::connect(_ui->combo_unit, SIGNAL(currentTextChanged(QString)), this, SLOT(changeUnit(QString)));

    QObject::connect(this, SIGNAL(accepted()), this, SLOT(validateNewSize()));
}

GUI_Resize::~GUI_Resize()
{
    delete _ui;
}

void GUI_Resize::setMeshOriginalDimensions(core::vector3df originalDimensions)
{
    _originalDimensions = originalDimensions;
    core::vector3df dimensions = originalDimensions * MeshSize::_scaleFactor;
    if (_unit == Unit_m)
    {
        dimensions /= 100.f;
    }

    enableEvents(false);
    _ui->spinbox_sizeX->setValue(dimensions.X);
    _ui->spinbox_sizeY->setValue(dimensions.Y);
    _ui->spinbox_sizeZ->setValue(dimensions.Z);
    enableEvents(true);

    // compute ratios and checks null values
    if (dimensions.X != 0.f)
    {
        _ratioYX = dimensions.Y / dimensions.X;
        _ratioZX = dimensions.Z / dimensions.X;
    }
    else
        _ui->spinbox_sizeX->setEnabled(false);

    if (dimensions.Y != 0.f)
    {
        _ratioXY = dimensions.X / dimensions.Y;
        _ratioZY = dimensions.Z / dimensions.Y;
    }
    else
        _ui->spinbox_sizeY->setEnabled(false);

    if (dimensions.Z != 0.f)
    {
        _ratioXZ = dimensions.X / dimensions.Z;
        _ratioYZ = dimensions.Y / dimensions.Z;
    }
    else
        _ui->spinbox_sizeZ->setEnabled(false);
}

void GUI_Resize::validateNewSize()
{
    core::vector3df newSize = core::vector3df(_ui->spinbox_sizeX->value(), _ui->spinbox_sizeY->value(), _ui->spinbox_sizeZ->value());
    Settings::_unit = _unit;
    if (_unit == Unit_m)
        newSize *= 100.f; // m to cm

    if (newSize.X != 0.f && _originalDimensions.X != 0.f)
        MeshSize::_scaleFactor = newSize.X / _originalDimensions.X;
    else if (newSize.Y != 0.f && _originalDimensions.Y != 0.f)
        MeshSize::_scaleFactor = newSize.Y / _originalDimensions.Y;
    else if (newSize.Z != 0.f && _originalDimensions.Z != 0.f)
        MeshSize::_scaleFactor = newSize.Z / _originalDimensions.Z;
    else
        MeshSize::_scaleFactor = 1.f;
}

void GUI_Resize::changeX(double newValue)
{
    enableEvents(false);
    _ui->spinbox_sizeY->setValue(newValue * _ratioYX);
    _ui->spinbox_sizeZ->setValue(newValue * _ratioZX);
    enableEvents(true);
}

void GUI_Resize::changeY(double newValue)
{
    enableEvents(false);
    _ui->spinbox_sizeX->setValue(newValue * _ratioXY);
    _ui->spinbox_sizeZ->setValue(newValue * _ratioZY);
    enableEvents(true);
}

void GUI_Resize::changeZ(double newValue)
{
    enableEvents(false);
    _ui->spinbox_sizeX->setValue(newValue * _ratioXZ);
    _ui->spinbox_sizeY->setValue(newValue * _ratioYZ);
    enableEvents(true);
}

void GUI_Resize::changeUnit(QString unit)
{
    if (unit == "cm")
    {
        _ui->label_unitX->setText("cm");
        _ui->label_unitY->setText("cm");
        _ui->label_unitZ->setText("cm");
        _unit = Unit_cm;
        enableEvents(false);
        _ui->spinbox_sizeX->setValue(_ui->spinbox_sizeX->value() * 100.);
        _ui->spinbox_sizeY->setValue(_ui->spinbox_sizeY->value() * 100.);
        _ui->spinbox_sizeZ->setValue(_ui->spinbox_sizeZ->value() * 100.);
        enableEvents(true);
    }
    else if (unit == "m")
    {
        _ui->label_unitX->setText("m");
        _ui->label_unitY->setText("m");
        _ui->label_unitZ->setText("m");
        _unit = Unit_m;
        enableEvents(false);
        _ui->spinbox_sizeX->setValue(_ui->spinbox_sizeX->value() / 100.);
        _ui->spinbox_sizeY->setValue(_ui->spinbox_sizeY->value() / 100.);
        _ui->spinbox_sizeZ->setValue(_ui->spinbox_sizeZ->value() / 100.);
        enableEvents(true);
    }
}

void GUI_Resize::enableEvents(bool enabled)
{
    _ui->spinbox_sizeX->blockSignals(!enabled);
    _ui->spinbox_sizeY->blockSignals(!enabled);
    _ui->spinbox_sizeZ->blockSignals(!enabled);
}
