#include "TW2_DZIP_ExtractorUI.h"
#include "ui_TW2_DZIP_ExtractorUI.h"

TW2_DZIP_ExtractorUI::TW2_DZIP_ExtractorUI(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::TW2_DZIP_ExtractorUI),
    _extractor(nullptr),
    _thread(nullptr)
{
    _ui->setupUi(this);

    QObject::connect(_ui->pushButton_select_dzipFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
    QObject::connect(_ui->pushButton_selectDestFolder, SIGNAL(clicked(bool)), this, SLOT(selectFolder()));
    QObject::connect(_ui->pushButton_extract, SIGNAL(clicked(bool)), this, SLOT(extract()));
    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));
}

TW2_DZIP_ExtractorUI::~TW2_DZIP_ExtractorUI()
{
    delete _ui;
}

void TW2_DZIP_ExtractorUI::destroyWindow()
{
    if (_thread)
    {
        _extractor->quitThread();
        while(_thread)
            QCoreApplication::processEvents();
    }
    delete this;
}

void TW2_DZIP_ExtractorUI::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select the export folder");
    if (dir != "")
        _ui->lineEdit_destFolder->setText(dir);
}

void TW2_DZIP_ExtractorUI::selectFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select a DZIP file");
    if (filename != "")
        _ui->lineEdit_dzipFile->setText(filename);
}

void TW2_DZIP_ExtractorUI::extract()
{
    const QString file = _ui->lineEdit_dzipFile->text();
    const QString folder = _ui->lineEdit_destFolder->text();

    QFileInfo fileInfo(file);
    if (!fileInfo.exists())
    {
        _ui->label_status->setText("State : Error, the specified DZIP file doesn't exists");
        return;
    }

    _thread = new QThread();
    _extractor = new TW2_DZIP_Extractor(file, folder);
    _extractor->moveToThread(_thread);

    QObject::connect(_thread, SIGNAL(started()), _extractor, SLOT(run()));
    QObject::connect(_extractor, SIGNAL(onProgress(int)), this, SLOT(extractSetProgress(int)));

    QObject::connect(_extractor, SIGNAL(error()), this, SLOT(extractFail()));
    QObject::connect(_extractor, SIGNAL(finished()), this, SLOT(extractEnd()));

    _ui->pushButton_extract->setEnabled(false);
    _ui->label_status->setText("State : Working...");
    extractSetProgress(0);
    _thread->start();
}

void TW2_DZIP_ExtractorUI::extractSetProgress(int value)
{
    _ui->progressBar->setValue(value);
}

void TW2_DZIP_ExtractorUI::killExtractThread()
{
    if (!_thread || !_extractor)
        return;

    _thread->quit();
    _thread->deleteLater();
    _extractor->deleteLater();

    _thread = nullptr;
    _extractor = nullptr;
}

void TW2_DZIP_ExtractorUI::extractEnd()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_status->setText("State : End");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}

void TW2_DZIP_ExtractorUI::extractFail()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_status->setText("State : Fatal error during the extraction.");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}
