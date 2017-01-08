#include "MaterialsExplorer.h"
#include "ui_MaterialsExplorer.h"
#include "extfiles.h"

MaterialsExplorer::MaterialsExplorer(QWidget *parent, QIrrlichtWidget* irrlicht) :
    QDialog(parent), _irrlicht(irrlicht),
    _ui(new Ui::MaterialsExplorer)
{
    _ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(_ui->button_selectFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
}

MaterialsExplorer::~MaterialsExplorer()
{
    delete _ui;
}

void MaterialsExplorer::loadTW3Materials(QString file)
{

}

void MaterialsExplorer::read(QString filename)
{
    _ui->lineEdit->setText(filename);
    _ui->listWidgetMaterials->clear();

    const io::path filenamePath = QSTRING_TO_PATH(filename);
    io::IReadFile* file = _irrlicht->getFileSystem()->createAndOpenFile(filenamePath);

    const WitcherFileType fileType = checkTWFileFormatVersion(file);

    switch (fileType)
    {
        case WFT_NOT_WITCHER:
            _ui->label_fileType->setText("File type : Not a witcher file");
            return;

        case WFT_WITCHER_2:
            _ui->label_fileType->setText("File type : Not a witcher file");
            return;
            break;

        case WFT_WITCHER_3:
            loadTW3Materials(filename);
            _ui->label_fileType->setText("File type : The Witcher 3 file");
            break;
    }
}


void MaterialsExplorer::selectFile()
{
    QString file = QFileDialog::getOpenFileName(this, "Select the file to analyze", _ui->lineEdit->text(), "");
    if (file != "")
    {
        read(file);
    }
}
