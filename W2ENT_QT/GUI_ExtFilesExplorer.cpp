#include "GUI_ExtFilesExplorer.h"
#include "ui_GUI_ExtFilesExplorer.h"

#include "QIrrlichtWidget.h"

#include "Utils_Loaders_Irr.h"

GUI_ExtFilesExplorer::GUI_ExtFilesExplorer(QIrrlichtWidget* irrlicht, QWidget *parent) :
    QDialog(parent), _irrlicht(irrlicht),
    _ui(new Ui::GUI_ExtFilesExplorer)
{
    _ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(_ui->button_close, SIGNAL(clicked(bool)), this, SLOT(close()));
    QObject::connect(_ui->button_selectFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));

    QObject::connect(_ui->button_back, SIGNAL(clicked(bool)), this, SLOT(back()));
    QObject::connect(_ui->button_checkW2MI, SIGNAL(clicked(bool)), this, SLOT(checkW2MI()));
    QObject::connect(_ui->listWidget, SIGNAL(currentTextChanged(QString)), this, SLOT(changeSelection(QString)));
}

GUI_ExtFilesExplorer::~GUI_ExtFilesExplorer()
{
    delete _ui;
}

void GUI_ExtFilesExplorer::read(QString filename)
{
    core::array<core::stringc> strings;
    core::array<core::stringc> files;

    _ui->lineEdit->setText(filename);
    _ui->listWidget->clear();

    const io::path filenamePath = QSTRING_TO_PATH(filename);
    io::IReadFile* file = _irrlicht->getFileSystem()->createAndOpenFile(filenamePath);

    WitcherFileType fileType = getTWFileType(file);
    switch (fileType)
    {
        case WFT_UNKNOWN:
            _ui->label_fileType->setText("File type : Not a witcher file");
            return;

        case WFT_WITCHER_2:
            _ui->label_fileType->setText("File type : The Witcher 2 file");
            break;

        case WFT_WITCHER_3:
            _ui->label_fileType->setText("File type : The Witcher 3 file");
            break;
    }

    loadTWStringsAndFiles(file, strings, files, true);
    for (u32 i = 0; i < files.size(); ++i)
    {
        _ui->listWidget->addItem(QString(files[i].c_str()));
    }

    if (file)
        file->drop();
}


void GUI_ExtFilesExplorer::selectFile()
{
    QString file = QFileDialog::getOpenFileName(this, "Select the file to analyze", _ui->lineEdit->text(), "");
    if (file != "")
    {
        read(file);
    }
}


// W2MI stuff ----------------
void GUI_ExtFilesExplorer::checkW2MI()
{
    _back = _ui->lineEdit->text();
    read(Settings::_pack0 + "/" + _ui->listWidget->currentItem()->text());
    _ui->button_back->setEnabled(true);
}

void GUI_ExtFilesExplorer::changeSelection(QString newSelectedText)
{
    _ui->button_checkW2MI->setEnabled(newSelectedText.contains(".w2mi"));
}

void GUI_ExtFilesExplorer::back()
{
    read(_back);
    _ui->button_back->setEnabled(false);
}
