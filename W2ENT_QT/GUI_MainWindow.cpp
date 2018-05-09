#include "GUI_MainWindow.h"
#include "ui_GUI_MainWindow.h"
#include <QDir>
#include <QCheckBox>
#include <QTextCodec>
#include <iostream>

GUI_MainWindow::GUI_MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _ui(new Ui::GUI_MainWindow)
{
    // setup the UI
    _ui->setupUi(this);
    //this->setFixedSize(this->size());

    _currentLOD = LOD_0;

    Settings::_pack0 = _ui->lineEdit_folder->text();
    Settings::loadFromXML(QCoreApplication::applicationDirPath() + "/config.xml");
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
    // Menu
    QObject::connect(_ui->action_main_Search, SIGNAL(triggered()), this, SLOT(search()));
    QObject::connect(_ui->action_main_Add_mesh, SIGNAL(triggered(bool)), this, SLOT(addMesh()));
    QObject::connect(_ui->action_main_Options, SIGNAL(triggered()), this, SLOT(options()));
    QObject::connect(_ui->action_main_Quitter, SIGNAL(triggered()), this, SLOT(close()));

    QObject::connect(_ui->action_redkit_Size, SIGNAL(triggered()), this, SLOT(re_size()));
    QObject::connect(_ui->action_redkit_LOD0, SIGNAL(triggered()), this, SLOT(changeLOD()));
    QObject::connect(_ui->action_redkit_LOD1, SIGNAL(triggered()), this, SLOT(changeLOD()));
    QObject::connect(_ui->action_redkit_LOD2, SIGNAL(triggered()), this, SLOT(changeLOD()));
    QObject::connect(_ui->action_redkit_Collision_mesh, SIGNAL(triggered()), this, SLOT(changeLOD()));
    QObject::connect(_ui->action_redkit_Clear_current_LOD, SIGNAL(triggered()), this, SLOT(clearLOD()));
    QObject::connect(_ui->action_redkit_Clear_all_LODs, SIGNAL(triggered()), this, SLOT(clearAllLODs()));

    QObject::connect(_ui->action_TW1_BIF_extractor, SIGNAL(triggered(bool)), this, SLOT(bifExtractor()));

    QObject::connect(_ui->action_TW2_DZIP_extractor, SIGNAL(triggered(bool)), this, SLOT(dzipExtractor()));
    QObject::connect(_ui->action_TW2_Materials_explorer, SIGNAL(triggered(bool)), this, SLOT(matExplorer()));
    QObject::connect(_ui->action_TW2_Show_linked_files, SIGNAL(triggered()), this, SLOT(extFiles()));

    QObject::connect(_ui->action_TW3_BUNDLE_extractor, SIGNAL(triggered(bool)), this, SLOT(bundleExtractor()));
    QObject::connect(_ui->action_TW3_Load_rig, SIGNAL(triggered(bool)), this, SLOT(selectRigFile()));
    QObject::connect(_ui->action_TW3_Load_animations, SIGNAL(triggered(bool)), this, SLOT(selectAnimationsFile()));
    QObject::connect(_ui->action_TW3_Materials_explorer, SIGNAL(triggered(bool)), this, SLOT(matExplorer()));
    QObject::connect(_ui->action_TW3_Show_linked_files, SIGNAL(triggered()), this, SLOT(extFiles()));
    QObject::connect(_ui->action_TW3_LUA_utils_Clean_textures_path_depreciated, SIGNAL(triggered()), this, SLOT(cleanTexturesPath()));

    QObject::connect(_ui->action_display_Wireframe, SIGNAL(triggered(bool)), this, SLOT(changeWireframe(bool)));
    QObject::connect(_ui->action_display_Rigging, SIGNAL(triggered(bool)), this, SLOT(changeRigging(bool)));

    QObject::connect(_ui->action_help_Webpage, SIGNAL(triggered()), this, SLOT(openWebpage()));


    // UI
    QObject::connect(_ui->button_fileSelector, SIGNAL(clicked()), this, SLOT(selectMeshFile()));
    QObject::connect(_ui->button_convert, SIGNAL(clicked()), this, SLOT(convertir()));
    QObject::connect(_ui->button_selectDir, SIGNAL(clicked()), this, SLOT(selectFolder()));
    QObject::connect(_ui->lineEdit_folder, SIGNAL(textChanged(QString)), this, SLOT(changeBaseDir(QString)));
    QObject::connect(_ui->comboBox_format, SIGNAL(currentTextChanged(QString)), this, SLOT(checkConvertButton()));

    for (int i = 0; i < _ui->menuLanguages->actions().size(); i++)
        QObject::connect(_ui->menuLanguages->actions().at(i), SIGNAL(triggered()), this, SLOT(changeLanguage()));

    // Logs
    _ui->textEdit_log->setReadOnly (true);
}

