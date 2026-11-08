#include "GUI_About.h"
#include "ui_GUI_About.h"

#include "Settings.h"

GUI_About::GUI_About(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_About)
{
    _ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    _ui->label_version->setText(QString("The Witcher Converter ") + Settings::getAppVersion());
}

GUI_About::~GUI_About()
{
    delete _ui;
}
