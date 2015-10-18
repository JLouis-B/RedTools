#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
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
    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + "The Witcher 3D models converter 2.3 WIP\n");


    OptionsData::_pack0 = _ui->lineEdit_folder->text();
    _formats = "The Witcher 2 3D models (*.w2ent , *.w2mesh)";


    // Load config from XML
    QDomDocument *dom = new QDomDocument("config");
    QFile xml_doc("config.xml");

    if(!xml_doc.open(QIODevice::ReadOnly))
    {
         QMessageBox::warning(this, "Erreur", "Erreur XML");
    }

    if (!dom->setContent(&xml_doc))
    {
        xml_doc.close();
        QMessageBox::warning(this, "Erreur", "Erreur XML");
    }
    QDomElement dom_element = dom->documentElement();
    QDomNode node = dom_element.firstChildElement();
    while (!node.isNull())
    {
        if(node.nodeName() == "language")
            _language = node.toElement().text();

        if(node.nodeName() == "pack0")
        {
            _ui->lineEdit_folder->setText(node.toElement().text());
            OptionsData::_pack0 = node.toElement().text();
        }

        if(node.nodeName() == "camera")
        {
            OptionsData::_camSpeed = node.firstChildElement("speed").text().toDouble();
            OptionsData::_camRotSpeed = node.firstChildElement("rotation_speed").text().toDouble();
        }

        if(node.nodeName() == "background_color")
        {
            OptionsData::_r = node.firstChildElement("r").text().toInt();
            OptionsData::_g = node.firstChildElement("g").text().toInt();
            OptionsData::_b = node.firstChildElement("b").text().toInt();
        }
        if(node.nodeName() == "textures_conversion")
        {
            OptionsData::_convertTextures = node.firstChildElement("enabled").text().toInt();
            OptionsData::_texFormat = node.firstChildElement("format").text();
        }

        if(node.nodeName() == "export")
        {
            if (node.firstChildElement("type").text() == "pack0")
                OptionsData::_mode = Export_Pack0;
            else if (node.firstChildElement("type").text() == "custom")
                OptionsData::_mode = Export_Custom;

            OptionsData::_exportDest = node.firstChildElement("dest").text();

            OptionsData::_moveTexture = node.firstChildElement("move_textures").text().toInt();
            OptionsData::_nm = node.firstChildElement("move_normals_map").text().toInt();
            OptionsData::_sm = node.firstChildElement("move_specular_map").text().toInt();
        }
        if(node.nodeName() == "formats_in")
        {
            _formats = node.toElement().text();
        }
        if(node.nodeName() == "unit")
        {
            QString unit = node.toElement().text();
            if (unit == "m")
                ReSize::_unit = Unit_m;
            else if (unit == "cm")
                ReSize::_unit = Unit_cm;
        }
        if(node.nodeName() == "debug")
        {
            OptionsData::_debugLog = node.toElement().text().toInt();
        }

        node = node.nextSibling();
    }

    xml_doc.close();
    delete dom;

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
        if ("langs/" + menuitem->text() == _language)
            menuitem->setChecked(true);
        _ui->menuLanguages->addAction(menuitem);
    }

    // The parent widget if the translator has to popup an error message box
    Translator::setParentWidget(this);

    // Translate the UI
    translate();

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

    for (int i = 0; i < _ui->menuLanguages->actions().size(); i++)
        QObject::connect(_ui->menuLanguages->actions().at(i), SIGNAL(triggered()), this, SLOT(changeLanguage()));


}

void MainWindow::loadRig()
{
    QString file = QFileDialog::getOpenFileName(this, "Select the w2rig file to use", OptionsData::_pack0, "The Witcher rig (*.w2rig)");

    if (file == "")
        return;

    core::stringc feedback;
    bool sucess = _irrWidget->loadRig(file.toStdString().c_str(), feedback);

    if (sucess)
        QMessageBox::information(this, "Sucess", feedback.c_str());
    else
        QMessageBox::critical(this, "Error", feedback.c_str());

    updateWindowTitle();
}


void MainWindow::changeBaseDir(QString newPath)
{
    OptionsData::_pack0 = newPath;
}

