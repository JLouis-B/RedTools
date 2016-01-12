#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QCheckBox>
#include <QTextCodec>
#include <iostream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _ui(new Ui::MainWindow)
{
    // setup the UI
    _ui->setupUi(this);
    this->setFixedSize(this->size());

    _currentLOD = LOD_0;

    // Logs
    _ui->textEdit_log->setReadOnly (true);
    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + "The Witcher 3D models converter 2.3.1\n");


    Settings::_pack0 = _ui->lineEdit_folder->text();
    Settings::loadFromXML("config.xml");
    _ui->lineEdit_folder->setText(Settings::_pack0);

    _firstSelection = true;

    // Search the translations files
    QDir dir(QCoreApplication::applicationDirPath() + "/langs/");

    QStringList filter;
    filter << "*.xml";
    dir.setNameFilters(filter);

    QStringList files = dir.entryList();
    foreach( QString file, files )
    {
        QAction* menuitem = new QAction(file, _ui->menuLanguages);
        menuitem->setCheckable(true);
        if ("langs/" + menuitem->text() == Settings::_language)
            menuitem->setChecked(true);
        _ui->menuLanguages->addAction(menuitem);
    }

    // The parent widget if the translator has to popup an error message box
    Translator::setParentWidget(this);
    Translator::loadCurrentLanguage();

    // Translate the UI
    translate();

    if (Settings::_firstUse)
    {
        QMessageBox msgBox(QMessageBox::Information, "First usage", "This is your first usage of the software. The first step to extract The Witcher assets is to configure the software by giving the path to the game folder, the textures, checking the export options...<br/> You can find all the informations about this on the <a href=\"http://jlouisb.users.sourceforge.net/\">website</a> of the software, and it's highly recommended to read them before to start to use the software.");
        QCheckBox* dontShow = new QCheckBox("Don't show this message again", &msgBox);

        msgBox.setCheckBox(dontShow);
        msgBox.exec();
        Settings::_firstUse = !dontShow->isChecked();
        delete dontShow;
    }

    // Events
    QObject::connect(_ui->actionWireframe, SIGNAL(triggered(bool)), this, SLOT(changeWireframe(bool)));
    QObject::connect(_ui->actionRigging, SIGNAL(triggered(bool)), this, SLOT(changeRigging(bool)));

    QObject::connect(_ui->actionQuitter, SIGNAL(triggered()), this, SLOT(close()));
    QObject::connect(_ui->actionWebpage, SIGNAL(triggered()), this, SLOT(openWebpage()));
    QObject::connect(_ui->button_fileSelector, SIGNAL(clicked()), this, SLOT(selectFile()));
    QObject::connect(_ui->button_convert, SIGNAL(clicked()), this, SLOT(convertir()));
    QObject::connect(_ui->button_selectDir, SIGNAL(clicked()), this, SLOT(selectFolder()));
    QObject::connect(_ui->actionSearch, SIGNAL(triggered()), this, SLOT(search()));
    QObject::connect(_ui->actionOptions, SIGNAL(triggered()), this, SLOT(options()));
    QObject::connect(_ui->actionSize, SIGNAL(triggered()), this, SLOT(re_size()));
    QObject::connect(_ui->comboBox_format, SIGNAL(currentTextChanged(QString)), this, SLOT(checkConvertButton()));

    QObject::connect(_ui->actionClean_textures_path, SIGNAL(triggered()), this, SLOT(cleanTexturesPath()));

    QObject::connect(_ui->actionLOD0, SIGNAL(triggered()), this, SLOT(changeLOD()));
    QObject::connect(_ui->actionLOD1, SIGNAL(triggered()), this, SLOT(changeLOD()));
    QObject::connect(_ui->actionLOD2, SIGNAL(triggered()), this, SLOT(changeLOD()));
    QObject::connect(_ui->actionCollision_mesh, SIGNAL(triggered()), this, SLOT(changeLOD()));

    QObject::connect(_ui->actionClear_current_LOD, SIGNAL(triggered()), this, SLOT(clearLOD()));
    QObject::connect(_ui->actionClear_all_LODs, SIGNAL(triggered()), this, SLOT(clearAllLODs()));
    QObject::connect(_ui->actionShow_linked_files, SIGNAL(triggered()), this, SLOT(extFiles()));

    QObject::connect(_ui->lineEdit_folder, SIGNAL(textChanged(QString)), this, SLOT(changeBaseDir(QString)));

    QObject::connect(_ui->actionSet_rig, SIGNAL(triggered(bool)), this, SLOT(loadRig()));
    QObject::connect(_ui->actionAdd_mesh_2, SIGNAL(triggered(bool)), this, SLOT(addMesh()));
    QObject::connect(_ui->actionBIF_extractor, SIGNAL(triggered(bool)), this, SLOT(bifExtractor()));

    for (int i = 0; i < _ui->menuLanguages->actions().size(); i++)
        QObject::connect(_ui->menuLanguages->actions().at(i), SIGNAL(triggered()), this, SLOT(changeLanguage()));


}

