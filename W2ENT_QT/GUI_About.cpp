#include "GUI_About.h"
#include "ui_GUI_About.h"

#include "settings.h"

GUI_About::GUI_About(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_About)
{
    _ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    _ui->label_version->setText(QString("The Witcher Converter ") + Settings::getAppVersion());
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_About::~GUI_About()
{
    delete _ui;
}
