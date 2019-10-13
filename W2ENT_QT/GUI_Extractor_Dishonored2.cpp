#include "GUI_Extractor_Dishonored2.h"
#include "ui_GUI_Extractor_Dishonored2.h"

#include <QFileDialog>

#include <iostream>

GUI_Extractor_Dishonored2::GUI_Extractor_Dishonored2(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_Extractor_Dishonored2),
    _thread(nullptr),
    _extractor(nullptr)
{
    _ui->setupUi(this);

    QObject::connect(_ui->pushButton_selectIndexFile, SIGNAL(clicked(bool)), this, SLOT(selectIndexFile()));
    QObject::connect(_ui->pushButton_selectResourcesFile, SIGNAL(clicked(bool)), this, SLOT(selectResourcesFile()));
    QObject::connect(_ui->pushButton_selectFolder, SIGNAL(clicked(bool)), this, SLOT(selectFolder()));
    QObject::connect(_ui->pushButton_extract, SIGNAL(clicked(bool)), this, SLOT(extract()));
    QObject::connect(_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_Extractor_Dishonored2::~GUI_Extractor_Dishonored2()
{
    delete _ui;
}

void GUI_Extractor_Dishonored2::destroyWindow()
{
    if (_thread)
    {
        _extractor->quitThread();
        while(_thread /*&& !_thread->isFinished()*/)
            QCoreApplication::processEvents();
    }
    deleteLater();
}

void GUI_Extractor_Dishonored2::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select the export folder");
    if (dir != "")
        _ui->lineEdit_exportFolder->setText(dir);
}

void GUI_Extractor_Dishonored2::selectIndexFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select a .index file", QString(), "INDEX file (*.index)");
    if (filename != "")
        _ui->lineEdit_indexFile->setText(filename);
}

void GUI_Extractor_Dishonored2::selectResourcesFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select a .resources file", QString(), "RESOURCES file (*.resources)");
    if (filename != "")
        _ui->lineEdit_resourcesFile->setText(filename);
}

void GUI_Extractor_Dishonored2::extract()
{
    const QString indexFile = _ui->lineEdit_indexFile->text();
    const QString resourcesFile = _ui->lineEdit_resourcesFile->text();
    const QString folder = _ui->lineEdit_exportFolder->text();

    QFileInfo indexFileInfo(indexFile);
    if (!indexFileInfo.exists())
    {
        _ui->label_state->setText("State : Error, the specified index file doesn't exists");
        return;
    }
    QFileInfo resourcesFileInfo(indexFile);
    if (!resourcesFileInfo.exists())
    {
        _ui->label_state->setText("State : Error, the specified resources file doesn't exists");
        return;
    }

    _thread = new QThread();
    _extractor = new Extractor_Dishonored2(indexFile, resourcesFile, folder);
    _extractor->moveToThread(_thread);

    QObject::connect(_thread, SIGNAL(started()), _extractor, SLOT(run()));
    QObject::connect(_extractor, SIGNAL(onProgress(int)), this, SLOT(extractSetProgress(int)));

    QObject::connect(_extractor, SIGNAL(error()), this, SLOT(extractFail()));
    QObject::connect(_extractor, SIGNAL(finished()), this, SLOT(extractEnd()));

    _ui->pushButton_extract->setEnabled(false);
    _ui->label_state->setText("State : Working...");
    _thread->start();
}


void GUI_Extractor_Dishonored2::extractSetProgress(int value)
{
    _ui->progressBar->setValue(value);
}

void GUI_Extractor_Dishonored2::killExtractThread()
{
    if (!_thread || !_extractor)
        return;

    _thread->quit();
    _thread->deleteLater();
    _extractor->deleteLater();

    _thread = nullptr;
    _extractor = nullptr;
}

void GUI_Extractor_Dishonored2::extractEnd()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_state->setText("State : End");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}

void GUI_Extractor_Dishonored2::extractFail()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_state->setText("State : Fatal error during the extraction.");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}