void MainWindow::addMesh()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Select the meshes to load", Settings::_pack0, Settings::_formats);

    for (int i = 0; i < files.size(); ++i)
    {
        const QString file = files.at(i);
        if (!_irrWidget->fileIsOpenableByIrrlicht(file))
        {
            QMessageBox::critical(this, "Error", "Error : The file " + file + " can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
            continue;
        }

        _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + Translator::findTranslation("log_readingFile") + " '" + file + "'... ");

        core::stringc feedbackMessage;

        if (_irrWidget->isEmpty(_currentLOD))
            _irrWidget->setModel(file, feedbackMessage);
        else
            _irrWidget->addMesh(file, feedbackMessage);

        _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + feedbackMessage.c_str() + "\n");
    }

    if (!_irrWidget->isEmpty(_currentLOD))
    {
        _irrWidget->changeWireframe(_ui->actionWireframe->isChecked());
        _irrWidget->changeRigging(_ui->actionRigging->isChecked());

        _ui->button_convert->setEnabled(true);
        _ui->actionSize->setEnabled(true);

        switch(_currentLOD)
        {
            case LOD_0:
                _ui->actionLOD0->setText("LOD0");
            break;
            case LOD_1:
                _ui->actionLOD1->setText("LOD1");
            break;
            case LOD_2:
                _ui->actionLOD2->setText("LOD2");
            break;
            case Collision:
                _ui->actionCollision_mesh->setText("Collision mesh");
            break;
        }
    }

    updateWindowTitle();
}

void MainWindow::loadRig()
{
    QString file = QFileDialog::getOpenFileName(this, "Select the w2rig file to use", Settings::_pack0, "The Witcher 3 rig (*.w2rig , *.w3fac)");

    if (file == "")
        return;

    core::stringc feedback;
    bool sucess = _irrWidget->loadRig(QSTRING_TO_PATH(file), feedback);

    if (sucess)
        QMessageBox::information(this, "Sucess", feedback.c_str());
    else
        QMessageBox::critical(this, "Error", feedback.c_str());

    updateWindowTitle();
}


void MainWindow::changeBaseDir(QString newPath)
{
    Settings::_pack0 = newPath;
}

void MainWindow::updateWindowTitle()
{
    QString title = "The Witcher 3D models converter";

    QString filename = _irrWidget->getFilename();
    if (filename == "")
        filename = "3D Model";

    if (!_irrWidget->isEmpty(_currentLOD))
    {
        title += " - " + filename + " - " + QString::number(_irrWidget->getPolysCount()) + " polys - " + QString::number(_irrWidget->getJointsCount()) + " joints";
    }

    this->setWindowTitle(title);
}


void MainWindow::changeLanguage()
{
    for (int i = 0; i < _ui->menuLanguages->actions().size(); i++)
        _ui->menuLanguages->actions().at(i)->setChecked(false);

    QAction* q = (QAction*)QObject::sender();
    q->setChecked(true);
    Settings::_language = "langs/" + q->text();
    Translator::loadCurrentLanguage();
    translate();
    emit languageChanged();
}


void MainWindow::cleanTexturesPath()
{
    QDir dir(Settings::_TW3TexPath);
    if (dir.isReadable())
    {
        CleanTexturesPath* w = new CleanTexturesPath (Settings::_TW3TexPath, this);
        w->show();
        w->clean();
        delete w;
    }
    else
        QMessageBox::critical(this, "Error", "TW3 textures folder can't be opened. Check the tuto for more informations.");
}

MainWindow::~MainWindow()
{
    Settings::saveToXML("config.xml");

    // Delete UI
    delete _ui;

    if (_irrWidget)
        delete _irrWidget;
}



void MainWindow::initIrrlicht()
{
    _irrWidget = new QIrrlichtWidget(this);
    _irrWidget->show();
    _irrWidget->setGeometry(0, 40, 768, 432);
    _irrWidget->init();
}

void MainWindow::selectFile()
{
    QString param = _ui->lineEdit_ImportedFile->text();
    if (_firstSelection)
        param = _ui->lineEdit_folder->text();

    QString file = QFileDialog::getOpenFileName(this, Translator::findTranslation("dialogue_file"), param, Settings::_formats);

    if (file != "")
    {
        loadFile(file);
    }
}

