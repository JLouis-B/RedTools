#include "GUI_Options.h"
#include "ui_GUI_Options.h"




GUI_Options::GUI_Options(QWidget *parent, QString loadedFile, QIrrlichtWidget* irr) :
    QDialog(parent), _filename(loadedFile),
    _ui(new Ui::GUI_Options), _irr(irr)
{
    _ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    _ui->doubleSpinBox_cameraRotSpeed->setValue(Settings::_camRotSpeed);
    _ui->doubleSpinBox_cameraSpeed->setValue(Settings::_camSpeed);
    _col = QColor(Settings::_r, Settings::_g, Settings::_b);

    _ui->checkBox_moveTextures->setChecked(Settings::_copyTextures);
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

    _ui->lineEdit_exportFolder->setText(Settings::_exportDest);

    setFixedSize(this->size());

    _ui->View->setTabText(0, Translator::get("options_export"));
    _ui->checkBox_moveTextures->setText(Translator::get("options_export_move_textures"));
    _ui->radioButton_custom->setText(Translator::get("options_export_to_other"));
    _ui->radioButton_pack0->setText(Translator::get("options_export_to_pack0"));
    _ui->checkBox_convertTextures->setText(Translator::get("options_export_convert_textures"));

    _ui->label_TW3_texFolder->setText(Translator::get("options_tw3_textures_folder"));
    _ui->checkBox_TW3_loadSkel->setText(Translator::get("options_tw3_skeleton"));

    _ui->View->setTabText(2, Translator::get("options_view"));
    _ui->label_camera->setText(Translator::get("options_camera"));
    _ui->label_movementSpeed->setText(Translator::get("options_camera_movement_speed"));
    _ui->label_rotSpeed->setText(Translator::get("options_camera_rot_speed"));
    _ui->checkBox_debug->setText(Translator::get("options_debug_log"));
    _ui->label_debugLog->setText(Translator::get("options_debug_log_label"));
    _ui->label_colorSelector->setText(Translator::get("options_background"));

    _ui->lineEdit_TW3_texFolder->setText(Settings::_TW3TexPath);
    _ui->checkBox_TW3_loadSkel->setChecked(Settings::_TW3LoadSkel);
    _ui->checkBox_TW3_loadBestLOD->setChecked(Settings::_TW3LoadBestLOD);

    _ui->checkBox_debug->setChecked(Settings::_debugLog);

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

    QObject::connect(_ui->pushButton_TW3_selectTexFolder, SIGNAL(clicked()), this, SLOT(selectTW3TexDir()));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_Options::~GUI_Options()
{
    delete _ui;
}

void GUI_Options::reset()
{
    Settings::_camRotSpeed = DEFAULT_CAM_ROT_SPEED;
    Settings::_camSpeed = DEFAULT_CAM_SPEED;

    _ui->doubleSpinBox_cameraRotSpeed->setValue(Settings::_camRotSpeed);
    _ui->doubleSpinBox_cameraSpeed->setValue(Settings::_camSpeed);

    Settings::_r = 0;
    Settings::_g = 0;
    Settings::_b = 0;
}

void GUI_Options::ok()
{
    Settings::_camRotSpeed = _ui->doubleSpinBox_cameraRotSpeed->value();
    Settings::_camSpeed = _ui->doubleSpinBox_cameraSpeed->value();

    Settings::_copyTextures = _ui->checkBox_moveTextures->isChecked();
    Settings::_nm = _ui->checkBox_mtNormalsMap->isChecked();
    Settings::_sm = _ui->checkBox_mtSpecularMap->isChecked();

    Settings::_exportDest = _ui->lineEdit_exportFolder->text();
    if (_ui->radioButton_custom->isChecked())
        Settings::_mode = Export_Custom;
    else if (_ui->radioButton_pack0->isChecked())
        Settings::_mode = Export_Pack0;

    Settings::_convertTextures = _ui->checkBox_convertTextures->isChecked();
    Settings::_texFormat = _ui->comboBox_format->currentText();

    Settings::_TW3TexPath = _ui->lineEdit_TW3_texFolder->text();
    Settings::_TW3LoadSkel = _ui->checkBox_TW3_loadSkel->isChecked();
    Settings::_TW3LoadBestLOD = _ui->checkBox_TW3_loadBestLOD->isChecked();

    Settings::_debugLog = _ui->checkBox_debug->isChecked();

    Settings::_r = _col.red();
    Settings::_g = _col.green();
    Settings::_b = _col.blue();

    accept();
    emit optionsValidation();
}

void GUI_Options::selectColor()
{
    QColor col = QColorDialog::getColor (_col, this);

    if(!col.isValid())
        return;

    _col = col;
}

void GUI_Options::changeExport()
{
    _ui->lineEdit_exportFolder->setEnabled(_ui->radioButton_custom->isChecked());
    _ui->button_selectDir->setEnabled(_ui->radioButton_custom->isChecked());
}

bool isASCII(QString path)
{
    for (int i = 0; i < path.size(); ++i)
    {
        if (path[i].unicode() > 127)
            return false;
    }
    return true;
}

void GUI_Options::selectDir()
{
    QString file = QFileDialog::getExistingDirectory(this, Translator::get("options_export_target"), _ui->lineEdit_exportFolder->text());
    if (file != "")
    {
        if (isASCII(file))
        {
            _ui->lineEdit_exportFolder->setText(file);
        }
        else
            QMessageBox::critical(this, "Error", "Error : Check that you don't use special characters in your path.");
    }
}

void GUI_Options::selectTW3TexDir()
{
    QString file = QFileDialog::getExistingDirectory(this, Translator::get("options_export_target"), _ui->lineEdit_TW3_texFolder->text());
    if (file != "")
    {
        if (isASCII(file))
        {
            _ui->lineEdit_TW3_texFolder->setText(file);
        }
        else
            QMessageBox::critical(this, "Error", "Error : Check that you don't use special characters in your path.");
    }
}
