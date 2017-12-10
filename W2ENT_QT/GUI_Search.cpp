#include "GUI_Search.h"
#include "ui_GUI_Search.h"

#include <iostream>


// UI -------------------------------
GUI_Search::GUI_Search(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_Search), _thread(0), _searchEngine(0)
{
    _ui->setupUi(this);

    resetExtensionsFilter();
    translate();

    QObject::connect(_ui->pushButton_search, SIGNAL(clicked()), this, SLOT(search()));
    QObject::connect(_ui->pushButton_load, SIGNAL(clicked()), this, SLOT(load()));
    QObject::connect(_ui->pushButton_reset, SIGNAL(clicked()), this, SLOT(resetExtensionsFilter()));
    QObject::connect(_ui->listWidget_results, SIGNAL(currentRowChanged(int)), this, SLOT(enableButton()));

    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_Search::~GUI_Search()
{
    delete _ui;
}

void GUI_Search::destroyWindow()
{
    if (_thread)
    {
        _searchEngine->quitThread();
        while (_thread)
            QCoreApplication::processEvents();
    }

    delete this;
}

void GUI_Search::translate()
{
    _ui->label_name->setText(Translator::get("search_name") + " :");
    _ui->label_result->setText(Translator::get("search_result") + " :");
    _ui->label_progression->setText(Translator::get("search_progress") + " :");
    _ui->label_extensions->setText(Translator::get("search_extensions_filter"));
    _ui->checkBox_folder->setText(Translator::get("search_check_folder"));
    _ui->pushButton_search->setText(Translator::get("search_button"));
    _ui->pushButton_load->setText(Translator::get("search_load"));
}

void GUI_Search::search()
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

    if (_rootDir != Settings::_pack0)
    {
        _rootDir = QDir::cleanPath(Settings::_pack0);
        updateFafSettings();
    }

    _thread = new QThread();
    _searchEngine = new SearchEngine(_rootDir, keywords, extensions, _ui->checkBox_folder->isChecked(), _useFafSearch, _fafSearchFilesIndex);
    _searchEngine->moveToThread(_thread);

    QObject::connect(_thread, SIGNAL(started()), _searchEngine, SLOT(run()));
    QObject::connect(_searchEngine, SIGNAL(onProgress(int)), this, SLOT(setProgress(int)));
    QObject::connect(_searchEngine, SIGNAL(finished()), this, SLOT(searchEnd()));
    QObject::connect(_searchEngine, SIGNAL(sendItem(QString)), this, SLOT(addResult(QString)));

    _thread->start();
}

void GUI_Search::updateFafSettings()
{
    // check if there is an index file
    QString indexFilename = _rootDir + "/files.txt";
    QFileInfo filesTxtInfos(indexFilename);
    if (filesTxtInfos.exists() && filesTxtInfos.isFile())
    {
        QFile indexfile(indexFilename);
        if (indexfile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            _fafSearchFilesIndex = indexfile.readAll();
            indexfile.close();
            _useFafSearch = true;
        }
        else
            _useFafSearch = false;
    }
    else
        _useFafSearch = false;
}

void GUI_Search::killThread()
{
    if (!_thread || !_searchEngine)
        return;

    _thread->quit();
    _thread->deleteLater();
    _searchEngine->deleteLater();

    _thread = 0;
    _searchEngine = 0;
}

void GUI_Search::setProgress(int progress)
{
    _ui->progressBar_search->setValue(progress);
}

void GUI_Search::addResult(QString item)
{
    _ui->listWidget_results->addItem(item);
}

void GUI_Search::searchEnd()
{
    _ui->pushButton_search->setEnabled(true);
    killThread();
}

void GUI_Search::load()
{
    emit loadPressed(_rootDir + _ui->listWidget_results->currentItem()->text().remove(0, 12));
}

void GUI_Search::enableButton()
{
    _ui->pushButton_load->setText("Load");
    _ui->pushButton_load->setEnabled(_ui->listWidget_results->currentRow() != -1);
    if (_ui->listWidget_results->currentRow() == -1)
        return;


    QString selected = _ui->listWidget_results->currentItem()->text();
    WitcherContentType type = getTWFileContentType(QSTRING_TO_PATH(selected));
    if (type == WTC_ENTITY)
    {
        _ui->pushButton_load->setText("Load entity");
    }
    else if (type == WTC_MESH)
    {
        _ui->pushButton_load->setText("Load mesh");
    }
    else if (type == WTC_RIG)
    {
        _ui->pushButton_load->setText("Load rig");
    }
    else if (type == WTC_ANIMATIONS)
    {
        _ui->pushButton_load->setText("Load animations");
    }
}

void GUI_Search::resetExtensionsFilter()
{
    _ui->lineEdit_extensionsFilter->setText("w2mesh w2ent w2rig w2anims");
}


// Search ------------------------
SearchEngine::SearchEngine(QString rootDir, QStringList keywords, QStringList extensions, bool searchFolders, bool useFafSearch, QString &index)
    : _rootDir(rootDir), _keywords(keywords), _extensions(extensions), _searchFolders(searchFolders), _useFafSearch(useFafSearch), _fafSearchFilesIndex(index), _stopped(false)
{

}

void SearchEngine::run()
{
    if (_useFafSearch)
    {
        // search from files.txt file
        fafSearch();
    }
    else
    {
        // classic search
        scanFolder(_rootDir, 0);
    }

    emit finished();
}


void SearchEngine::isASearchedFile(QFileInfo& fileInfo)
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

void SearchEngine::fafSearch()
{
    _fafSearchNextProgressionCap = 10;

    const qint64 fileSize = _fafSearchFilesIndex.size();
    qint64 customPos = 0;

    QTextStream in(&_fafSearchFilesIndex);
    while (!in.atEnd())
    {
        if (_stopped)
            break;

        // read
        QString line = in.readLine();
        customPos += line.size()+1;

        // test file
        QFileInfo fileInfo(line);
        if (_extensions.contains(QString("*.") + fileInfo.completeSuffix()))
            isASearchedFile(fileInfo);

        // update progression
        //std::cout << line.toStdString().c_str() << std::endl;
        int newProgression = (customPos * 100)/fileSize;
        if (newProgression > _fafSearchNextProgressionCap)
        {
           _fafSearchNextProgressionCap = newProgression + 10;
           emit onProgress(newProgression);
        }
    }
    emit onProgress(100);
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
        isASearchedFile(fileInfo);
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