void GUI_MainWindow::addToUILog(QString log)
{
    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + log);

    QString logLogFile = "LOG UI LEVEL : " + log;
    Log::Instance()->addAndFlush(logLogFile.toStdString().c_str());
}

void GUI_MainWindow::addMesh()
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

        addToUILog(Translator::get("log_readingFile") + " '" + file + "'... ");

        core::stringc feedbackMessage;

        if (_irrWidget->isEmpty(_currentLOD))
            _irrWidget->setModel(file, feedbackMessage);
        else
            _irrWidget->addMesh(file, feedbackMessage);

        addToUILog(QString(feedbackMessage.c_str()) + "\n");
    }

    if (!_irrWidget->isEmpty(_currentLOD))
    {
        _irrWidget->changeWireframe(_ui->action_display_Wireframe->isChecked());
        _irrWidget->changeRigging(_ui->action_display_Rigging->isChecked());

        _ui->button_convert->setEnabled(true);
        _ui->action_redkit_Size->setEnabled(true);

        switch(_currentLOD)
        {
            case LOD_0:
                _ui->action_redkit_LOD0->setText("LOD0");
            break;
            case LOD_1:
                _ui->action_redkit_LOD1->setText("LOD1");
            break;
            case LOD_2:
                _ui->action_redkit_LOD2->setText("LOD2");
            break;
            case Collision:
                _ui->action_redkit_Collision_mesh->setText("Collision mesh");
            break;
        }
    }

    updateWindowTitle();
}

void GUI_MainWindow::changeBaseDir(QString newPath)
{
    Settings::_pack0 = newPath;
}

void GUI_MainWindow::updateWindowTitle()
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


void GUI_MainWindow::changeLanguage()
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


void GUI_MainWindow::cleanTexturesPath()
{
    QDir dir(Settings::_TW3TexPath);
    if (dir.isReadable())
    {
        GUI_CleanTexturesPath* w = new GUI_CleanTexturesPath (Settings::_TW3TexPath, this);
        w->show();
        w->clean();
        delete w;
    }
    else
        QMessageBox::critical(this, "Error", "TW3 textures folder can't be opened. Check the tuto for more informations.");
}

GUI_MainWindow::~GUI_MainWindow()
{
    Settings::saveToXML(QCoreApplication::applicationDirPath() + "/config.xml");

    // Delete UI
    delete _ui;

    if (_irrWidget)
        delete _irrWidget;
}



void GUI_MainWindow::initIrrlicht()
{
    _irrWidget = new QIrrlichtWidget(this);
    _ui->layout_irrlichtRender->addWidget(_irrWidget);

    _irrWidget->show();
    _irrWidget->init();
    addToUILog(QString("The Witcher 3D models converter ") + Settings::getAppVersion() + "\n");
}

void GUI_MainWindow::selectMeshFile()
{
    QString param = _ui->lineEdit_ImportedFile->text();
    if (_firstSelection)
        param = _ui->lineEdit_folder->text();

    QString file = QFileDialog::getOpenFileName(this, Translator::get("dialogue_file"), param, Settings::_formats);

    if (file != "")
    {
        loadMesh(file);
    }
}

void GUI_MainWindow::selectRigFile()
{
    QString file = QFileDialog::getOpenFileName(this, "Select the w2rig file to use", Settings::_pack0, "The Witcher 3 rig (*.w2rig , *.w3fac)");

    if (file != "")
        loadRig(file);
}

void GUI_MainWindow::selectAnimationsFile()
{
    QString file = QFileDialog::getOpenFileName(this, "Select the w2anims file to use", Settings::_pack0, "The Witcher 3 animations (*.w2anims)");

    if (file != "")
        loadAnimations(file);
}


