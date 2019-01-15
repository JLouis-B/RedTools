#include "GUI_Extractor_TheCouncil.h"
#include "ui_GUI_Extractor_TheCouncil.h"

GUI_Extractor_TheCouncil::GUI_Extractor_TheCouncil(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_Extractor_TheCouncil),
    _thread(nullptr),
    _extractor(nullptr)
{
    _ui->setupUi(this);

    QObject::connect(_ui->pushButton_selectCPKFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
    QObject::connect(_ui->pushButton_selectFolder, SIGNAL(clicked(bool)), this, SLOT(selectFolder()));
    QObject::connect(_ui->pushButton_extract, SIGNAL(clicked(bool)), this, SLOT(extract()));
    QObject::connect(_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_Extractor_TheCouncil::~GUI_Extractor_TheCouncil()
{
    delete _ui;
}

void GUI_Extractor_TheCouncil::destroyWindow()
{
    if (_thread)
    {
        _extractor->quitThread();
        while(_thread /*&& !_thread->isFinished()*/)
            QCoreApplication::processEvents();
    }
    delete this;
}

void GUI_Extractor_TheCouncil::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select the export folder");
    if (dir != "")
        _ui->lineEdit_exportFolder->setText(dir);
}

void GUI_Extractor_TheCouncil::selectFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select a CPK file", QString(), "The Council CPK file (*.cpk)");
    if (filename != "")
        _ui->lineEdit_CPKFile->setText(filename);
}

void GUI_Extractor_TheCouncil::extract()
{
    const QString file = _ui->lineEdit_CPKFile->text();
    const QString folder = _ui->lineEdit_exportFolder->text();

    QFileInfo fileInfo(file);
    if (!fileInfo.exists())
    {
        _ui->label_state->setText("State : Error, the specified CPK file doesn't exists");
        return;
    }

    _thread = new QThread();
    _extractor = new Extractor_TheCouncil(file, folder);
    _extractor->moveToThread(_thread);

    QObject::connect(_thread, SIGNAL(started()), _extractor, SLOT(run()));
    QObject::connect(_extractor, SIGNAL(onProgress(int)), this, SLOT(extractSetProgress(int)));

    QObject::connect(_extractor, SIGNAL(error()), this, SLOT(extractFail()));
    QObject::connect(_extractor, SIGNAL(finished()), this, SLOT(extractEnd()));

    _ui->pushButton_extract->setEnabled(false);
    _ui->label_state->setText("State : Working...");
    _thread->start();
}


void GUI_Extractor_TheCouncil::extractSetProgress(int value)
{
    _ui->progressBar->setValue(value);
}

void GUI_Extractor_TheCouncil::killExtractThread()
{
    if (!_thread || !_extractor)
        return;

    _thread->quit();
    _thread->deleteLater();
    _extractor->deleteLater();

    _thread = nullptr;
    _extractor = nullptr;
}

void GUI_Extractor_TheCouncil::extractEnd()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_state->setText("State : End");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}

void GUI_Extractor_TheCouncil::extractFail()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_state->setText("State : Fatal error during the extraction.");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}
