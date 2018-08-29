#include "Settings.h"


QString Settings::_language = "";

double Settings::_camRotSpeed = DEFAULT_CAM_ROT_SPEED;
double Settings::_camSpeed = DEFAULT_CAM_SPEED;

int Settings::_r = 0;
int Settings::_g = 0;
int Settings::_b = 0;

Export_Mode Settings::_mode = Export_Pack0;
QString Settings::_exportDest = "";

bool Settings::_copyTextures = true;
bool Settings::_nm = false;
bool Settings::_sm = false;

bool Settings::_debugLog = false;

bool Settings::_convertTextures = false;
QString Settings::_texFormat = ".jpg";

QString Settings::_pack0 = "";

QString Settings::_TW3TexPath = "";
bool Settings::_TW3LoadSkel = false;
bool Settings::_TW3LoadBestLOD = false;

QString Settings::_formats = "All files/The witcher 1/2/3, Irrlicht and Assimp supported files(*);;The Witcher 2/3 3D models (*.w2ent , *.w2mesh);;The Witcher 1 3D models (*.mdb)";

bool Settings::_firstUse = true;

QString Settings::_appVersion = "2.10";

QString Settings::_exporter = "";
QString Settings::_selectedFilter = "All files/The witcher 1/2/3, Irrlicht and Assimp supported files(*)";

QString Settings::getExportFolder()
{
    if (Settings::_mode == Export_Pack0)
        return Settings::_pack0;
    else
        return Settings::_exportDest;
}

QString Settings::getAppVersion()
{
#ifdef IS_A_DEVELOPMENT_BUILD
    QString appVersion = _appVersion + " - DEV BUILD";
    return appVersion;
#else
    return _appVersion;
#endif
}

QString Settings::getFilters()
{
    QRegExp rx("(\\;;)");
    QStringList filters = Settings::_formats.split(rx);

    QString filter = "";

    bool selectedFilterFound = false;
    foreach (QString filterPart, filters)
    {
        if (filterPart != Settings::_selectedFilter)
        {
            filter += ";;";
            filter += filterPart;
        }
        else
            selectedFilterFound = true;
    }

    if (selectedFilterFound)
        filter = Settings::_selectedFilter + filter;
    else
    {
        Settings::_selectedFilter = filters[0];
        filter = filter.remove(0, 2);
    }

    return filter;
}

