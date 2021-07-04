#include "GUI_MainWindow.h"
#include "ui_GUI_MainWindow.h"

#include <iostream>

#include <QFileDialog>
#include <QDir>
#include <QCheckBox>
#include <QMessageBox>

#include "GUI_Search.h"
#include "GUI_Options.h"
#include "GUI_Resize.h"
#include "GUI_CleanTexturesPath.h"
#include "GUI_Extractor_TW1_BIF.h"
#include "GUI_Extractor_TW2_DZIP.h"
#include "GUI_Extractor_TW3_CACHE.h"
#include "GUI_Extractor_TW3_BUNDLE.h"
#include "GUI_Extractor_TheCouncil.h"
#include "GUI_Extractor_Dishonored2.h"
#include "GUI_MaterialsExplorer.h"
#include "GUI_ExtFilesExplorer.h"
#include "GUI_About.h"
#include "Translator.h"
#include "Utils_Qt.h"


GUI_MainWindow::GUI_MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _ui(new Ui::GUI_MainWindow)
{
    createUILogger();

    // setup the UI
    _ui->setupUi(this);
    _ui->textEdit_log->setReadOnly (true);

    _ui->lineEdit_folder->setText(Settings::_baseDir);

    WindowState window = Settings::_windowState;
    if (window._initialised)
    {
        restoreGeometry(window._geometry);
    }

    // Register and set current exporter
    registerExporters();
    if (!Settings::_exporter.isEmpty())
    {
        _ui->comboBox_exportFormat->setCurrentText(Settings::_exporter);
    }

    // Other settings
    _firstSelection = true;
    _currentLOD = LOD_0;

    // Search the translations files
    QDir dir(QCoreApplication::applicationDirPath() + "/langs/");

    QStringList filter;
    filter << "*.xml";
    dir.setNameFilters(filter);

    QStringList filenames = dir.entryList();
    foreach( QString filename, filenames )
    {
        QAction* menuitem = new QAction(filename, _ui->menu_Languages);
        menuitem->setCheckable(true);
        if ("langs/" + menuitem->text() == Settings::_language)
            menuitem->setChecked(true);
        _ui->menu_Languages->addAction(menuitem);
    }

    // Translator
    Translator::setParentWidget(this);
    Translator::loadCurrentLanguage();
    translate();

    // First use message
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
    QObject::connect(_ui->action_main_Add_mesh, SIGNAL(triggered()), this, SLOT(onAddMeshClicked()));
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
    QObject::connect(_ui->action_TW1_Load_animations, SIGNAL(triggered(bool)), this, SLOT(selectTW1AnimationsFile()));

    QObject::connect(_ui->action_TW2_DZIP_extractor, SIGNAL(triggered(bool)), this, SLOT(dzipExtractor()));
    QObject::connect(_ui->action_TW2_Materials_explorer, SIGNAL(triggered(bool)), this, SLOT(matExplorer()));
    QObject::connect(_ui->action_TW2_Show_linked_files, SIGNAL(triggered()), this, SLOT(extFiles()));

    QObject::connect(_ui->action_TW3_BUNDLE_extractor, SIGNAL(triggered(bool)), this, SLOT(bundleExtractor()));
    QObject::connect(_ui->action_TW3_Load_rig, SIGNAL(triggered(bool)), this, SLOT(selectRigFile()));
    QObject::connect(_ui->action_TW3_Load_animations, SIGNAL(triggered(bool)), this, SLOT(selectAnimationsFile()));
    QObject::connect(_ui->action_TW3_Materials_explorer, SIGNAL(triggered(bool)), this, SLOT(matExplorer()));
    QObject::connect(_ui->action_TW3_Show_linked_files, SIGNAL(triggered()), this, SLOT(extFiles()));
    QObject::connect(_ui->action_TW3_LUA_utils_Clean_textures_path_depreciated, SIGNAL(triggered()), this, SLOT(cleanTexturesPath()));

    QObject::connect(_ui->action_other_TheCouncil_unpacker, SIGNAL(triggered()), this, SLOT(thecouncilExtractor()));
    QObject::connect(_ui->action_other_TheCouncil_Load_template, SIGNAL(triggered()), this, SLOT(selectTheCouncilTemplate()));
    QObject::connect(_ui->action_other_Dishonored_2_unpacker, SIGNAL(triggered()), this, SLOT(dishonoredExtractor()));
    deleteInReleaseBuild(_ui->action_other_Dishonored_2_unpacker);

    QObject::connect(_ui->action_help_About, SIGNAL(triggered()), this, SLOT(openAbout()));


    // UI
    QObject::connect(_ui->button_fileSelector, SIGNAL(clicked()), this, SLOT(onLoadMeshClicked()));
    QObject::connect(_ui->button_convert, SIGNAL(clicked()), this, SLOT(convert()));
    QObject::connect(_ui->button_selectDir, SIGNAL(clicked()), this, SLOT(selectBaseDir()));
    QObject::connect(_ui->lineEdit_folder, SIGNAL(textChanged(QString)), this, SLOT(changeBaseDir(QString)));
    QObject::connect(_ui->comboBox_exportFormat, SIGNAL(currentTextChanged(QString)), this, SLOT(checkConvertButton()));

    for (int i = 0; i < _ui->menu_Languages->actions().size(); i++)
        QObject::connect(_ui->menu_Languages->actions().at(i), SIGNAL(triggered()), this, SLOT(changeLanguage()));
}

