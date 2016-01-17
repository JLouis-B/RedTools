#include "tw1bifextractorui.h"
#include "ui_tw1bifextractorui.h"


#include <iostream>

tw1bifextractorUI::tw1bifextractorUI(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::tw1bifextractorUI)
{
    _ui->setupUi(this);
    _thread = 0;
    _extractor = 0;

    QObject::connect(_ui->pushButton_selectFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
    QObject::connect(_ui->pushButton_selectFolder, SIGNAL(clicked(bool)), this, SLOT(selectFolder()));
    QObject::connect(_ui->pushButton_extract, SIGNAL(clicked(bool)), this, SLOT(extract()));
    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));
}

tw1bifextractorUI::~tw1bifextractorUI()
{
    delete _ui;
}

void tw1bifextractorUI::destroyWindow()
{
    if (_thread)
    {
        _extractor->quitThread();
        while(_thread /*&& !_thread->isFinished()*/)
            QCoreApplication::processEvents();
    }
    delete this;
}

void tw1bifextractorUI::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select the export folder");
    if (dir != "")
        _ui->lineEdit_exportFolder->setText(dir);
}

void tw1bifextractorUI::selectFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select a KEY file");
    if (filename != "")
        _ui->lineEdit_keyFile->setText(filename);
}

void tw1bifextractorUI::extract()
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
    _extractor = new TW1bifExtractor(file, folder);
    _extractor->moveToThread(_thread);

    QObject::connect(_thread, SIGNAL(started()), _extractor, SLOT(run()));
    QObject::connect(_extractor, SIGNAL(onProgress(int)), this, SLOT(extractSetProgress(int)));

    QObject::connect(_extractor, SIGNAL(error()), this, SLOT(extractFail()));
    QObject::connect(_extractor, SIGNAL(finished()), this, SLOT(extractEnd()));

    _ui->pushButton_extract->setEnabled(false);
    _ui->label_state->setText("State : Working...");
    _thread->start();
}


void tw1bifextractorUI::extractSetProgress(int value)
{
    _ui->progressBar->setValue(value);
}

void tw1bifextractorUI::killExtractThread()
{
    if (!_thread || !_extractor)
        return;

    _thread->quit();
    _thread->deleteLater();
    _extractor->deleteLater();

    _thread = 0;
    _extractor = 0;
}

void tw1bifextractorUI::extractEnd()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_state->setText("State : End");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}

void tw1bifextractorUI::extractFail()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_state->setText("State : Fatal error during the extraction.");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}