void MainWindow::updateWindowTitle()
{
    QString title = "The Witcher 3D models converter";
    if (!_irrWidget->isEmpty(_currentLOD))
    {
        title += " - " + _irrWidget->getFilename() + " - " + QString::number(_irrWidget->getPolysCount()) + " polys - " + QString::number(_irrWidget->getJointsCount()) + " joints";
    }

    this->setWindowTitle(title);
}


void MainWindow::changeLanguage()
{
    for (int i = 0; i < _ui->menuLanguages->actions().size(); i++)
        _ui->menuLanguages->actions().at(i)->setChecked(false);

    QAction* q = (QAction*)QObject::sender();
    q->setChecked(true);
    _language = "langs/" + q->text();
    translate();
    emit languageChanged(_language);
}


void MainWindow::cleanTexturesPath()
{
    QDir dir(_ui->lineEdit_folder->text() + "/textures_unpack");
    if (dir.isReadable())
    {
        CleanTexturesPath* w = new CleanTexturesPath (_ui->lineEdit_folder->text() + "/textures_unpack", this);
        w->show();
        w->clean();
        delete w;
    }
    else
        QMessageBox::critical(this, "Error", "'textures_unpack' folder not found in the base directory. Check the tuto for more informations.");
}

MainWindow::~MainWindow()
{
    // Save the config in a xml
    // Create the DOM
    QDomDocument dom("config");
    //QDomElement docElem = dom.documentElement(); // the document element
    QDomElement config_elem = dom.createElement("config");
    dom.appendChild(config_elem);

    // language
    QDomElement lang_elem = dom.createElement("language");
    config_elem.appendChild(lang_elem);

    QDomText lang_txt = dom.createTextNode(_language);
    lang_elem.appendChild(lang_txt);


    // pack0 folder
    QDomElement pack0_elem = dom.createElement("pack0");
    config_elem.appendChild(pack0_elem);

    QDomText pack0_txt = dom.createTextNode(_ui->lineEdit_folder->text());
    pack0_elem.appendChild(pack0_txt);


    // backgroud color
    QDomElement background_elem = dom.createElement("background_color");
    config_elem.appendChild(background_elem);

    QDomElement br_elem = dom.createElement("r");
    background_elem.appendChild(br_elem);
    br_elem.appendChild(dom.createTextNode(QString::number(OptionsData::_r)));

    QDomElement bg_elem = dom.createElement("g");
    background_elem.appendChild(bg_elem);
    bg_elem.appendChild(dom.createTextNode(QString::number(OptionsData::_g)));

    QDomElement bb_elem = dom.createElement("b");
    background_elem.appendChild(bb_elem);
    bb_elem.appendChild(dom.createTextNode(QString::number(OptionsData::_b)));


    // camera data
    QDomElement camera_elem = dom.createElement("camera");
    config_elem.appendChild(camera_elem);

    QDomElement camera_speed_elem = dom.createElement("speed");
    camera_elem.appendChild(camera_speed_elem);
    camera_speed_elem.appendChild(dom.createTextNode(QString::number(OptionsData::_camSpeed)));

    QDomElement camera_rotationSpeed_elem = dom.createElement("rotation_speed");
    camera_elem.appendChild(camera_rotationSpeed_elem);
    camera_rotationSpeed_elem.appendChild(dom.createTextNode(QString::number(OptionsData::_camRotSpeed)));


    QDomElement export_elem = dom.createElement("export");
    config_elem.appendChild(export_elem);

    QDomElement export_type_elem = dom.createElement("type");
    export_elem.appendChild(export_type_elem);
    if (OptionsData::_mode == Export_Custom)
        export_type_elem.appendChild(dom.createTextNode("custom"));
    else if (OptionsData::_mode == Export_Pack0)
        export_type_elem.appendChild(dom.createTextNode("pack0"));

    QDomElement export_dest_elem = dom.createElement("dest");
    export_elem.appendChild(export_dest_elem);
    export_dest_elem.appendChild(dom.createTextNode(OptionsData::_exportDest));

    QDomElement export_move_tex_elem = dom.createElement("move_textures");
    export_elem.appendChild(export_move_tex_elem);
    export_move_tex_elem.appendChild(dom.createTextNode(QString::number((int)OptionsData::_moveTexture)));

    QDomElement export_move_nm_elem = dom.createElement("move_normals_map");
    export_elem.appendChild(export_move_nm_elem);
    export_move_nm_elem.appendChild(dom.createTextNode(QString::number((int)OptionsData::_nm)));

    QDomElement export_move_sm_elem = dom.createElement("move_specular_map");
    export_elem.appendChild(export_move_sm_elem);
    export_move_sm_elem.appendChild(dom.createTextNode(QString::number((int)OptionsData::_sm)));


    QDomElement formats_elem = dom.createElement("formats_in");
    config_elem.appendChild(formats_elem);
    formats_elem.appendChild(dom.createTextNode(_formats));

    QDomElement unit_elem = dom.createElement("unit");
    QString unit;
    if (ReSize::_unit == Unit_cm)
        unit = "cm";
    else if (ReSize::_unit == Unit_m)
        unit = "m";
    config_elem.appendChild(unit_elem);
    unit_elem.appendChild(dom.createTextNode(unit));




    // textures conversion
    QDomElement tex_conv_elem = dom.createElement("textures_conversion");
    config_elem.appendChild(tex_conv_elem);

    QDomElement tex_conv_enabled_elem = dom.createElement("enabled");
    tex_conv_elem.appendChild(tex_conv_enabled_elem);
    tex_conv_enabled_elem.appendChild(dom.createTextNode(QString::number(OptionsData::_convertTextures)));

    QDomElement tex_conv_format_elem = dom.createElement("format");
    tex_conv_elem.appendChild(tex_conv_format_elem);
    tex_conv_format_elem.appendChild(dom.createTextNode(OptionsData::_texFormat));




    // debug.log
    QDomElement debug_elem = dom.createElement("debug");
    config_elem.appendChild(debug_elem);
    debug_elem.appendChild(dom.createTextNode(QString::number((int)OptionsData::_debugLog)));


    // Write the DOM in a XML
    QString write_doc = dom.toString().toUtf8();
    //write_doc = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" + write_doc;

    QFile file("config.xml");
    if(!file.open(QIODevice::WriteOnly))
    {
        file.close();
        QMessageBox::critical(this,"Erreur","Impossible d'écrire dans le document XML");
        return;
    }
    QTextStream stream(&file);
    stream.setCodec("UTF8");
    stream << write_doc;

    // Delete UI
    delete _ui;
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

    QString file = QFileDialog::getOpenFileName(this, Translator::findTranslation("dialogue_file", _language), param, _formats);

    if (file != "")
    {
        loadFile(file);
    }
}