void GUI_MainWindow::convertir()
{
    // Warning if no filename specified
    if (_ui->lineEdit_exportedFilename->text() == "")
        addToUILog(Translator::get("log_warning_empty") + "\n");

    addToUILog(Translator::get("log_writingFile") + " '" + _ui->lineEdit_exportedFilename->text()+ _ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).left(_ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).indexOf(' ')) + "'... ");
    QCoreApplication::processEvents();

    core::stringc feedback = "";

    // Check if the destination folder exist
    QDir dir(Settings::getExportFolder());
    if (dir.exists())
        _irrWidget->writeFile(Settings::getExportFolder(), _ui->lineEdit_exportedFilename->text(), _ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).left(_ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).indexOf(' ')), feedback);
    else
    {
        QMessageBox::warning(this, "Error", "The destination folder '" + Settings::_exportDest + "' doesn't exist.");
        addToUILog("\n" + Translator::get("log_abort") + "\n");
        return;
    }

    addToUILog(QString(feedback.c_str()) + "\n");
}

void GUI_MainWindow::translate()
{
    _ui->button_fileSelector->setText(Translator::get("button_fileSelector"));
    _ui->button_convert->setText(Translator::get("button_convertir"));
    _ui->label_baseDir->setText(Translator::get("base_directory") + " :");
    _ui->label_exportedFilename->setText(Translator::get("label_exported_file_name") + " :");

    // Menus
    _ui->menuMenu->setTitle(Translator::get("menu_menu"));
    _ui->action_main_Search->setText(Translator::get("menu_menu_search"));
    _ui->action_main_Options->setText(Translator::get("menu_menu_options"));
    _ui->action_main_Add_mesh->setText(Translator::get("menu_menu_addMesh"));
    _ui->action_main_Quitter->setText(Translator::get("menu_menu_quit"));

    _ui->menu_RE_tools->setTitle(Translator::get("menu_re"));
    _ui->action_redkit_Clear_current_LOD->setText(Translator::get("menu_re_lod_clear"));
    _ui->action_redkit_Clear_all_LODs->setText(Translator::get("menu_re_lod_clear_all"));
    _ui->action_redkit_Size->setText(Translator::get("menu_re_size"));

    _ui->action_TW1_BIF_extractor->setText(Translator::get("menu_tw1_bif"));

    _ui->action_TW2_Show_linked_files->setText(Translator::get("menu_tw2_3_linkedFiles"));
    _ui->action_TW2_Materials_explorer->setText(Translator::get("menu_tw2_3_materialsExplorer"));

    _ui->action_TW3_Load_rig->setText(Translator::get("menu_tw3_setRig"));
    _ui->action_TW3_Load_animations->setText(Translator::get("menu_tw3_setAnimation"));
    _ui->action_TW3_Show_linked_files->setText(Translator::get("menu_tw2_3_linkedFiles"));
    _ui->action_TW3_Materials_explorer->setText(Translator::get("menu_tw2_3_materialsExplorer"));
    _ui->action_TW3_LUA_utils_Clean_textures_path_depreciated->setText(Translator::get("menu_tw3_tex_path"));

    _ui->menuLanguages->setTitle(Translator::get("menu_language"));

    _ui->menuDisplay->setTitle(Translator::get("menu_display"));
    _ui->action_display_Wireframe->setText(Translator::get("menu_display_wireframe"));

    _ui->menuHelp->setTitle(Translator::get("menu_help"));
    _ui->action_help_Webpage->setText(Translator::get("menu_help_webpage"));


    if (_ui->action_redkit_LOD0->text() != "LOD0")
        _ui->action_redkit_LOD0->setText("LOD0 (" + Translator::get("menu_re_lod_empty") + ")");
    if (_ui->action_redkit_LOD1->text() != "LOD1")
        _ui->action_redkit_LOD1->setText("LOD1 (" + Translator::get("menu_re_lod_empty") + ")");
    if (_ui->action_redkit_LOD2->text() != "LOD2")
        _ui->action_redkit_LOD2->setText("LOD2 (" + Translator::get("menu_re_lod_empty") + ")");
}

