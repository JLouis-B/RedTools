#include "search.h"
#include "ui_search.h"

#include <iostream>


// UI -------------------------------
Search::Search(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::Search), _thread(0), _searchEngine(0)
{
    _ui->setupUi(this);

    resetExtensionsFilter();
    translate();

    QObject::connect(_ui->pushButton_search, SIGNAL(clicked()), this, SLOT(search()));
    QObject::connect(_ui->pushButton_load, SIGNAL(clicked()), this, SLOT(load()));
    QObject::connect(_ui->pushButton_reset, SIGNAL(clicked()), this, SLOT(resetExtensionsFilter()));
    QObject::connect(_ui->listWidget_results, SIGNAL(currentRowChanged(int)), this, SLOT(enableButton()));

    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));  
}

Search::~Search()
{
    delete _ui;
}

void Search::destroyWindow()
{
    if (_thread)
    {
        _searchEngine->quitThread();
        while (_thread)
            QCoreApplication::processEvents();
    }

    delete this;
}

void Search::translate()
{
    _ui->label_name->setText(Translator::get("search_name") + " :");
    _ui->label_result->setText(Translator::get("search_result") + " :");
    _ui->label_progression->setText(Translator::get("search_progress") + " :");
    _ui->label_extensions->setText(Translator::get("search_extensions_filter"));
    _ui->checkBox_folder->setText(Translator::get("search_check_folder"));
    _ui->pushButton_search->setText(Translator::get("search_button"));
    _ui->pushButton_load->setText(Translator::get("search_load"));
}

void Search::search()
{
    QString name = _ui->lineEdit_name->text();
    QStringList keywords = name.split(" ", QString::SkipEmptyParts);
    QStringList extensions = _ui->lineEdit_extensionsFilter->text().split(" ", QString::SkipEmptyParts);
    for (QStringList::iterator it = extensions.begin(); it != extensions.end(); ++it)
    {
        *it = "*." + (*it);
    }

    if (keywords.size() == 0 || extensions.size() == 0)
        return;

    _ui->pushButton_search->setEnabled(false);
    _ui->pushButton_load->setEnabled(false);
    _ui->listWidget_results->clear();

    _rootDir = Settings::_pack0;

    _thread = new QThread();
    _searchEngine = new SearchEngine(_rootDir, keywords, extensions, _ui->checkBox_folder->isChecked());
    _searchEngine->moveToThread(_thread);

    QObject::connect(_thread, SIGNAL(started()), _searchEngine, SLOT(run()));
    QObject::connect(_searchEngine, SIGNAL(onProgress(int)), this, SLOT(setProgress(int)));
    QObject::connect(_searchEngine, SIGNAL(finished()), this, SLOT(searchEnd()));
    QObject::connect(_searchEngine, SIGNAL(sendItem(QString)), this, SLOT(addResult(QString)));

    _thread->start();
}

void Search::killThread()
{
    if (!_thread || !_searchEngine)
        return;

    _thread->quit();
    _thread->deleteLater();
    _searchEngine->deleteLater();

    _thread = 0;
    _searchEngine = 0;
}

void Search::setProgress(int progress)
{
    _ui->progressBar_search->setValue(progress);
}

void Search::addResult(QString item)
{
    _ui->listWidget_results->addItem(item);
}

void Search::searchEnd()
{
    _ui->pushButton_search->setEnabled(true);
    killThread();
}

void Search::load()
{
    emit loadPressed(_rootDir + _ui->listWidget_results->currentItem()->text().remove(0, 12));
}

void Search::enableButton()
{
    _ui->pushButton_load->setEnabled(_ui->listWidget_results->currentRow() != -1);
}

void Search::resetExtensionsFilter()
{
    _ui->lineEdit_extensionsFilter->setText("w2mesh w2ent w2rig w2anims");
}


// Search ------------------------
SearchEngine::SearchEngine(QString rootDir, QStringList keywords, QStringList extensions, bool searchFolders)
    : _rootDir(rootDir), _keywords(keywords), _extensions(extensions), _searchFolders(searchFolders), _stopped(false)
{

}

void SearchEngine::run()
{
    _rootDir = QDir::cleanPath(_rootDir);
    scanFolder(_rootDir, 0);
    emit finished();
}

void SearchEngine::scanFolder(QString repName, int level)
{   
    level++;

    if (_stopped)
        return;

    // search the files
    QDir dirFiles(repName);
    dirFiles.setFilter(QDir::NoDotAndDotDot | QDir::Files);
    dirFiles.setNameFilters(_extensions);

    foreach(QFileInfo fileInfo, dirFiles.entryInfoList())
    {
        QString target = fileInfo.fileName();

        if (_searchFolders)
        {
            target = fileInfo.absolutePath() + fileInfo.fileName();
            target.remove(0, _rootDir.size());
        }

        bool ok = true;
        for (int i = 0; i < _keywords.size(); i++)
        {
            if (!target.contains(_keywords[i], Qt::CaseInsensitive))
            {
                ok = false;
                break;
            }
        }
        if (ok)
            emit sendItem("{Search dir}" + fileInfo.absoluteFilePath().remove(0, _rootDir.size()));
    }

    // search in the subfolders
    dirFiles.setNameFilters(QStringList());
    dirFiles.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
    int i = 0;
    foreach (QFileInfo fileInfo, dirFiles.entryInfoList())
    {
        scanFolder(fileInfo.absoluteFilePath(), level);
        if (level == 1)
        {
            emit onProgress(((float)++i)/dirFiles.entryInfoList().size() * 100);
        }
    }
}

void SearchEngine::quitThread()
{
    _stopped = true;
}