void Settings::loadFromXML(QString filename)
{

    // Load config from XML
    QDomDocument* dom = new QDomDocument("config");
    QFile xml_doc(filename);

    if(!xml_doc.open(QIODevice::ReadOnly))
    {
         QMessageBox::warning(nullptr, "Erreur", "Erreur XML");
    }

    if (!dom->setContent(&xml_doc))
    {
        xml_doc.close();
        QMessageBox::warning(nullptr, "Erreur", "Erreur XML");
    }
    QDomElement dom_element = dom->documentElement();
    QDomNode node = dom_element.firstChildElement();
    while (!node.isNull())
    {
        const QString nodeName = node.nodeName();

        if(nodeName == "language")
            Settings::_language = node.toElement().text();
        else if(nodeName == "pack0")
        {
            Settings::_pack0 = node.toElement().text();
        }
        else if(nodeName == "camera")
        {
            Settings::_camSpeed = node.firstChildElement("speed").text().toDouble();
            Settings::_camRotSpeed = node.firstChildElement("rotation_speed").text().toDouble();
        }
        else if(nodeName == "background_color")
        {
            Settings::_r = node.firstChildElement("r").text().toInt();
            Settings::_g = node.firstChildElement("g").text().toInt();
            Settings::_b = node.firstChildElement("b").text().toInt();
        }
        else if(nodeName == "textures_conversion")
        {
            Settings::_convertTextures = node.firstChildElement("enabled").text().toInt();
            Settings::_texFormat = node.firstChildElement("format").text();
        }
        else if(nodeName == "export")
        {
            if (node.firstChildElement("type").text() == "pack0")
                Settings::_mode = Export_Pack0;
            else if (node.firstChildElement("type").text() == "custom")
                Settings::_mode = Export_Custom;

            Settings::_exportDest = node.firstChildElement("dest").text();

            Settings::_copyTextures = node.firstChildElement("move_textures").text().toInt();
            Settings::_nm = node.firstChildElement("move_normals_map").text().toInt();
            Settings::_sm = node.firstChildElement("move_specular_map").text().toInt();
        }
        else if(nodeName == "unit")
        {
            QString unit = node.toElement().text();
            if (unit == "m")
                GUI_Resize::_unit = Unit_m;
            else if (unit == "cm")
                GUI_Resize::_unit = Unit_cm;
        }
        else if(nodeName == "debug")
        {
            Settings::_debugLog = node.toElement().text().toInt();
        }
        else if(nodeName == "TW3")
        {
            Settings::_TW3TexPath = node.firstChildElement("TW3_textures").text();
            Settings::_TW3LoadSkel = node.firstChildElement("TW3_loadSkel").text().toInt();
            Settings::_TW3LoadBestLOD = node.firstChildElement("TW3_loadBestLOD").text().toInt();
        }
        else if (nodeName == "first_use")
        {
            Settings::_firstUse = node.toElement().text().toInt();
        }
        else if (nodeName == "exporter")
        {
            Settings::_exporter = node.toElement().text();
        }
        else if (nodeName == "selected_filter")
        {
            Settings::_selectedFilter = node.toElement().text();
        }

        node = node.nextSibling();
    }

    xml_doc.close();
    delete dom;
}

