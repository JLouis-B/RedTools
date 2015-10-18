#include "options.h"
#include "ui_options.h"


double OptionsData::_camRotSpeed = DEFAULT_CAM_ROT_SPEED;
double OptionsData::_camSpeed = DEFAULT_CAM_SPEED;

int OptionsData::_r = 0;
int OptionsData::_g = 0;
int OptionsData::_b = 0;

Export_Mode OptionsData::_mode = Export_Pack0;
QString OptionsData::_exportDest = "";

bool OptionsData::_moveTexture = true;
bool OptionsData::_nm = false;
bool OptionsData::_sm = false;

bool OptionsData::_debugLog = false;

bool OptionsData::_convertTextures = false;
QString OptionsData::_texFormat = ".jpg";

QString OptionsData::_pack0 = "";


QString OptionsData::getExportFolder()
{
    if (OptionsData::_mode == Export_Pack0)
        return OptionsData::_pack0;
    else
        return OptionsData::_exportDest;
}



Options::Options(QWidget *parent, QString language, QString loadedFile, QIrrlichtWidget* irr) :
    QDialog(parent), _language(language), _filename(loadedFile),
    _ui(new Ui::Options), _irr(irr)
{
    _ui->setupUi(this);
    _ui->doubleSpinBox_cameraRotSpeed->setValue(OptionsData::_camRotSpeed);
    _ui->doubleSpinBox_cameraSpeed->setValue(OptionsData::_camSpeed);

    _ui->checkBox_moveTextures->setChecked(OptionsData::_moveTexture);
    _ui->checkBox_mtNormalsMap->setChecked(OptionsData::_nm);
    _ui->checkBox_mtSpecularMap->setChecked(OptionsData::_sm);


    _ui->checkBox_convertTextures->setChecked(OptionsData::_convertTextures);
    if (_ui->comboBox_format->findText(OptionsData::_texFormat) != -1)
        _ui->comboBox_format->setCurrentIndex(_ui->comboBox_format->findText(OptionsData::_texFormat));
    _ui->comboBox_format->setEnabled(_ui->checkBox_convertTextures->isChecked());

    if(OptionsData::_mode == Export_Custom)
        _ui->radioButton_custom->setChecked(true);
    else
        _ui->radioButton_pack0->setChecked(true);

    _ui->lineEdit_dir->setText(OptionsData::_exportDest);

    setFixedSize(this->size());

    _ui->checkBox_moveTextures->setText(Translator::findTranslation("options_export_move_textures", language));
    _ui->radioButton_custom->setText(Translator::findTranslation("options_export_to_other", language));
    _ui->radioButton_pack0->setText(Translator::findTranslation("options_export_to_pack0", language));
    _ui->label_colorSelector->setText(Translator::findTranslation("options_background", language));
    _ui->label_movementSpeed->setText(Translator::findTranslation("options_camera_movement_speed", language));
    _ui->label_rotSpeed->setText(Translator::findTranslation("options_camera_rot_speed", language));
    _ui->View->setTabText(1, Translator::findTranslation("options_view", language));
    _ui->View->setTabText(0, Translator::findTranslation("options_export", language));
    _ui->checkBox_debug->setText(Translator::findTranslation("options_debug_log", language));
    _ui->label_debugLog->setText(Translator::findTranslation("options_debug_log_label", language));

    _ui->checkBox_debug->setChecked(OptionsData::_debugLog);

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



}

Options::~Options()
{
    delete _ui;
}

void Options::reset()
{
    OptionsData::_camRotSpeed = DEFAULT_CAM_ROT_SPEED;
    OptionsData::_camSpeed = DEFAULT_CAM_SPEED;

    _ui->doubleSpinBox_cameraRotSpeed->setValue(OptionsData::_camRotSpeed);
    _ui->doubleSpinBox_cameraSpeed->setValue(OptionsData::_camSpeed);

    OptionsData::_r = 0;
    OptionsData::_g = 0;
    OptionsData::_b = 0;
}

void Options::ok()
{
    OptionsData::_camRotSpeed = _ui->doubleSpinBox_cameraRotSpeed->value();
    OptionsData::_camSpeed = _ui->doubleSpinBox_cameraSpeed->value();

    OptionsData::_moveTexture = _ui->checkBox_moveTextures->isChecked();
    OptionsData::_nm = _ui->checkBox_mtNormalsMap->isChecked();
    OptionsData::_sm = _ui->checkBox_mtSpecularMap->isChecked();

    OptionsData::_exportDest = _ui->lineEdit_dir->text();
    if (_ui->radioButton_custom->isChecked())
        OptionsData::_mode = Export_Custom;
    else if (_ui->radioButton_pack0->isChecked())
        OptionsData::_mode = Export_Pack0;

    OptionsData::_convertTextures = _ui->checkBox_convertTextures->isChecked();
    OptionsData::_texFormat = _ui->comboBox_format->currentText();

    accept();
    emit optionsValidation();
}

void Options::selectColor()
{
    QColor col = QColorDialog::getColor ( QColor(OptionsData::_r, OptionsData::_g, OptionsData::_b), this );

    if(!col.isValid())
        return;

    OptionsData::_r = col.red();
    OptionsData::_g = col.green();
    OptionsData::_b = col.blue();
}

void Options::changeExport()
{
    _ui->lineEdit_dir->setEnabled(_ui->radioButton_custom->isChecked());
    _ui->button_selectDir->setEnabled(_ui->radioButton_custom->isChecked());
}

void Options::changeDebug()
{
    OptionsData::_debugLog = _ui->checkBox_debug->isChecked();
}

void Options::selectDir()
{
    QString fichier = QFileDialog::getExistingDirectory(this, Translator::findTranslation("options_export_target", _language), _ui->lineEdit_dir->text());
    if (fichier != "")
    {
        _ui->lineEdit_dir->setText(fichier);
        OptionsData::_exportDest = fichier;
    }
}