void MainWindow::convertir()
{
    // Warning if no filename specified
    if (_ui->lineEdit_exportedFilename->text() == "")
        _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + Translator::findTranslation("log_warning_empty", _language) + "\n");

    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + Translator::findTranslation("log_writingFile", _language) + " '" + _ui->lineEdit_exportedFilename->text()+ _ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).left(_ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).indexOf(' ')) + "'... ");
    QCoreApplication::processEvents();

    // Check if the destination folder exist
    QDir dir(OptionsData::getExportFolder());
    if (dir.exists())
        _irrWidget->writeFile(OptionsData::getExportFolder(), _ui->lineEdit_exportedFilename->text(), _ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).left(_ui->comboBox_format->itemText(_ui->comboBox_format->currentIndex()).indexOf(' ')));
    else
    {
        QMessageBox::warning(this, "Error", "The destination folder '" + OptionsData::_exportDest + "' doesn't exist.");
        _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + "\n" + Translator::findTranslation("log_abort", _language) + "\n");
        return;
    }

    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + Translator::findTranslation("log_done", _language) + "\n");
}

void MainWindow::translate()
{
    _ui->button_fileSelector->setText(Translator::findTranslation("button_fileSelector", _language));
    _ui->button_convert->setText(Translator::findTranslation("button_convertir", _language));
    _ui->label_baseDir->setText(Translator::findTranslation("base_directory", _language) + " :");
    _ui->menuLanguages->setTitle(Translator::findTranslation("menu_language", _language));
    _ui->menuMenu->setTitle(Translator::findTranslation("menu_menu", _language));
    _ui->actionQuitter->setText(Translator::findTranslation("menu_quit", _language));
    _ui->menuDisplay->setTitle(Translator::findTranslation("menu_display", _language));
    _ui->actionWireframe->setText(Translator::findTranslation("menu_wireframe", _language));
    _ui->actionSearch->setText(Translator::findTranslation("menu_search", _language));
    _ui->actionOptions->setText(Translator::findTranslation("menu_options", _language));
    _ui->actionWebpage->setText(Translator::findTranslation("menu_webpage", _language));
    _ui->actionShow_linked_files->setText(Translator::findTranslation("menu_linkedFiles", _language));
    _ui->menuHelp->setTitle(Translator::findTranslation("menu_help", _language));
    _ui->label_exportedFilename->setText(Translator::findTranslation("label_exported_file_name", _language) + " :");
    _ui->actionClear_current_LOD->setText(Translator::findTranslation("re_lod_clear", _language));
    _ui->actionClear_all_LODs->setText(Translator::findTranslation("re_lod_clear_all", _language));
    _ui->actionSize->setText(Translator::findTranslation("re_size", _language));
    _ui->menu_RE_tools->setTitle(Translator::findTranslation("re_tools", _language));
    _ui->menuThe_Witcher_3_tools->setTitle(Translator::findTranslation("w3_tools", _language));
    _ui->actionClean_textures_path->setText(Translator::findTranslation("w3_tex_path", _language));

    if (_ui->actionLOD0->text() != "LOD0")
        _ui->actionLOD0->setText("LOD0 (" + Translator::findTranslation("re_lod_empty", _language) + ")");
    if (_ui->actionLOD1->text() != "LOD1")
        _ui->actionLOD1->setText("LOD1 (" + Translator::findTranslation("re_lod_empty", _language) + ")");
    if (_ui->actionLOD2->text() != "LOD2")
        _ui->actionLOD2->setText("LOD2 (" + Translator::findTranslation("re_lod_empty", _language) + ")");
}

