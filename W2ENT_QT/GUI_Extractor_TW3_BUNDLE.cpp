#include "GUI_Extractor_TW3_BUNDLE.h"
#include "ui_GUI_Extractor_TW3_BUNDLE.h"

GUI_Extractor_TW3_BUNDLE::GUI_Extractor_TW3_BUNDLE(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_Extractor_TW3_BUNDLE),
    _extractor(nullptr),
    _thread(nullptr)
{
    _ui->setupUi(this);

    QObject::connect(_ui->pushButton_select_dzipFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
    QObject::connect(_ui->pushButton_selectDestFolder, SIGNAL(clicked(bool)), this, SLOT(selectFolder()));
    QObject::connect(_ui->pushButton_extract, SIGNAL(clicked(bool)), this, SLOT(extract()));
    QObject::connect(_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_Extractor_TW3_BUNDLE::~GUI_Extractor_TW3_BUNDLE()
{
    delete _ui;
}

void GUI_Extractor_TW3_BUNDLE::destroyWindow()
{
    if (_thread)
    {
        _extractor->quitThread();
        while(_thread)
            QCoreApplication::processEvents();
    }
    delete this;
}

void GUI_Extractor_TW3_BUNDLE::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select the export folder");
    if (dir != "")
        _ui->lineEdit_destFolder->setText(dir);
}

void GUI_Extractor_TW3_BUNDLE::selectFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select a BUNDLE file", "", "*.bundle");
    if (filename != "")
        _ui->lineEdit_dzipFile->setText(filename);
}

void GUI_Extractor_TW3_BUNDLE::extract()
{
    const QString file = _ui->lineEdit_dzipFile->text();
    const QString folder = _ui->lineEdit_destFolder->text();

    QFileInfo fileInfo(file);
    if (!fileInfo.exists())
    {
        _ui->label_status->setText("State : Error, the specified BUNDLE file doesn't exists");
        return;
    }

    _thread = new QThread();
    _extractor = new Extractor_TW3_BUNDLE(file, folder);
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

void GUI_Extractor_TW3_BUNDLE::extractSetProgress(int value)
{
    _ui->progressBar->setValue(value);
}

void GUI_Extractor_TW3_BUNDLE::killExtractThread()
{
    if (!_thread || !_extractor)
        return;

    _thread->quit();
    _thread->deleteLater();
    _extractor->deleteLater();

    _thread = nullptr;
    _extractor = nullptr;
}

void GUI_Extractor_TW3_BUNDLE::extractEnd()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_status->setText("State : End");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}

void GUI_Extractor_TW3_BUNDLE::extractFail()
{
    _ui->progressBar->setValue(_ui->progressBar->maximum());
    _ui->label_status->setText("State : Fatal error during the extraction.");
    _ui->pushButton_extract->setEnabled(true);

    killExtractThread();
}