GUI_MainWindow::~GUI_MainWindow()
{
    if (_ui->comboBox_exportFormat->currentIndex() != -1)
        Settings::_exporter = _ui->comboBox_exportFormat->currentText();

    WindowState window;
    window._geometry = saveGeometry();
    Settings::_windowState = window;

    Settings::saveToXML(QCoreApplication::applicationDirPath() + "/config.xml");

    // Delete UI
    delete _ui;

    if (_irrWidget)
        delete _irrWidget;

    destroyUILogger();
}

void GUI_MainWindow::createUILogger()
{
    _UILogger = new CallbackLogger();
    _UILogger->registerLogToUserCallback(std::bind(&GUI_MainWindow::logToUser, this, std::placeholders::_1));
    LoggerManager::Instance()->registerLogger(_UILogger, Logger_User);
}

void GUI_MainWindow::destroyUILogger()
{
    if (_UILogger)
    {
        LoggerManager::Instance()->unregisterLogger(_UILogger);
        delete _UILogger;
        _UILogger = nullptr;
    }
}

void GUI_MainWindow::logToUser(core::stringc log)
{
    QString qLog = irrStringToQString(log);

    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + qLog);
}


void GUI_MainWindow::onAddMeshClicked()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, "Select the file(s) to load", Settings::_baseDir, Settings::getFilters(), &Settings::_selectedFilter);
    if (filePaths.size() > 0)
    {
        addMeshes(filePaths);
    }
}

void GUI_MainWindow::addMeshes(QStringList filePaths)
{
    for (int i = 0; i < filePaths.size(); ++i)
    {
        const QString filePath = filePaths.at(i);
        if (!_irrWidget->fileIsOpenableByIrrlicht(filePath))
        {
            QMessageBox::critical(this, "Error", "Error : The file " + filePath + " can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
            continue;
        }

        LoggerManager::Instance()->addAndFlush(qStringToIrrString(QString("Read '") + filePath + "'... "), true);

        if (_irrWidget->isEmpty(_currentLOD))
            _irrWidget->loadAndReplaceMesh(filePath);
        else
            _irrWidget->addMesh(filePath);
    }

    if (!_irrWidget->isEmpty(_currentLOD))
    {
        _irrWidget->enableWireframe(_ui->action_display_Wireframe->isChecked());
        _irrWidget->enableRigging(_ui->action_display_Rigging->isChecked());

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
                _ui->action_redkit_Collision_mesh->setText("Convex collision mesh");
            break;
        }
    }
    updateWindowTitle();
}

void GUI_MainWindow::changeBaseDir(QString newPath)
{
    Settings::_baseDir = newPath;
}

void GUI_MainWindow::updateWindowTitle()
{
    QString title = "The Witcher 3D models converter";

    QString filename = _irrWidget->getFilename();
    if (filename.isEmpty())
        filename = "3D Model";

    if (!_irrWidget->isEmpty(_currentLOD))
    {
        title += " - " + filename + " - " + QString::number(_irrWidget->getPolysCount()) + " polys - " + QString::number(_irrWidget->getJointsCount()) + " joints";
    }

    this->setWindowTitle(title);
}