void MainWindow::selectFolder()
{
    QString translation = Translator::findTranslation("dialogue_folder", _language);
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
    Options *w = new Options (this, _language, _irrWidget->getPath(), _irrWidget);
    w->show();
    QObject::connect(w, SIGNAL(optionsValidation()), this, SLOT(changeOptions()));
}

void MainWindow::search()
{
    Search* w = new Search (this, _language);
    w->show();
    QObject::connect(w, SIGNAL(loadPressed(QString)), this, SLOT(loadFile(QString)));
    QObject::connect(this, SIGNAL(languageChanged(QString)), w, SLOT(translate(QString)));
}


void MainWindow::loadFile(QString path)
{
    if (!_irrWidget->fileIsOpenableByIrrlicht(path))
    {
        QMessageBox::critical(this, "Error", "Error : The file can't be opened by Irrlicht. Check that you doesn't use special characters in your paths and that you have the reading persission in the corresponding folder.");
        return;
    }

    _ui->textEdit_log->setText(_ui->textEdit_log->toPlainText() + Translator::findTranslation("log_readingFile", _language) + " '" + path + "'... ");
    _ui->lineEdit_ImportedFile->setText(path);
    QCoreApplication::processEvents();

    core::stringc feedbackMessage = "";
    bool success = _irrWidget->setModel(path, _ui->lineEdit_folder->text(), feedbackMessage);
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
            _ui->actionLOD0->setText("LOD0 (" + Translator::findTranslation("re_lod_empty", _language) + ")");
        break;
        case LOD_1:
            _ui->actionLOD1->setText("LOD1 (" + Translator::findTranslation("re_lod_empty", _language) + ")");
        break;
        case LOD_2:
            _ui->actionLOD2->setText("LOD2 (" + Translator::findTranslation("re_lod_empty", _language) + ")");
        break;
        case Collision:
            _ui->actionLOD2->setText("Collision mesh (" + Translator::findTranslation("re_lod_empty", _language) + ")");
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
    _ui->actionLOD0->setText("LOD0 (" + Translator::findTranslation("re_lod_empty", _language) + ")");
    _ui->actionLOD1->setText("LOD1 (" + Translator::findTranslation("re_lod_empty", _language) + ")");
    _ui->actionLOD2->setText("LOD2 (" + Translator::findTranslation("re_lod_empty", _language) + ")");

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
    ext->show();
    ext->read(_ui->lineEdit_ImportedFile->text());
}
