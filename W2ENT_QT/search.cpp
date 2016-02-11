#include "search.h"
#include "ui_search.h"

#include <iostream>


// UI -------------------------------
Search::Search(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::Search), _thread(0), _searchEngine(0)
{
    _ui->setupUi(this);

    translate();

    QObject::connect(_ui->pushButton_search, SIGNAL(clicked()), this, SLOT(search()));
    QObject::connect(_ui->pushButton_load, SIGNAL(clicked()), this, SLOT(load()));
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
        while(_thread /*&& !_thread->isFinished()*/)
            QCoreApplication::processEvents();
    }

    delete this;
}

void Search::translate()
{
    _ui->label_name->setText(Translator::findTranslation("search_name") + " :");
    _ui->label_result->setText(Translator::findTranslation("search_result") + " :");
    _ui->label->setText(Translator::findTranslation("search_progress") + " :");
    _ui->checkBox_folder->setText(Translator::findTranslation("search_check_folder"));
    _ui->pushButton_search->setText(Translator::findTranslation("search_button"));
    _ui->pushButton_load->setText(Translator::findTranslation("search_load"));
}

void Search::search()
{
    QString name = _ui->lineEdit_name->text();
    QStringList keywords = name.split(" ", QString::SkipEmptyParts);
    if (keywords.size() == 0)
        return;

    _ui->pushButton_search->setEnabled(false);
    _ui->pushButton_load->setEnabled(false);
    _ui->listWidget_results->clear();

    _pack0lastSearch = Settings::_pack0;

    //scanFolder(Settings::_pack0, 0, keywords);
    _thread = new QThread();
    _searchEngine = new SearchEngine(keywords, _ui->checkBox_folder->isChecked());
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
    emit loadPressed(_pack0lastSearch + _ui->listWidget_results->currentItem()->text().remove(0, 7));
}

void Search::enableButton()
{
    _ui->pushButton_load->setEnabled(_ui->listWidget_results->currentRow() != -1);
}

void SearchEngine::run()
{
    scanFolder(Settings::_pack0, 0);
    emit finished();
}


// Search ------------------------
SearchEngine::SearchEngine(QStringList keywords, bool searchFolders) : _keywords(keywords), _searchFolders(searchFolders), _stopped(false)
{

}

void SearchEngine::scanFolder(QString repName, int level)
{   
    level++;

    if (_stopped)
        return;

    // search the w2ent and w2mesh file
    QDir dirFiles(repName);
    dirFiles.setFilter(QDir::NoDotAndDotDot | QDir::Files);
    dirFiles.setNameFilters(QStringList() << "*.w2mesh" << "*.w2ent" << "*.w2rig");

    if (level == 1)
    {
        _baseDir = QDir::cleanPath(repName);
    }

    foreach(QFileInfo fileInfo, dirFiles.entryInfoList())
    {
        QString target = fileInfo.fileName();

        if (_searchFolders)
        {
            target = fileInfo.absolutePath() + fileInfo.fileName();
            target.remove(0, Settings::_pack0.size());
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
            emit sendItem("{pack0}" + fileInfo.absoluteFilePath().remove(0, _baseDir.size()));
    }

    // search in the subfolders
    //QDir dirSubfolder(repName);
    dirFiles.setNameFilters(QStringList());
    dirFiles.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
    float i = 0.f;
    foreach(QFileInfo fileInfo, dirFiles.entryInfoList())
    {
        scanFolder(fileInfo.absoluteFilePath(), level);
        i++;
        if (level == 1)
        {
            emit onProgress((i/dirFiles.entryInfoList().size())*(100));
        }
    }
}

void SearchEngine::quitThread()
{
    _stopped = true;
}
