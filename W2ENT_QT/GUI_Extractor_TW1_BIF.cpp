#include "GUI_Extractor_TW1_BIF.h"
#include "ui_GUI_Extractor_TW1_BIF.h"

#include <QFileDialog>

#include <iostream>

GUI_Extractor_TW1_BIF::GUI_Extractor_TW1_BIF(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_Extractor_TW1_BIF),
    _thread(nullptr),
    _extractor(nullptr)
{
    _ui->setupUi(this);

    QObject::connect(_ui->pushButton_selectKeyFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
    QObject::connect(_ui->pushButton_selectFolder, SIGNAL(clicked(bool)), this, SLOT(selectFolder()));
    QObject::connect(_ui->pushButton_extract, SIGNAL(clicked(bool)), this, SLOT(extract()));
    QObject::connect(_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_Extractor_TW1_BIF::~GUI_Extractor_TW1_BIF()
{
    delete _ui;
}

void GUI_Extractor_TW1_BIF::destroyWindow()
{
    if (_thread)
    {
        _extractor->quitThread();
        while(_thread /*&& !_thread->isFinished()*/)
            QCoreApplication::processEvents();
    }
    delete this;
}

void GUI_Extractor_TW1_BIF::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select the export folder");
    if (dir != "")
        _ui->lineEdit_exportFolder->setText(dir);
}

void GUI_Extractor_TW1_BIF::selectFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select a KEY file", QString(), "The Witcher key file (*.key)");
    if (filename != "")
        _ui->lineEdit_keyFile->setText(filename);
}

void GUI_Extractor_TW1_BIF::extract()
{
    const QString file = _ui->lineEdit_keyFile->text();
    const QString folder = _ui->lineEdit_exportFolder->text();

    QFileInfo fileInfo(file);
    if (!fileInfo.exists())
    {
        _ui->label_state->setText("State : Error, the specified KEY file doesn't exists");
        return;
    }

    _thread = new QThread();
    _extractor = new Extractor_TW1_BIF(file, folder);
    _extractor->moveToThread(_thread);

    QObject::connect(_thread, SIGNAL(started()), _extractor, SLOT(run()));
    QObject::connect(_extractor, SIGNAL(onProgress(int)), this, SLOT(extractSetProgress(int)));

    QObject::connect(_extractor, SIGNAL(error()), this, SLOT(extractFail()));
    QObject::connect(_extractor, SIGNAL(finished()), this, SLOT(extractEnd()));

    _ui->pushButton_extract->setEnabled(false);
    _ui->label_state->setText("State : Working...");
    _thread->start();
}


void GUI_Extractor_TW1_BIF::extractSetProgress(int value)
{
    _ui->progressBar->setValue(value);
}

void GUI_Extractor_TW1_BIF::killExtractThread()
{
    if (!_thread || !_extractor)
        return;

    _thread->quit();
    _thread->deleteLater();
    _extractor->deleteLater();

    _thread = nullptr;
    _extractor = nullptr;
}

void GUI_Extractor_TW1_BIF::extractEnd()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_state->setText("State : End");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}

void GUI_Extractor_TW1_BIF::extractFail()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_state->setText("State : Fatal error during the extraction.");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}
