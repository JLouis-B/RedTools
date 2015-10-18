#include "options.h"
#include "ui_options.h"




Options::Options(QWidget *parent, QString loadedFile, QIrrlichtWidget* irr) :
    QDialog(parent), _filename(loadedFile),
    _ui(new Ui::Options), _irr(irr)
{
    _ui->setupUi(this);
    _ui->doubleSpinBox_cameraRotSpeed->setValue(Settings::_camRotSpeed);
    _ui->doubleSpinBox_cameraSpeed->setValue(Settings::_camSpeed);

    _ui->checkBox_moveTextures->setChecked(Settings::_moveTexture);
    _ui->checkBox_mtNormalsMap->setChecked(Settings::_nm);
    _ui->checkBox_mtSpecularMap->setChecked(Settings::_sm);


    _ui->checkBox_convertTextures->setChecked(Settings::_convertTextures);
    if (_ui->comboBox_format->findText(Settings::_texFormat) != -1)
        _ui->comboBox_format->setCurrentIndex(_ui->comboBox_format->findText(Settings::_texFormat));
    _ui->comboBox_format->setEnabled(_ui->checkBox_convertTextures->isChecked());

    if(Settings::_mode == Export_Custom)
        _ui->radioButton_custom->setChecked(true);
    else
        _ui->radioButton_pack0->setChecked(true);

    _ui->lineEdit_dir->setText(Settings::_exportDest);

    setFixedSize(this->size());

    _ui->checkBox_moveTextures->setText(Translator::findTranslation("options_export_move_textures"));
    _ui->radioButton_custom->setText(Translator::findTranslation("options_export_to_other"));
    _ui->radioButton_pack0->setText(Translator::findTranslation("options_export_to_pack0"));
    _ui->label_colorSelector->setText(Translator::findTranslation("options_background"));
    _ui->label_movementSpeed->setText(Translator::findTranslation("options_camera_movement_speed"));
    _ui->label_rotSpeed->setText(Translator::findTranslation("options_camera_rot_speed"));
    _ui->View->setTabText(1, Translator::findTranslation("options_view"));
    _ui->View->setTabText(0, Translator::findTranslation("options_export"));
    _ui->checkBox_debug->setText(Translator::findTranslation("options_debug_log"));
    _ui->label_debugLog->setText(Translator::findTranslation("options_debug_log_label"));

    _ui->checkBox_debug->setChecked(Settings::_debugLog);

    _ui->lineEdit_TW3_texFolder->setText(Settings::_TW3TexPath);

    changeExport();


    QObject::connect(_ui->buttonBox, SIGNAL(accepted()), this, SLOT(ok()));
    QObject::connect(_ui->button_colorSelector, SIGNAL(clicked()), this, SLOT(selectColor()));
    QObject::connect(_ui->pushButton_reset, SIGNAL(clicked()), this, SLOT(reset()));

    QObject::connect(_ui->button_selectDir, SIGNAL(clicked()), this, SLOT(selectDir()));
    QObject::connect(_ui->radioButton_custom, SIGNAL(clicked()), this, SLOT(changeExport()));
    QObject::connect(_ui->radioButton_pack0, SIGNAL(clicked()), this, SLOT(changeExport()));

    QObject::connect(_ui->checkBox_moveTextures, SIGNAL(clicked(bool)), _ui->checkBox_mtNormalsMap, SLOT(setEnabled(bool)));
    QObject::connect(_ui->checkBox_moveTextures, SIGNAL(clicked(bool)), _ui->checkBox_mtSpecularMap, SLOT(setEnabled(bool)));

    QObject::connect(_ui->checkBox_convertTextures, SIGNAL(clicked(bool)), _ui->comboBox_format, SLOT(setEnabled(bool)));

    QObject::connect(_ui->checkBox_debug, SIGNAL(clicked()), this, SLOT(changeDebug()));

    QObject::connect(_ui->pushButton_TW3_selectTexFolder, SIGNAL(clicked()), this, SLOT(selectTW3TexDir()));


}

Options::~Options()
{
    delete _ui;
}

void Options::reset()
{
    Settings::_camRotSpeed = DEFAULT_CAM_ROT_SPEED;
    Settings::_camSpeed = DEFAULT_CAM_SPEED;

    _ui->doubleSpinBox_cameraRotSpeed->setValue(Settings::_camRotSpeed);
    _ui->doubleSpinBox_cameraSpeed->setValue(Settings::_camSpeed);

    Settings::_r = 0;
    Settings::_g = 0;
    Settings::_b = 0;
}

void Options::ok()
{
    Settings::_camRotSpeed = _ui->doubleSpinBox_cameraRotSpeed->value();
    Settings::_camSpeed = _ui->doubleSpinBox_cameraSpeed->value();

    Settings::_moveTexture = _ui->checkBox_moveTextures->isChecked();
    Settings::_nm = _ui->checkBox_mtNormalsMap->isChecked();
    Settings::_sm = _ui->checkBox_mtSpecularMap->isChecked();

    Settings::_exportDest = _ui->lineEdit_dir->text();
    if (_ui->radioButton_custom->isChecked())
        Settings::_mode = Export_Custom;
    else if (_ui->radioButton_pack0->isChecked())
        Settings::_mode = Export_Pack0;

    Settings::_convertTextures = _ui->checkBox_convertTextures->isChecked();
    Settings::_texFormat = _ui->comboBox_format->currentText();

    accept();
    emit optionsValidation();
}

void Options::selectColor()
{
    QColor col = QColorDialog::getColor ( QColor(Settings::_r, Settings::_g, Settings::_b), this );

    if(!col.isValid())
        return;

    Settings::_r = col.red();
    Settings::_g = col.green();
    Settings::_b = col.blue();
}

void Options::changeExport()
{
    _ui->lineEdit_dir->setEnabled(_ui->radioButton_custom->isChecked());
    _ui->button_selectDir->setEnabled(_ui->radioButton_custom->isChecked());
}

void Options::changeDebug()
{
    Settings::_debugLog = _ui->checkBox_debug->isChecked();
}

void Options::selectDir()
{
    QString file = QFileDialog::getExistingDirectory(this, Translator::findTranslation("options_export_target"), _ui->lineEdit_dir->text());
    if (file != "")
    {
        _ui->lineEdit_dir->setText(file);
        Settings::_exportDest = file;
    }
}

void Options::selectTW3TexDir()
{
    QString file = QFileDialog::getExistingDirectory(this, Translator::findTranslation("options_export_target"), _ui->lineEdit_dir->text());
    if (file != "")
    {
        _ui->lineEdit_TW3_texFolder->setText(file);
        Settings::_TW3TexPath = file;
    }
}