void MainWindow::convertir()
{
    // Warning if no filename specified
    if (_ui->lineEdit_exportedFilename->text() == "")
        _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + Translator::findTranslation("log_warning_empty") + "\n");

    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + Translator::findTranslation("log_writingFile") + " '" + _ui->lineEdit_exportedFilename->text()+ _ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).left(_ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).indexOf(' ')) + "'... ");
    QCoreApplication::processEvents();

    core::stringc feedback = "";

    // Check if the destination folder exist
    QDir dir(Settings::getExportFolder());
    if (dir.exists())
        _irrWidget->writeFile(Settings::getExportFolder(), _ui->lineEdit_exportedFilename->text(), _ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).left(_ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).indexOf(' ')), feedback);
    else
    {
        QMessageBox::warning(this, "Error", "The destination folder '" + Settings::_exportDest + "' doesn't exist.");
        _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + "\n" + Translator::findTranslation("log_abort") + "\n");
        return;
    }

    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + feedback.c_str() + "\n");
}

void MainWindow::translate()
{
    _ui->button_fileSelector->setText(Translator::findTranslation("button_fileSelector"));
    _ui->button_convert->setText(Translator::findTranslation("button_convertir"));
    _ui->label_baseDir->setText(Translator::findTranslation("base_directory") + " :");
    _ui->menuLanguages->setTitle(Translator::findTranslation("menu_language"));
    _ui->menuMenu->setTitle(Translator::findTranslation("menu_menu"));
    _ui->actionQuitter->setText(Translator::findTranslation("menu_quit"));
    _ui->menuDisplay->setTitle(Translator::findTranslation("menu_display"));
    _ui->actionWireframe->setText(Translator::findTranslation("menu_wireframe"));
    _ui->actionSearch->setText(Translator::findTranslation("menu_search"));
    _ui->actionOptions->setText(Translator::findTranslation("menu_options"));
    _ui->actionWebpage->setText(Translator::findTranslation("menu_webpage"));
    _ui->actionSet_rig->setText(Translator::findTranslation("menu_setRig"));
    _ui->actionAdd_mesh_2->setText(Translator::findTranslation("menu_addMesh"));
    _ui->actionShow_linked_files->setText(Translator::findTranslation("menu_linkedFiles"));
    _ui->menuHelp->setTitle(Translator::findTranslation("menu_help"));
    _ui->label_exportedFilename->setText(Translator::findTranslation("label_exported_file_name") + " :");
    _ui->actionClear_current_LOD->setText(Translator::findTranslation("re_lod_clear"));
    _ui->actionClear_all_LODs->setText(Translator::findTranslation("re_lod_clear_all"));
    _ui->actionSize->setText(Translator::findTranslation("re_size"));
    _ui->menu_RE_tools->setTitle(Translator::findTranslation("re_tools"));
    _ui->menuThe_Witcher_3_tools->setTitle(Translator::findTranslation("w3_tools"));
    _ui->actionClean_textures_path->setText(Translator::findTranslation("w3_tex_path"));


    if (_ui->actionLOD0->text() != "LOD0")
        _ui->actionLOD0->setText("LOD0 (" + Translator::findTranslation("re_lod_empty") + ")");
    if (_ui->actionLOD1->text() != "LOD1")
        _ui->actionLOD1->setText("LOD1 (" + Translator::findTranslation("re_lod_empty") + ")");
    if (_ui->actionLOD2->text() != "LOD2")
        _ui->actionLOD2->setText("LOD2 (" + Translator::findTranslation("re_lod_empty") + ")");
}

void MainWindow::selectFolder()
{
    QString translation = Translator::findTranslation("dialogue_folder");
    QString folder = QFileDialog::getExistingDirectory(this, translation, _ui->lineEdit_folder->text());
    if (folder != "")
    {
        _ui->lineEdit_folder->setText(folder);
    }
}

void MainWindow::changeWireframe(bool enable)
{
    _irrWidget->changeWireframe(enable);
}

void MainWindow::changeRigging(bool enable)
{
    _irrWidget->changeRigging(enable);
}

void MainWindow::openWebpage()
{
    QString link = "http://jlouisb.users.sourceforge.net/";
    QDesktopServices::openUrl(QUrl(link));
}

void MainWindow::options()
{
    Options *w = new Options (this, _irrWidget->getPath(), _irrWidget);
    w->show();
    QObject::connect(w, SIGNAL(optionsValidation()), this, SLOT(changeOptions()));
}

void MainWindow::search()
{
    Search* w = new Search (this);
    w->show();
    QObject::connect(w, SIGNAL(loadPressed(QString)), this, SLOT(loadFile(QString)));
    QObject::connect(this, SIGNAL(languageChanged()), w, SLOT(translate()));
}


