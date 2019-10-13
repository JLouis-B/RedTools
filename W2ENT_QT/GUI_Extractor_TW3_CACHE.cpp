#include "GUI_Extractor_TW3_CACHE.h"
#include "ui_GUI_Extractor_TW3_CACHE.h"

#include <QListView>
#include <QTreeView>
#include <QFileDialog>

GUI_Extractor_TW3_CACHE::GUI_Extractor_TW3_CACHE(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_Extractor_TW3_CACHE),
    _thread(nullptr),
    _extractor(nullptr)
{
    _ui->setupUi(this);

    QObject::connect(_ui->pushButton_selectDestFolder, SIGNAL(clicked(bool)), this, SLOT(selectFolder()));
    QObject::connect(_ui->pushButton_extract, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
    QObject::connect(_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_Extractor_TW3_CACHE::~GUI_Extractor_TW3_CACHE()
{
    delete _ui;
}

void GUI_Extractor_TW3_CACHE::destroyWindow()
{
    _filesQueue.clear();
    if (_thread)
    {
        _extractor->quitThread();
        while(_thread)
            QCoreApplication::processEvents();
    }
    deleteLater();
}

void GUI_Extractor_TW3_CACHE::selectFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select the export folder");
    if (dir != "")
        _ui->lineEdit_destFolder->setText(dir);
}

void GUI_Extractor_TW3_CACHE::selectFile()
{
    QFileDialog* dialog = new QFileDialog(this);
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::DontUseNativeDialog, true);

    // Try to select multiple files and directories at the same time in QFileDialog
    QListView *l = dialog->findChild<QListView*>("listView");
    if (l)
        l->setSelectionMode(QAbstractItemView::MultiSelection);

    QTreeView *t = dialog->findChild<QTreeView*>();
    if (t)
        t->setSelectionMode(QAbstractItemView::MultiSelection);


    dialog->exec();
    QStringList filenames = dialog->selectedFiles();
    delete dialog;

    foreach (QString filename, filenames)
    {
        getFiles(filename);
    }

    _nbFiles = _filesQueue.size();
    _nbFilesProcessed = 1;

    nextFile();
    //QString filename = QFileDialog::getOpenFileName(this, "Select a BUNDLE file", "", "*.bundle");
    //if (filename != "")
    //    _ui->lineEdit_dzipFile->setText(filename);
}

void GUI_Extractor_TW3_CACHE::getFiles(QString file)
{
    QFileInfo info(file);
    if (!info.isDir())
        return;

    // search the files
    QDir dirFiles(file);
    dirFiles.setFilter(QDir::NoDotAndDotDot | QDir::Files);
    QStringList filter = QStringList("*.bundle");
    dirFiles.setNameFilters(filter);

    foreach(QFileInfo fileInfo, dirFiles.entryInfoList())
    {
        _filesQueue.push_back(fileInfo.absoluteFilePath());
    }

    // search in the subfolders
    dirFiles.setNameFilters(QStringList());
    dirFiles.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
    foreach (QFileInfo fileInfo, dirFiles.entryInfoList())
    {
        getFiles(fileInfo.absoluteFilePath());
    }
}

void GUI_Extractor_TW3_CACHE::extract(QString file)
{
    const QString folder = _ui->lineEdit_destFolder->text();

    QFileInfo fileInfo(file);
    if (!fileInfo.exists())
    {
        _ui->label_status->setText("State : Error, the specified BUNDLE file doesn't exists");
        return;
    }

    _thread = new QThread();
    _extractor = new Extractor_TW3_CACHE(file, folder);
    _extractor->moveToThread(_thread);

    QObject::connect(_thread, SIGNAL(started()), _extractor, SLOT(run()));
    QObject::connect(_extractor, SIGNAL(onProgress(int)), this, SLOT(extractSetProgress(int)));

    QObject::connect(_extractor, SIGNAL(error()), this, SLOT(extractFail()));
    QObject::connect(_extractor, SIGNAL(finished()), this, SLOT(extractEnd()));

    _ui->pushButton_extract->setEnabled(false);
    _ui->label_status->setText("State : Working on " + file + "... (" + QString::number(_nbFilesProcessed) + "/" + QString::number(_nbFiles) + ")");
    extractSetProgress(0);
    _thread->start();
}

void GUI_Extractor_TW3_CACHE::extractSetProgress(int value)
{
    _ui->progressBar->setValue(value);
}

void GUI_Extractor_TW3_CACHE::killExtractThread()
{
    if (!_thread || !_extractor)
        return;

    _thread->quit();
    _thread->deleteLater();
    _extractor->deleteLater();

    _thread = nullptr;
    _extractor = nullptr;
}

bool GUI_Extractor_TW3_CACHE::nextFile()
{
    if (!_filesQueue.empty())
    {
        QString currentFilename = _filesQueue.dequeue();
        extract(currentFilename);
        return true;
    }
    else
    {
        _nbFiles = 0;
        _nbFilesProcessed = 0;
        return false;
    }
}

void GUI_Extractor_TW3_CACHE::extractEnd()
{
    killExtractThread();
    _nbFilesProcessed++;

    if (!nextFile())
    {
        _ui->progressBar->setValue(_ui->progressBar->maximum());
        _ui->label_status->setText("State : End");
        _ui->pushButton_extract->setEnabled(true);
    }
}

void GUI_Extractor_TW3_CACHE::extractFail()
{
    killExtractThread();
    _nbFilesProcessed++;

    if (!nextFile())
    {
        _ui->progressBar->setValue(_ui->progressBar->maximum());
        _ui->label_status->setText("State : Fatal error during the extraction.");
        _ui->pushButton_extract->setEnabled(true);
    }
}