void GUI_MainWindow::changeLanguage()
{
    for (int i = 0; i < _ui->menu_Languages->actions().size(); i++)
        _ui->menu_Languages->actions().at(i)->setChecked(false);

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

void GUI_MainWindow::initIrrlicht()
{
    _irrWidget = new QIrrlichtWidget(this);
    _ui->layout_irrlichtRender->addWidget(_irrWidget);

    _irrWidget->show();
    _irrWidget->init();

    //LoggerManager::Instance()->registerLogToUserCallback(std::bind(&GUI_MainWindow::logToUser, this, std::placeholders::_1));

    QObject::connect(_ui->action_display_Wireframe, SIGNAL(triggered(bool)), _irrWidget, SLOT(enableWireframe(bool)));
    QObject::connect(_ui->action_display_Rigging, SIGNAL(triggered(bool)), _irrWidget, SLOT(enableRigging(bool)));
    QObject::connect(_ui->action_display_Normals, SIGNAL(triggered(bool)), _irrWidget, SLOT(enableNormals(bool)));

    LoggerManager::Instance()->addLineAndFlush(qStringToIrrString(QString("The Witcher 3D models converter ") + Settings::getAppVersion()), true);
}

void GUI_MainWindow::registerExporters()
{
    _ui->comboBox_exportFormat->clear();
    _exporters.clear();

    _exporters.push_back({ Exporter_Irrlicht, ".obj (Wavefront OBJ)"                , ".obj"     , ""    , IrrlichtExporterInfos(scene::EMWT_OBJ       , scene::EMWF_NONE)});
    _exporters.push_back({ Exporter_Irrlicht, ".dae (Collada)"                      , ".dae"     , ""    , IrrlichtExporterInfos(scene::EMWT_COLLADA   , scene::EMWF_NONE)});
    _exporters.push_back({ Exporter_Irrlicht, ".ply (Polygon File Format (ascii))"  , ".ply"     , ""    , IrrlichtExporterInfos(scene::EMWT_PLY       , scene::EMWF_NONE)});
    _exporters.push_back({ Exporter_Irrlicht, ".ply (Polygon File Format (binary))" , ".ply"     , ""    , IrrlichtExporterInfos(scene::EMWT_PLY       , scene::EMWF_WRITE_BINARY)});
    _exporters.push_back({ Exporter_Irrlicht, ".stl (STereoLithography (ascii))"    , ".stl"     , ""    , IrrlichtExporterInfos(scene::EMWT_STL       , scene::EMWF_NONE)});
    _exporters.push_back({ Exporter_Irrlicht, ".stl (STereoLithography (binary))"   , ".stl"     , ""    , IrrlichtExporterInfos(scene::EMWT_STL       , scene::EMWF_WRITE_BINARY)});
    _exporters.push_back({ Exporter_Irrlicht, ".irrmesh (Irrlicht mesh)"            , ".irrmesh" , ""    , IrrlichtExporterInfos(scene::EMWT_IRR_MESH  , scene::EMWF_NONE)});
    _exporters.push_back({ Exporter_Irrlicht, ".b3d (Blitz3D)"                      , ".b3d"     , ""    , IrrlichtExporterInfos(scene::EMWT_B3D       , scene::EMWF_NONE)});
    _exporters.push_back({ Exporter_Redkit  , ".re (Red Engine)"                    , ".re"      , ""    , IrrlichtExporterInfos(scene::EMWT_OBJ       , scene::EMWF_NONE)});

    QVector<QString> noAssimpExportExtensions;
    for (int i = 0; i < _exporters.size(); ++i)
    {
        const QString extension = _exporters[i]._extension;
        noAssimpExportExtensions.push_back(extension);
    }

    core::array<ExportFormat> formats = IrrAssimp::getExportFormats();
    for (u32 i = 0; i < formats.size(); ++i)
    {
        const ExportFormat format = formats[i];
        const QString extension = QString(".") + format.fileExtension.c_str();
        if (noAssimpExportExtensions.indexOf(extension) == -1)
        {
            const QString exportString = extension + " by Assimp library (" + format.description.c_str() + ")";
            _exporters.push_back({ Exporter_Assimp, exportString, extension, format.id.c_str(), IrrlichtExporterInfos()});
        }
    }

    for (int i = 0; i < _exporters.size(); ++i)
    {
        _ui->comboBox_exportFormat->addItem(_exporters[i]._exporterName);
    }
}

void GUI_MainWindow::onLoadMeshClicked()
{
    QString defaultFolder = _ui->lineEdit_ImportedFile->text();
    if (_firstSelection)
        defaultFolder = _ui->lineEdit_folder->text();

    QString filePath = QFileDialog::getOpenFileName(this, "Select the file to load", defaultFolder, Settings::getFilters(), &Settings::_selectedFilter);
    if (!filePath.isEmpty())
    {
        replaceMesh(filePath);
    }
}

void GUI_MainWindow::selectRigFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select the w2rig file to load", Settings::_baseDir, "The Witcher 3 rig (*.w2rig , *.w3fac)");
    if (!filePath.isEmpty())
    {
        loadRig(filePath);
    }
}