void GUI_MainWindow::selectFolder()
{
    QString translation = Translator::get("dialogue_folder");
    QString folder = QFileDialog::getExistingDirectory(this, translation, _ui->lineEdit_folder->text());
    if (folder != "")
    {
        _ui->lineEdit_folder->setText(folder);
    }
}

void GUI_MainWindow::changeWireframe(bool enable)
{
    _irrWidget->changeWireframe(enable);
}

void GUI_MainWindow::changeRigging(bool enable)
{
    _irrWidget->changeRigging(enable);
}

void GUI_MainWindow::openWebpage()
{
    QString link = "http://jlouisb.users.sourceforge.net/";
    QDesktopServices::openUrl(QUrl(link));
}

void GUI_MainWindow::options()
{
    GUI_Options *w = new GUI_Options (this, _irrWidget->getPath(), _irrWidget);
    w->show();
    QObject::connect(w, SIGNAL(optionsValidation()), this, SLOT(changeOptions()));
}

void GUI_MainWindow::search()
{
    GUI_Search* w = new GUI_Search (this);
    w->show();
    QObject::connect(w, SIGNAL(loadPressed(QString)), this, SLOT(loadFileGeneric(QString)));
    QObject::connect(this, SIGNAL(languageChanged()), w, SLOT(translate()));
}

void GUI_MainWindow::matExplorer()
{
    GUI_MaterialsExplorer* m = new GUI_MaterialsExplorer(this, _irrWidget, _ui->lineEdit_ImportedFile->text());
    m->show();
}

void GUI_MainWindow::changeOptions()
{
    _irrWidget->changeOptions();
}

void GUI_MainWindow::re_size()
{
    GUI_Resize* r = new GUI_Resize(this);
    r->show();
}

void GUI_MainWindow::changeLOD()
{    
    for (int i = 0; i < _ui->menuLOD->actions().size(); i++)
        _ui->menuLOD->actions().at(i)->setChecked(false);

    QAction* q = (QAction*)QObject::sender();
    q->setChecked(true);

    LOD newlod = LOD_0;
    if (q == _ui->action_redkit_LOD0)
        newlod = LOD_0;
    else if (q == _ui->action_redkit_LOD1)
        newlod = LOD_1;
    else if (q == _ui->action_redkit_LOD2)
        newlod = LOD_2;
    else if (q == _ui->action_redkit_Collision_mesh)
        newlod = Collision;

    _irrWidget->changeLOD(newlod);
    _currentLOD = newlod;

    checkConvertButton();
    updateWindowTitle();
}

void GUI_MainWindow::clearLOD()
{
    switch(_currentLOD)
    {
        case LOD_0:
            _ui->action_redkit_LOD0->setText("LOD0 (" + Translator::get("re_lod_empty") + ")");
        break;
        case LOD_1:
            _ui->action_redkit_LOD1->setText("LOD1 (" + Translator::get("re_lod_empty") + ")");
        break;
        case LOD_2:
            _ui->action_redkit_LOD2->setText("LOD2 (" + Translator::get("re_lod_empty") + ")");
        break;
        case Collision:
            _ui->action_redkit_LOD2->setText("Collision mesh (" + Translator::get("re_lod_empty") + ")");
        break;
    }

    _irrWidget->clearLOD();

    _ui->action_redkit_Size->setEnabled(false);

    checkConvertButton();
    updateWindowTitle();
}

void GUI_MainWindow::clearAllLODs()
{
    _ui->action_redkit_LOD0->setText("LOD0 (" + Translator::get("re_lod_empty") + ")");
    _ui->action_redkit_LOD1->setText("LOD1 (" + Translator::get("re_lod_empty") + ")");
    _ui->action_redkit_LOD2->setText("LOD2 (" + Translator::get("re_lod_empty") + ")");

    _irrWidget->clearAllLODs();

    _ui->action_redkit_Size->setEnabled(false);

    checkConvertButton();
    updateWindowTitle();
}

void GUI_MainWindow::checkConvertButton()
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

void GUI_MainWindow::extFiles()
{
    GUI_ExtFilesExplorer* ext = new GUI_ExtFilesExplorer(_irrWidget, this);
    ext->read(_ui->lineEdit_ImportedFile->text());
    ext->show();
}

