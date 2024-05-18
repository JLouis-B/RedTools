#include "GUI_Search.h"
#include "ui_GUI_Search.h"

#include <QDir>
#include <QDirIterator>
#include <QTextStream>

#include "Settings.h"
#include "Translator.h"
#include "Utils_Qt_Irr.h"
#include "Utils_RedEngine.h"

#include <iostream>


// UI -------------------------------
GUI_Search::GUI_Search(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::GUI_Search), _thread(nullptr), _searchEngine(nullptr)
{
    _ui->setupUi(this);

    SearchSettings settings = Settings::_searchSettings;
    if (settings._windowGeometry.size() > 0)
        restoreGeometry(settings._windowGeometry);
    _ui->checkBox_checkFolderNames->setChecked(settings._checkFolderNames);
    _ui->checkBox_searchMeshes->setChecked(settings._searchRedMeshes);
    _ui->checkBox_searchRigs->setChecked(settings._searchRedRigs);
    _ui->checkBox_searchAnimations->setChecked(settings._searchRedAnimations);
    _ui->lineEdit_additionalExtensions->setText(settings._additionnalExtensions);

    translate();

    QObject::connect(_ui->pushButton_search, SIGNAL(clicked()), this, SLOT(search()));
    QObject::connect(_ui->pushButton_load, SIGNAL(clicked()), this, SLOT(load()));
    QObject::connect(_ui->listWidget_results, SIGNAL(currentRowChanged(int)), this, SLOT(enableButton()));

    QObject::connect(this, SIGNAL(finished(int)), this, SLOT(destroyWindow()));

    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_Search::~GUI_Search()
{
    SearchSettings settings;
    settings._windowGeometry = this->saveGeometry();
    settings._checkFolderNames = _ui->checkBox_checkFolderNames->isChecked();
    settings._searchRedMeshes = _ui->checkBox_searchMeshes->isChecked();
    settings._searchRedRigs = _ui->checkBox_searchRigs->isChecked();
    settings._searchRedAnimations = _ui->checkBox_searchAnimations->isChecked();
    settings._additionnalExtensions = _ui->lineEdit_additionalExtensions->text();
    Settings::_searchSettings = settings;

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
    deleteLater();
}

void GUI_Search::translate()
{
    _ui->label_name->setText(Translator::get("search_name") + " :");
    _ui->label_result->setText(Translator::get("search_result") + " :");
    _ui->label_progression->setText(Translator::get("search_progress") + " :");
    _ui->checkBox_checkFolderNames->setText(Translator::get("search_check_folder"));
    _ui->pushButton_search->setText(Translator::get("search_button"));
    _ui->pushButton_load->setText(Translator::get("search_load"));
}

void GUI_Search::search()
{
    QString name = _ui->lineEdit_name->text();
    QStringList keywords = name.split(" ", Qt::SkipEmptyParts);
    QStringList extensions = _ui->lineEdit_additionalExtensions->text().split(" ", Qt::SkipEmptyParts);
    if (_ui->checkBox_searchMeshes->isChecked())
    {
        extensions.push_back("w2mesh");
        extensions.push_back("w2ent");
    }
    if (_ui->checkBox_searchRigs->isChecked())
    {
        extensions.push_back("w2rig");
    }
    if (_ui->checkBox_searchAnimations->isChecked())
    {
        extensions.push_back("w2anims");
    }

    for (QStringList::iterator it = extensions.begin(); it != extensions.end(); ++it)
    {
        *it = "*." + (*it);
    }

    if (keywords.size() == 0 || extensions.size() == 0)
        return;

    _ui->pushButton_search->setEnabled(false);
    _ui->pushButton_load->setEnabled(false);
    _ui->listWidget_results->clear();

    if (_rootDir != Settings::_baseDir)
    {
        _rootDir = QDir::cleanPath(Settings::_baseDir);
        updateFafSettings();
    }

    _thread = new QThread();
    _searchEngine = new SearchEngine(_rootDir, keywords, extensions, _ui->checkBox_checkFolderNames->isChecked(), _useFafSearch, _fafSearchFilesIndex);
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

    _thread = nullptr;
    _searchEngine = nullptr;
}

void GUI_Search::setProgress(int progress)
{
    if (progress >= 0)
    {
        _ui->progressBar_search->setMaximum(100);
        _ui->progressBar_search->setValue(progress);
    }
    else
    {
        _ui->progressBar_search->setMaximum(0); // infinite mode
    }
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
    RedEngineContentType type = getRedEngineFileContentType(qStringToIrrPath(selected));
    if (type == RECT_WITCHER_ENTITY)
    {
        _ui->pushButton_load->setText("Load entity");
    }
    else if (type == RECT_WITCHER_MESH)
    {
        _ui->pushButton_load->setText("Load mesh");
    }
    else if (type == RECT_WITCHER_RIG)
    {
        _ui->pushButton_load->setText("Load rig");
    }
    else if (type == RECT_WITCHER_ANIMATIONS)
    {
        _ui->pushButton_load->setText("Load animations");
    }
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
        scanFolder(_rootDir);
    }

    emit finished();
}


void SearchEngine::checkIfIsASearchedFile(QFileInfo& fileInfo)
{
    QString target = fileInfo.fileName();
    if (_searchFolders)
    {
        target = fileInfo.absolutePath() + fileInfo.fileName();
        target.remove(0, _rootDir.size());
    }

    for (int i = 0; i < _keywords.size(); i++)
    {
        if (!target.contains(_keywords[i], Qt::CaseInsensitive))
        {
            return;
        }
    }

    emit sendItem("{Search dir}" + fileInfo.absoluteFilePath().remove(0, _rootDir.size()));
}

void SearchEngine::fafSearch()
{
    int nextProgressionUpdate = 10;

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
            checkIfIsASearchedFile(fileInfo);

        // update progression
        //std::cout << line.toStdString().c_str() << std::endl;
        int newProgression = (customPos * 100)/fileSize;
        if (newProgression > nextProgressionUpdate)
        {
           nextProgressionUpdate = newProgression + 10;
           emit onProgress(newProgression);
        }
    }
    emit onProgress(100);
}

void SearchEngine::scanFolder(QString repName)
{   
    emit onProgress(-1);

    // search the files
    QDirIterator it(repName, _extensions, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        checkIfIsASearchedFile(fileInfo);

        if (_stopped)
            return;
    }

    emit onProgress(100);
}

void SearchEngine::quitThread()
{
    _stopped = true;
}