void Settings::saveToXML(QString filename)
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

    QDomText lang_txt = dom.createTextNode(Settings::_language);
    lang_elem.appendChild(lang_txt);


    // pack0 folder
    QDomElement pack0_elem = dom.createElement("pack0");
    config_elem.appendChild(pack0_elem);

    QDomText pack0_txt = dom.createTextNode(Settings::_pack0);
    pack0_elem.appendChild(pack0_txt);


    // backgroud color
    QDomElement background_elem = dom.createElement("background_color");
    config_elem.appendChild(background_elem);

    QDomElement br_elem = dom.createElement("r");
    background_elem.appendChild(br_elem);
    br_elem.appendChild(dom.createTextNode(QString::number(Settings::_r)));

    QDomElement bg_elem = dom.createElement("g");
    background_elem.appendChild(bg_elem);
    bg_elem.appendChild(dom.createTextNode(QString::number(Settings::_g)));

    QDomElement bb_elem = dom.createElement("b");
    background_elem.appendChild(bb_elem);
    bb_elem.appendChild(dom.createTextNode(QString::number(Settings::_b)));


    // camera data
    QDomElement camera_elem = dom.createElement("camera");
    config_elem.appendChild(camera_elem);

    QDomElement camera_speed_elem = dom.createElement("speed");
    camera_elem.appendChild(camera_speed_elem);
    camera_speed_elem.appendChild(dom.createTextNode(QString::number(Settings::_camSpeed)));

    QDomElement camera_rotationSpeed_elem = dom.createElement("rotation_speed");
    camera_elem.appendChild(camera_rotationSpeed_elem);
    camera_rotationSpeed_elem.appendChild(dom.createTextNode(QString::number(Settings::_camRotSpeed)));


    QDomElement export_elem = dom.createElement("export");
    config_elem.appendChild(export_elem);

    QDomElement export_type_elem = dom.createElement("type");
    export_elem.appendChild(export_type_elem);
    if (Settings::_mode == Export_Custom)
        export_type_elem.appendChild(dom.createTextNode("custom"));
    else if (Settings::_mode == Export_Pack0)
        export_type_elem.appendChild(dom.createTextNode("pack0"));

    QDomElement export_dest_elem = dom.createElement("dest");
    export_elem.appendChild(export_dest_elem);
    export_dest_elem.appendChild(dom.createTextNode(Settings::_exportDest));

    QDomElement export_move_tex_elem = dom.createElement("move_textures");
    export_elem.appendChild(export_move_tex_elem);
    export_move_tex_elem.appendChild(dom.createTextNode(QString::number((int)Settings::_copyTextures)));

    QDomElement export_move_nm_elem = dom.createElement("move_normals_map");
    export_elem.appendChild(export_move_nm_elem);
    export_move_nm_elem.appendChild(dom.createTextNode(QString::number((int)Settings::_nm)));

    QDomElement export_move_sm_elem = dom.createElement("move_specular_map");
    export_elem.appendChild(export_move_sm_elem);
    export_move_sm_elem.appendChild(dom.createTextNode(QString::number((int)Settings::_sm)));

    QDomElement exporter_elem = dom.createElement("exporter");
    config_elem.appendChild(exporter_elem);
    exporter_elem.appendChild(dom.createTextNode(Settings::_exporter));

    QDomElement selectedFilter_elem = dom.createElement("selected_filter");
    config_elem.appendChild(selectedFilter_elem);
    selectedFilter_elem.appendChild(dom.createTextNode(Settings::_selectedFilter));

    QDomElement unit_elem = dom.createElement("unit");
    QString unit;
    if (GUI_Resize::_unit == Unit_cm)
        unit = "cm";
    else if (GUI_Resize::_unit == Unit_m)
        unit = "m";
    config_elem.appendChild(unit_elem);
    unit_elem.appendChild(dom.createTextNode(unit));




    // textures conversion
    QDomElement tex_conv_elem = dom.createElement("textures_conversion");
    config_elem.appendChild(tex_conv_elem);

    QDomElement tex_conv_enabled_elem = dom.createElement("enabled");
    tex_conv_elem.appendChild(tex_conv_enabled_elem);
    tex_conv_enabled_elem.appendChild(dom.createTextNode(QString::number(Settings::_convertTextures)));

    QDomElement tex_conv_format_elem = dom.createElement("format");
    tex_conv_elem.appendChild(tex_conv_format_elem);
    tex_conv_format_elem.appendChild(dom.createTextNode(Settings::_texFormat));




    // debug.log
    QDomElement debug_elem = dom.createElement("debug");
    config_elem.appendChild(debug_elem);
    debug_elem.appendChild(dom.createTextNode(QString::number((int)Settings::_debugLog)));


    // TW3
    QDomElement TW3_elem = dom.createElement("TW3");
    config_elem.appendChild(TW3_elem);

    QDomElement tw3tex_elem = dom.createElement("TW3_textures");
    TW3_elem.appendChild(tw3tex_elem);
    tw3tex_elem.appendChild(dom.createTextNode(Settings::_TW3TexPath));

    QDomElement tw3skel_elem = dom.createElement("TW3_loadSkel");
    TW3_elem.appendChild(tw3skel_elem);
    tw3skel_elem.appendChild(dom.createTextNode(QString::number((int)Settings::_TW3LoadSkel)));

    QDomElement tw3bestlod_elem = dom.createElement("TW3_loadBestLOD");
    TW3_elem.appendChild(tw3bestlod_elem);
    tw3bestlod_elem.appendChild(dom.createTextNode(QString::number((int)Settings::_TW3LoadBestLOD)));




    QDomElement firstuse_elem = dom.createElement("first_use");
    config_elem.appendChild(firstuse_elem);
    firstuse_elem.appendChild(dom.createTextNode(QString::number((int)Settings::_firstUse)));


    // Write the DOM in a XML
    QString write_doc = dom.toString().toUtf8();
    //write_doc = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" + write_doc;

    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly))
    {
        file.close();
        QMessageBox::critical(0, "Erreur", "Impossible d'écrire dans le document XML");
        return;
    }
    QTextStream stream(&file);
    stream.setCodec("UTF8");
    stream << write_doc;
}