void GUI_MainWindow::selectAnimationsFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select the w2anims file to load", Settings::_baseDir, "The Witcher 3 animations (*.w2anims)");
    if (!filePath.isEmpty())
    {
        loadAnimations(filePath);
    }
}

void GUI_MainWindow::selectTW1AnimationsFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select the MBA file to load", Settings::_baseDir, "The Witcher 1 animations (*.mba)");
    if (!filePath.isEmpty())
    {
        loadTW1Animations(filePath);
    }
}

void GUI_MainWindow::selectTheCouncilTemplate()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select the template file to load", Settings::_baseDir, "The Council template (*.template)");
    if (!filePath.isEmpty())
    {
        loadTheCouncilTemplate(filePath);
    }
}

void GUI_MainWindow::convert()
{
    if (_ui->lineEdit_exportedFilename->text().isEmpty())
    {
        LoggerManager::Instance()->addLineAndFlush("Warning : The name of the exported file is empty", true);
    }

    // Get the exporter and log the infos
    int currentIndex = _ui->comboBox_exportFormat->currentIndex();
    if (currentIndex == -1) // not supposed to happen
    {
        LoggerManager::Instance()->addLineAndFlush("Invalid exporter", true);
        return;
    }
    const ExporterInfos infos = _exporters[currentIndex];
    LoggerManager::Instance()->addAndFlush(qStringToIrrString("Writing file '" + _ui->lineEdit_exportedFilename->text() + infos._extension + "'... "), true);
    QCoreApplication::processEvents();

    // Check if the destination folder exist
    QDir dir(Settings::getExportFolder());
    if (dir.exists())
    {
        _irrWidget->exportMesh(Settings::getExportFolder(), _ui->lineEdit_exportedFilename->text(), infos);
    }
    else
    {
        QMessageBox::warning(this, "Error", "The destination folder '" + Settings::_exportDest + "' doesn't exist.");
        LoggerManager::Instance()->addLineAndFlush("\nAbort : Destination folder doesn't exist", true);
    }
}

void GUI_MainWindow::translate()
{
    _ui->button_fileSelector->setText(Translator::get("button_fileSelector"));
    _ui->button_convert->setText(Translator::get("button_convertir"));
    _ui->label_baseDir->setText(Translator::get("base_directory") + " :");
    _ui->label_exportedFilename->setText(Translator::get("label_exported_file_name") + " :");

    // Menus
    _ui->action_main_Search->setText(Translator::get("menu_menu_search"));
    _ui->action_main_Add_mesh->setText(Translator::get("menu_menu_addMesh"));
    _ui->action_main_Quitter->setText(Translator::get("menu_menu_quit"));

    _ui->menu_Redkit_tools->setTitle(Translator::get("menu_re"));
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

    _ui->menu_Languages->setTitle(Translator::get("menu_language"));

    _ui->menu_Display->setTitle(Translator::get("menu_display"));
    _ui->action_display_Wireframe->setText(Translator::get("menu_display_wireframe"));

    _ui->menu_Help->setTitle(Translator::get("menu_help"));


    if (_ui->action_redkit_LOD0->text() != "LOD0")
        _ui->action_redkit_LOD0->setText("LOD0 (" + Translator::get("menu_re_lod_empty") + ")");
    if (_ui->action_redkit_LOD1->text() != "LOD1")
        _ui->action_redkit_LOD1->setText("LOD1 (" + Translator::get("menu_re_lod_empty") + ")");
    if (_ui->action_redkit_LOD2->text() != "LOD2")
        _ui->action_redkit_LOD2->setText("LOD2 (" + Translator::get("menu_re_lod_empty") + ")");
}

