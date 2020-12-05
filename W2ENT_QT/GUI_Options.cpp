#include "GUI_Options.h"
#include "ui_GUI_Options.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>

#include "Settings.h"
#include "Translator.h"
#include "UIThemeManager.h"


GUI_Options::GUI_Options(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_Options)
{
    _ui->setupUi(this);
    translate();
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(this->size());

    _ui->doubleSpinBox_view_cameraRotSpeed->setValue(Settings::_cameraRotationSpeed);
    _ui->doubleSpinBox_view_cameraSpeed->setValue(Settings::_cameraSpeed);

    _originalBackgroundColor = Settings::_backgroundColor;
    updateBackgroundColorButtonColor();

    _ui->checkBox_export_copyTextures->setChecked(Settings::_copyTexturesEnabled);
    _ui->checkBox_export_copyTexturesSlot1->setChecked(Settings::_copyTexturesSlot1);
    _ui->checkBox_export_copyTexturesSlot2->setChecked(Settings::_copyTexturesSlot2);

    _ui->checkBox_export_convertTextures->setChecked(Settings::_convertTexturesEnabled);
    if (_ui->comboBox_export_texturesFormat->findText(Settings::_convertTexturesFormat) != -1)
        _ui->comboBox_export_texturesFormat->setCurrentIndex(_ui->comboBox_export_texturesFormat->findText(Settings::_convertTexturesFormat));
    _ui->comboBox_export_texturesFormat->setEnabled(_ui->checkBox_export_convertTextures->isChecked());

    if(Settings::_mode == Export_Custom)
        _ui->radioButton_export_exportCustomDir->setChecked(true);
    else
        _ui->radioButton_export_exportBaseDir->setChecked(true);
    changeExportMode();

    _ui->lineEdit_export_exportDir->setText(Settings::_exportDest);

    _ui->checkBox_TW1_loadStaticMeshes->setChecked(Settings::_TW1LoadStaticMesh);
    _ui->checkBox_TW1_loadSkinnedMeshes->setChecked(Settings::_TW1LoadSkinnedMesh);
    _ui->checkBox_TW1_loadPaintedMeshes->setChecked(Settings::_TW1LoadPaintedMesh);

    _ui->checkBox_TW2_loadBestLOD->setChecked(Settings::_TW2LoadBestLODEnabled);

    _ui->lineEdit_TW3_texFolder->setText(Settings::_TW3TexPath);
    _ui->checkBox_TW3_loadSkel->setChecked(Settings::_TW3LoadSkeletonEnabled);
    _ui->checkBox_TW3_loadBestLOD->setChecked(Settings::_TW3LoadBestLODEnabled);

    _ui->checkBox_debug_log->setChecked(Settings::_debugLog);


    QObject::connect(_ui->buttonBox, SIGNAL(accepted()), this, SLOT(ok()));
    QObject::connect(_ui->buttonBox, SIGNAL(rejected()), this, SLOT(cancel()));

    QObject::connect(_ui->button_view_backgroundColorSelector, SIGNAL(clicked()), this, SLOT(selectBackgroundColor()));
    QObject::connect(_ui->pushButton_view_reset, SIGNAL(clicked()), this, SLOT(resetViewPanel()));

    QObject::connect(_ui->button_export_selectExportDir, SIGNAL(clicked()), this, SLOT(selectExportDir()));
    QObject::connect(_ui->radioButton_export_exportCustomDir, SIGNAL(clicked()), this, SLOT(changeExportMode()));
    QObject::connect(_ui->radioButton_export_exportBaseDir, SIGNAL(clicked()), this, SLOT(changeExportMode()));

    QObject::connect(_ui->checkBox_export_copyTextures, SIGNAL(clicked(bool)), _ui->checkBox_export_copyTexturesSlot1, SLOT(setEnabled(bool)));
    QObject::connect(_ui->checkBox_export_copyTextures, SIGNAL(clicked(bool)), _ui->checkBox_export_copyTexturesSlot2, SLOT(setEnabled(bool)));
    _ui->checkBox_export_copyTexturesSlot1->setEnabled(Settings::_copyTexturesEnabled);
    _ui->checkBox_export_copyTexturesSlot2->setEnabled(Settings::_copyTexturesEnabled);

    QObject::connect(_ui->checkBox_export_convertTextures, SIGNAL(clicked(bool)), _ui->comboBox_export_texturesFormat, SLOT(setEnabled(bool)));

    QObject::connect(_ui->pushButton_TW3_selectTexFolder, SIGNAL(clicked()), this, SLOT(selectTW3TexDir()));


    _originalTheme = Settings::_theme;
    QList<QString> themes = UIThemeManager::GetAvailableThemes();
    for (int i = 0; i < themes.size(); ++i)
    {
        _ui->comboBox_theme->addItem(themes[i]);
    }
    _ui->comboBox_theme->setCurrentText(Settings::_theme);
    QObject::connect(_ui->comboBox_theme, SIGNAL(currentTextChanged(QString)), this, SLOT(changeTheme(QString)));

}

