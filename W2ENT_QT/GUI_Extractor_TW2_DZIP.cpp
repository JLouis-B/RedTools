#include "GUI_Extractor_TW2_DZIP.h"
#include "ui_GUI_Extractor_TW2_DZIP.h"

GUI_Extractor_TW2_DZIP::GUI_Extractor_TW2_DZIP(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_Extractor_TW2_DZIP),
    _thread(nullptr),
    _extractor(nullptr)
{
    _ui->setupUi(this);

    QObject::connect(_ui->pushButton_selectDzipFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
    QObject::connect(_ui->pushButton_selectDestFolder, SIGNAL(clicked(bool)), this, SLOT(selectFolder()));
    QObject::connect(_ui->pushButton_extract, SIGNAL(clicked(bool)), this, SLOT(extract()));
    QObject::connect(_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_Extractor_TW2_DZIP::~GUI_Extractor_TW2_DZIP()
{
    delete _ui;
}

void GUI_Extractor_TW2_DZIP::destroyWindow()
{
    if (_thread)
    {
        _extractor->quitThread();
        while(_thread)
            QCoreApplication::processEvents();
    }
    delete this;
}

void GUI_Extractor_TW2_DZIP::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select the export folder");
    if (dir != "")
        _ui->lineEdit_destFolder->setText(dir);
}

void GUI_Extractor_TW2_DZIP::selectFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select a DZIP file", "", "*.dzip");
    if (filename != "")
        _ui->lineEdit_dzipFile->setText(filename);
}

void GUI_Extractor_TW2_DZIP::extract()
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
    _extractor = new Extractor_TW2_DZIP(file, folder);
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

void GUI_Extractor_TW2_DZIP::extractSetProgress(int value)
{
    _ui->progressBar->setValue(value);
}

void GUI_Extractor_TW2_DZIP::killExtractThread()
{
    if (!_thread || !_extractor)
        return;

    _thread->quit();
    _thread->deleteLater();
    _extractor->deleteLater();

    _thread = nullptr;
    _extractor = nullptr;
}

void GUI_Extractor_TW2_DZIP::extractEnd()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_status->setText("State : End");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}

void GUI_Extractor_TW2_DZIP::extractFail()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_status->setText("State : Fatal error during the extraction.");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}