void MainWindow::loadFile(QString path)
{
    if (!_irrWidget->fileIsOpenableByIrrlicht(path))
    {
        QMessageBox::critical(this, "Error", "Error : The file can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
        return;
    }

    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + Translator::findTranslation("log_readingFile") + " '" + path + "'... ");
    _ui->lineEdit_ImportedFile->setText(path);
    QCoreApplication::processEvents();

    core::stringc feedbackMessage = "";
    bool success = _irrWidget->setModel(path, feedbackMessage);
    if (success)
    {
        _irrWidget->changeWireframe(_ui->actionWireframe->isChecked());
        _irrWidget->changeRigging(_ui->actionRigging->isChecked());

        _ui->button_convert->setEnabled(true);
        _ui->actionSize->setEnabled(true);

        const QFileInfo fileInfo (path);
        const QString filename = fileInfo.fileName();
        const QString extension = fileInfo.suffix();

        _ui->actionShow_linked_files->setEnabled(extension == "w2ent" || extension == "w2mesh");

        switch(_currentLOD)
        {
            case LOD_0:
                _ui->actionLOD0->setText("LOD0");
            break;
            case LOD_1:
                _ui->actionLOD1->setText("LOD1");
            break;
            case LOD_2:
                _ui->actionLOD2->setText("LOD2");
            break;
            case Collision:
                _ui->actionCollision_mesh->setText("Collision mesh");
            break;
        }
        _firstSelection = false;
    }

    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + feedbackMessage.c_str() + "\n");
    updateWindowTitle();
}

void MainWindow::changeOptions()
{
    _irrWidget->changeOptions();
}

void MainWindow::re_size()
{
    ReSize* r = new ReSize(this);
    r->show();
}

void MainWindow::changeLOD()
{    
    for (int i = 0; i < _ui->menuLOD->actions().size(); i++)
        _ui->menuLOD->actions().at(i)->setChecked(false);

    QAction* q = (QAction*)QObject::sender();
    q->setChecked(true);

    LOD newlod = LOD_0;
    if (q == _ui->actionLOD0)
        newlod = LOD_0;
    else if (q == _ui->actionLOD1)
        newlod = LOD_1;
    else if (q == _ui->actionLOD2)
        newlod = LOD_2;
    else if (q == _ui->actionCollision_mesh)
        newlod = Collision;

    _irrWidget->changeLOD(newlod);
    _currentLOD = newlod;

    checkConvertButton();
    updateWindowTitle();
}

void MainWindow::clearLOD()
{
    switch(_currentLOD)
    {
        case LOD_0:
            _ui->actionLOD0->setText("LOD0 (" + Translator::findTranslation("re_lod_empty") + ")");
        break;
        case LOD_1:
            _ui->actionLOD1->setText("LOD1 (" + Translator::findTranslation("re_lod_empty") + ")");
        break;
        case LOD_2:
            _ui->actionLOD2->setText("LOD2 (" + Translator::findTranslation("re_lod_empty") + ")");
        break;
        case Collision:
            _ui->actionLOD2->setText("Collision mesh (" + Translator::findTranslation("re_lod_empty") + ")");
        break;
    }

    _irrWidget->clearLOD();

    _ui->actionShow_linked_files->setEnabled(false);

    _ui->actionSize->setEnabled(false);

    checkConvertButton();
    updateWindowTitle();
}

void MainWindow::clearAllLODs()
{
    _ui->actionLOD0->setText("LOD0 (" + Translator::findTranslation("re_lod_empty") + ")");
    _ui->actionLOD1->setText("LOD1 (" + Translator::findTranslation("re_lod_empty") + ")");
    _ui->actionLOD2->setText("LOD2 (" + Translator::findTranslation("re_lod_empty") + ")");

    _irrWidget->clearAllLODs();

    _ui->actionShow_linked_files->setEnabled(false);

    _ui->actionSize->setEnabled(false);

    checkConvertButton();
    updateWindowTitle();
}

void MainWindow::checkConvertButton()
{
    if (_ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).left(_ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).indexOf(' ')) == ".re")
    {
        _ui->button_convert->setEnabled(!_irrWidget->isEmpty(LOD_0) || !_irrWidget->isEmpty(LOD_1) || !_irrWidget->isEmpty(LOD_2) || !_irrWidget->isEmpty(Collision));
    }
    else
    {
        _ui->button_convert->setEnabled(!_irrWidget->isEmpty(_currentLOD));
    }
}

void MainWindow::extFiles()
{
    ExtFiles* ext = new ExtFiles(_irrWidget, this);
    ext->read(_ui->lineEdit_ImportedFile->text());
    ext->show();
}

void MainWindow::bifExtractor()
{
    tw1bifextractorUI* bif = new tw1bifextractorUI(this);
    bif->show();
}