void GUI_MainWindow::selectBaseDir()
{
    QString baseDirPath = QFileDialog::getExistingDirectory(this, "Select the base directory (root of the unpacked files)", _ui->lineEdit_folder->text());
    if (!baseDirPath.isEmpty())
    {
        _ui->lineEdit_folder->setText(baseDirPath);
    }
}

void GUI_MainWindow::openAbout()
{
    GUI_About* about = new GUI_About(this);
    about->show();
}

void GUI_MainWindow::options()
{
    GUI_Options *w = new GUI_Options(this);
    w->show();
    QObject::connect(w, SIGNAL(optionsValidation()), this, SLOT(changeOptions()));
}

void GUI_MainWindow::search()
{
    GUI_Search* w = new GUI_Search(this);
    w->show();
    QObject::connect(w, SIGNAL(loadPressed(QString)), this, SLOT(loadFileGeneric(QString)));
    QObject::connect(this, SIGNAL(languageChanged()), w, SLOT(translate()));
}

void GUI_MainWindow::matExplorer()
{
    GUI_MaterialsExplorer* m = new GUI_MaterialsExplorer(this, _irrWidget->getFileSystem(), _ui->lineEdit_ImportedFile->text());
    m->show();
}

void GUI_MainWindow::changeOptions()
{
    _irrWidget->changeOptions();
}