GUI_Options::~GUI_Options()
{
    delete _ui;
}

void GUI_Options::ok()
{
    Settings::_theme = _ui->comboBox_theme->currentText();

    Settings::_cameraRotationSpeed = _ui->doubleSpinBox_view_cameraRotSpeed->value();
    Settings::_cameraSpeed = _ui->doubleSpinBox_view_cameraSpeed->value();

    Settings::_copyTexturesEnabled = _ui->checkBox_export_copyTextures->isChecked();
    Settings::_copyTexturesSlot1 = _ui->checkBox_export_copyTexturesSlot1->isChecked();
    Settings::_copyTexturesSlot2 = _ui->checkBox_export_copyTexturesSlot2->isChecked();

    Settings::_exportDest = _ui->lineEdit_export_exportDir->text();
    if (_ui->radioButton_export_exportCustomDir->isChecked())
        Settings::_mode = Export_Custom;
    else if (_ui->radioButton_export_exportBaseDir->isChecked())
        Settings::_mode = Export_BaseDir;

    Settings::_convertTexturesEnabled = _ui->checkBox_export_convertTextures->isChecked();
    Settings::_convertTexturesFormat = _ui->comboBox_export_texturesFormat->currentText();

    Settings::_TW1LoadStaticMesh = _ui->checkBox_TW1_loadStaticMeshes->isChecked();
    Settings::_TW1LoadSkinnedMesh = _ui->checkBox_TW1_loadSkinnedMeshes->isChecked();
    Settings::_TW1LoadPaintedMesh = _ui->checkBox_TW1_loadPaintedMeshes->isChecked();

    Settings::_TW2LoadBestLODEnabled = _ui->checkBox_TW2_loadBestLOD->isChecked();

    Settings::_TW3TexPath = _ui->lineEdit_TW3_texFolder->text();
    Settings::_TW3LoadSkeletonEnabled = _ui->checkBox_TW3_loadSkel->isChecked();
    Settings::_TW3LoadBestLODEnabled = _ui->checkBox_TW3_loadBestLOD->isChecked();

    Settings::_debugLog = _ui->checkBox_debug_log->isChecked();

    accept();
    emit optionsValidation();
}

void GUI_Options::cancel()
{
    Settings::_backgroundColor = _originalBackgroundColor;
    UIThemeManager::SetTheme(_originalTheme);
    reject();
}

void GUI_Options::resetViewPanel()
{
    _ui->doubleSpinBox_view_cameraRotSpeed->setValue(Settings::_cameraRotationSpeed);
    _ui->doubleSpinBox_view_cameraSpeed->setValue(Settings::_cameraSpeed);

    Settings::_backgroundColor = QColor(0, 0, 0);
    updateBackgroundColorButtonColor();
}

void GUI_Options::updateBackgroundColorButtonColor()
{
    QPalette pal = _ui->button_view_backgroundColorSelector->palette();
    pal.setColor(QPalette::Button, Settings::_backgroundColor);
    _ui->button_view_backgroundColorSelector->setPalette(pal);
    _ui->button_view_backgroundColorSelector->update();
}

void GUI_Options::changeTheme(QString newThemeName)
{
    UIThemeManager::SetTheme(newThemeName);
}

void GUI_Options::selectBackgroundColor()
{
    QColor color = QColorDialog::getColor(Settings::_backgroundColor, this, "Select the background color");
    if(color.isValid())
    {
        Settings::_backgroundColor = color;
        updateBackgroundColorButtonColor();
    }
}

void GUI_Options::changeExportMode()
{
    _ui->lineEdit_export_exportDir->setEnabled(_ui->radioButton_export_exportCustomDir->isChecked());
    _ui->button_export_selectExportDir->setEnabled(_ui->radioButton_export_exportCustomDir->isChecked());
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

void GUI_Options::selectExportDir()
{
    QString file = QFileDialog::getExistingDirectory(this, Translator::get("options_export_target"), _ui->lineEdit_export_exportDir->text());
    if (file != "")
    {
        if (isASCII(file))
        {
            _ui->lineEdit_export_exportDir->setText(file);
        }
        else
            QMessageBox::critical(this, "Error", "Error : Check that you don't use special characters in your path.");
    }
}

void GUI_Options::selectTW3TexDir()
{
    QString file = QFileDialog::getExistingDirectory(this, "Select you TW3 textures folder", _ui->lineEdit_TW3_texFolder->text());
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

void GUI_Options::translate()
{
    _ui->radioButton_export_exportCustomDir->setText(Translator::get("options_export_to_other"));
    _ui->radioButton_export_exportBaseDir->setText(Translator::get("options_export_to_pack0"));

    _ui->label_TW3_texFolder->setText(Translator::get("options_tw3_textures_folder"));
    _ui->checkBox_TW3_loadSkel->setText(Translator::get("options_tw3_skeleton"));

    _ui->checkBox_debug_log->setText(Translator::get("options_debug_log"));
    _ui->label_debug_log->setText(Translator::get("options_debug_log_label"));
}