void GUI_MainWindow::bifExtractor()
{
    GUI_Extractor_TW1_BIF* bif = new GUI_Extractor_TW1_BIF(this);
    bif->show();
}

void GUI_MainWindow::dzipExtractor()
{
    GUI_Extractor_TW2_DZIP* dzip = new GUI_Extractor_TW2_DZIP(this);
    dzip->show();
}

void GUI_MainWindow::bundleExtractor()
{
    GUI_Extractor_TW3_BUNDLE* bundle = new GUI_Extractor_TW3_BUNDLE(this);
    bundle->show();
}

void GUI_MainWindow::loadFileGeneric(QString path)
{
    const io::path filenamePath = QSTRING_TO_PATH(path);
    io::IReadFile* file = _irrWidget->getFileSystem()->createAndOpenFile(filenamePath);

    WitcherFileType type = getTWFileType(file);
    WitcherContentType contentType = getTWFileContentType(filenamePath);
    if (file)
        file->drop();

    if (type == WFT_UNKNOWN)
        loadMesh(path);
    else if (contentType == WCT_WITCHER_RIG)
    {
        if (type == WFT_WITCHER_3)
            loadRig(path);
        else
            QMessageBox::warning(0, "Load fail", "The Witcher 2 rig files not supported");
    }
    else if (contentType == WCT_WITCHER_ANIMATIONS)
    {
        if (type == WFT_WITCHER_3)
            loadAnimations(path);
        else
            QMessageBox::warning(0, "Load fail", "The Witcher 2 animations files not supported");
    }
    else
        loadMesh(path);
}

void GUI_MainWindow::loadMesh(QString path)
{
    if (!_irrWidget->fileIsOpenableByIrrlicht(path))
    {
        QMessageBox::critical(this, "Error", "Error : The file can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
        return;
    }

    addToUILog(Translator::get("log_readingFile") + " '" + path + "'... ");
    _ui->lineEdit_ImportedFile->setText(path);
    QCoreApplication::processEvents();

    core::stringc feedbackMessage = "";
    bool success = _irrWidget->setModel(path, feedbackMessage);
    if (success)
    {
        _irrWidget->changeWireframe(_ui->action_display_Wireframe->isChecked());
        _irrWidget->changeRigging(_ui->action_display_Rigging->isChecked());

        _ui->button_convert->setEnabled(true);
        _ui->action_redkit_Size->setEnabled(true);

        switch(_currentLOD)
        {
            case LOD_0:
                _ui->action_redkit_LOD0->setText("LOD0");
            break;
            case LOD_1:
                _ui->action_redkit_LOD1->setText("LOD1");
            break;
            case LOD_2:
                _ui->action_redkit_LOD2->setText("LOD2");
            break;
            case Collision:
                _ui->action_redkit_Collision_mesh->setText("Collision mesh");
            break;
        }
        _firstSelection = false;
    }

    addToUILog(QString(feedbackMessage.c_str()) + "\n");
    updateWindowTitle();
}

void GUI_MainWindow::loadRig(QString path)
{
    if (!_irrWidget->fileIsOpenableByIrrlicht(path))
    {
        QMessageBox::critical(this, "Error", "Error : The file can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
        return;
    }
    addToUILog(Translator::get("log_readingFile") + " '" + path + "'... ");

    core::stringc feedback;
    bool sucess = _irrWidget->loadRig(QSTRING_TO_PATH(path), feedback);

    if (sucess)
        QMessageBox::information(this, "Sucess", feedback.c_str());
    else
        QMessageBox::critical(this, "Error", feedback.c_str());

    addToUILog(QString(feedback.c_str()) + "\n");
    updateWindowTitle();
}

void GUI_MainWindow::loadAnimations(QString path)
{
    if (!_irrWidget->fileIsOpenableByIrrlicht(path))
    {
        QMessageBox::critical(this, "Error", "Error : The file can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
        return;
    }
    addToUILog(Translator::get("log_readingFile") + " '" + path + "'... ");

    core::stringc feedback;
    bool sucess = _irrWidget->loadAnims(QSTRING_TO_PATH(path), feedback);

    addToUILog(QString(feedback.c_str()) + "\n");
    updateWindowTitle();
}