void GUI_MainWindow::re_size()
{
    GUI_Resize* r = new GUI_Resize(this);
    r->setMeshOriginalDimensions(_irrWidget->getMeshDimensions());
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
            _ui->action_redkit_LOD2->setText("Convex collision mesh (" + Translator::get("re_lod_empty") + ")");
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
    const int currentIndex = _ui->comboBox_exportFormat->currentIndex();
    if (currentIndex == -1) // not supposed to happen
    {
        _ui->button_convert->setEnabled(false);
        return;
    }

    const ExporterInfos infos = _exporters[currentIndex];
    if (infos._extension == ".re")
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

void GUI_MainWindow::thecouncilExtractor()
{
    GUI_Extractor_TheCouncil* CPK = new GUI_Extractor_TheCouncil(this);
    CPK->show();
}

void GUI_MainWindow::dishonoredExtractor()
{
    GUI_Extractor_Dishonored2* dishonored = new GUI_Extractor_Dishonored2(this);
    dishonored->show();
}

void GUI_MainWindow::loadFileGeneric(QString path)
{
    const io::path filePath = qStringToIrrPath(path);
    io::IReadFile* file = _irrWidget->getFileSystem()->createAndOpenFile(filePath);

    RedEngineVersion type = getRedEngineFileType(file);
    RedEngineContentType contentType = getRedEngineFileContentType(filePath);
    if (file)
        file->drop();

    if (type == REV_UNKNOWN)
        replaceMesh(path);
    else if (contentType == RECT_WITCHER_RIG)
    {
        if (type == REV_WITCHER_3)
            loadRig(path);
        else
            QMessageBox::warning(nullptr, "Load fail", "The Witcher 2 rig files not supported");
    }
    else if (contentType == RECT_WITCHER_ANIMATIONS)
    {
        if (type == REV_WITCHER_3)
            loadAnimations(path);
        else
            QMessageBox::warning(nullptr, "Load fail", "The Witcher 2 animations files not supported");
    }
    else
        replaceMesh(path);
}

void GUI_MainWindow::addFileGeneric(QString path)
{
    const io::path filePath = qStringToIrrPath(path);
    io::IReadFile* file = _irrWidget->getFileSystem()->createAndOpenFile(filePath);

    RedEngineVersion type = getRedEngineFileType(file);
    RedEngineContentType contentType = getRedEngineFileContentType(filePath);
    if (file)
        file->drop();

    if (type == REV_UNKNOWN)
        addMeshes(QStringList(path));
    else if (contentType == RECT_WITCHER_RIG)
    {
        if (type == REV_WITCHER_3)
            loadRig(path);
        else
            QMessageBox::warning(nullptr, "Load fail", "The Witcher 2 rig files not supported");
    }
    else if (contentType == RECT_WITCHER_ANIMATIONS)
    {
        if (type == REV_WITCHER_3)
            loadAnimations(path);
        else
            QMessageBox::warning(nullptr, "Load fail", "The Witcher 2 animations files not supported");
    }
    else
        addMeshes(QStringList(path));
}



void GUI_MainWindow::replaceMesh(QString path)
{
    if (!_irrWidget->fileIsOpenableByIrrlicht(path))
    {
        QMessageBox::critical(this, "Error", "Error : The file can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
        return;
    }

    LoggerManager::Instance()->addAndFlush(qStringToIrrString(QString("Reading file '") + path + "'... "), true);
    _ui->lineEdit_ImportedFile->setText(path);
    QCoreApplication::processEvents();

    bool success = _irrWidget->loadAndReplaceMesh(path);
    if (success)
    {
        _irrWidget->enableWireframe(_ui->action_display_Wireframe->isChecked());
        _irrWidget->enableRigging(_ui->action_display_Rigging->isChecked());

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
                _ui->action_redkit_Collision_mesh->setText("Convex collision mesh");
            break;
        }
        _firstSelection = false;
    }

    logLoadingResult(success);

    updateWindowTitle();
}

void GUI_MainWindow::loadRig(QString path)
{
    if (!_irrWidget->fileIsOpenableByIrrlicht(path))
    {
        QMessageBox::critical(this, "Error", "Error : The file can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
        return;
    }
    LoggerManager::Instance()->addAndFlush(qStringToIrrString(QString("Reading file '") + path + "'... "), true);
    QCoreApplication::processEvents();

    bool success = _irrWidget->loadRig(qStringToIrrPath(path));
    logLoadingResult(success);

    updateWindowTitle();
}

void GUI_MainWindow::loadAnimations(QString path)
{
    if (!_irrWidget->fileIsOpenableByIrrlicht(path))
    {
        QMessageBox::critical(this, "Error", "Error : The file can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
        return;
    }
    LoggerManager::Instance()->addAndFlush(qStringToIrrString(QString("Reading file '") + path + "'... "), true);
    QCoreApplication::processEvents();

    bool success = _irrWidget->loadAnims(qStringToIrrPath(path));
    logLoadingResult(success);

    updateWindowTitle();
}

void GUI_MainWindow::loadTW1Animations(QString path)
{
    if (!_irrWidget->fileIsOpenableByIrrlicht(path))
    {
        QMessageBox::critical(this, "Error", "Error : The file can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
        return;
    }
    LoggerManager::Instance()->addAndFlush(qStringToIrrString(QString("Reading file '") + path + "'... "), true);
    QCoreApplication::processEvents();

    bool success = _irrWidget->loadTW1Anims(qStringToIrrPath(path));
    logLoadingResult(success);

    updateWindowTitle();
}

void GUI_MainWindow::loadTheCouncilTemplate(QString path)
{
    if (!_irrWidget->fileIsOpenableByIrrlicht(path))
    {
        QMessageBox::critical(this, "Error", "Error : The file can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
        return;
    }
    LoggerManager::Instance()->addAndFlush(qStringToIrrString(QString("Reading file '") + path + "'... "), true);
    QCoreApplication::processEvents();

    bool success = _irrWidget->loadTheCouncilTemplate(qStringToIrrPath(path));
    logLoadingResult(success);
}

void GUI_MainWindow::logLoadingResult(bool result)
{
    if (result)
    {
        LoggerManager::Instance()->addLineAndFlush("Loading finished", true);
    }
    else
    {
        LoggerManager::Instance()->addLineAndFlush("Loading failed", true);
    }
}
