#include "Settings.h"

#include <QtXml>
#include <QMessageBox>


QString Settings::_language = QString();

double Settings::_camRotSpeed = DEFAULT_CAM_ROT_SPEED;
double Settings::_camSpeed = DEFAULT_CAM_SPEED;

QColor Settings::_backgroundColor;


Export_Mode Settings::_mode = Export_BaseDir;
QString Settings::_exportDest = QString();

bool Settings::_copyTextures = true;
bool Settings::_nm = false;
bool Settings::_sm = false;

bool Settings::_debugLog = false;

bool Settings::_convertTextures = false;
QString Settings::_texFormat = ".jpg";

QString Settings::_baseDir = QString();

QString Settings::_TW3TexPath = QString();
bool Settings::_TW3LoadSkel = false;
bool Settings::_TW3LoadBestLOD = false;

QString Settings::_formats = "All files/The witcher 1/2/3, Irrlicht and Assimp supported files(*);;The Witcher 2/3 3D models (*.w2ent , *.w2mesh);;The Witcher 1 3D models (*.mdb)";

bool Settings::_firstUse = true;

QString Settings::_appVersion = "2.10";

QString Settings::_exporter = QString();
QString Settings::_selectedFilter = "All files/The witcher 1/2/3, Irrlicht and Assimp supported files(*)";

Unit Settings::_unit = Unit_m;

WindowState Settings::_windowState;
SearchSettings Settings::_searchSettings;

QString Settings::getExportFolder()
{
    if (Settings::_mode == Export_BaseDir)
        return Settings::_baseDir;
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

float MeshSize::_scaleFactor = 1.f;

void Settings::loadFromXML(QString filename)
{
    // Load config from XML
    QDomDocument* dom = new QDomDocument("config");
    QFile xmlDoc(filename);

    if(!xmlDoc.open(QIODevice::ReadOnly))
    {
         QMessageBox::warning(nullptr, "Error", "Fail to load config.xml");
    }

    if (!dom->setContent(&xmlDoc))
    {
        xmlDoc.close();
        QMessageBox::warning(nullptr, "Error", "Fail to load config.xml");
    }
    QDomElement dom_element = dom->documentElement();
    QDomNode node = dom_element.firstChildElement();
    while (!node.isNull())
    {
        const QString nodeName = node.nodeName();

        if(nodeName == "language")
            Settings::_language = node.toElement().text();
        else if(nodeName == "base_directory")
        {
            Settings::_baseDir = node.toElement().text();
        }
        else if(nodeName == "camera")
        {
            Settings::_camSpeed = node.firstChildElement("speed").text().toDouble();
            Settings::_camRotSpeed = node.firstChildElement("rotation_speed").text().toDouble();
        }
        else if(nodeName == "background_color")
        {
            QColor backgroundColor;
            backgroundColor.setRed(node.firstChildElement("r").text().toInt());
            backgroundColor.setGreen(node.firstChildElement("g").text().toInt());
            backgroundColor.setBlue(node.firstChildElement("b").text().toInt());
            Settings::_backgroundColor = backgroundColor;
        }
        else if(nodeName == "textures_conversion")
        {
            Settings::_convertTextures = node.firstChildElement("enabled").text().toInt();
            Settings::_texFormat = node.firstChildElement("format").text();
        }
        else if(nodeName == "export")
        {
            if (node.firstChildElement("type").text() == "pack0")
                Settings::_mode = Export_BaseDir;
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
                Settings::_unit = Unit_m;
            else if (unit == "cm")
                Settings::_unit = Unit_cm;
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
        else if (nodeName == "window_state")
        {
            QByteArray windowGeometry = QByteArray::fromStdString(node.toElement().text().toStdString());
            windowGeometry = QByteArray::fromBase64(windowGeometry);

            WindowState window;
            window._geometry = windowGeometry;
            window._initialised = true;
            Settings::_windowState = window;
        }
        else if (nodeName == "search_settings")
        {
            SearchSettings searchSettings;
            searchSettings._searchRedMeshes = node.firstChildElement("meshes").text().toInt();
            searchSettings._searchRedRigs = node.firstChildElement("rigs").text().toInt();
            searchSettings._searchRedAnimations = node.firstChildElement("animations").text().toInt();
            searchSettings._additionnalExtensions = node.firstChildElement("additional_extensions").text();
            Settings::_searchSettings = searchSettings;
        }

        node = node.nextSibling();
    }

    xmlDoc.close();
    delete dom;
}

// some helpers to write the xml
void appendNewStringElement(QDomDocument& dom, QDomElement& parent, QString name, QString value)
{
    QDomElement newElement = dom.createElement(name);
    parent.appendChild(newElement);
    newElement.appendChild(dom.createTextNode(value));
}

void appendNewIntElement(QDomDocument& dom, QDomElement& parent, QString name, int value)
{
    appendNewStringElement(dom, parent, name, QString::number(value));
}

void appendNewDoubleElement(QDomDocument& dom, QDomElement& parent, QString name, double value)
{
    appendNewStringElement(dom, parent, name, QString::number(value));
}

void appendNewBoolElement(QDomDocument& dom, QDomElement& parent, QString name, bool value)
{
    appendNewStringElement(dom, parent, name, QString::number(value));
}

void appendNewColorElement(QDomDocument& dom, QDomElement& parent, QString name, QColor value)
{
    QDomElement newElement = dom.createElement(name);
    parent.appendChild(newElement);

    appendNewIntElement(dom, newElement, "r", value.red());
    appendNewIntElement(dom, newElement, "g", value.green());
    appendNewIntElement(dom, newElement, "b", value.blue());
}

void Settings::saveToXML(QString filename)
{
    QDomDocument dom("config");
    QDomElement configElem = dom.createElement("config");
    dom.appendChild(configElem);

    // language
    appendNewStringElement(dom, configElem, "language", Settings::_language);

    // base directory
    appendNewStringElement(dom, configElem, "base_directory", Settings::_baseDir);

    // backgroud color
    appendNewColorElement(dom, configElem, "background_color", Settings::_backgroundColor);

    // camera data
    QDomElement cameraElem = dom.createElement("camera");
    configElem.appendChild(cameraElem);
    appendNewDoubleElement(dom, cameraElem, "speed", Settings::_camSpeed);
    appendNewDoubleElement(dom, cameraElem, "rotation_speed", Settings::_camRotSpeed);

    // export settings
    QDomElement exportElem = dom.createElement("export");
    configElem.appendChild(exportElem);

    QString exportType = Settings::_mode == Export_BaseDir ? "pack0" : "custom";
    appendNewStringElement(dom, exportElem, "type", exportType);
    appendNewStringElement(dom, exportElem, "dest", Settings::_exportDest);
    appendNewBoolElement(dom, exportElem, "move_textures", Settings::_copyTextures);
    appendNewBoolElement(dom, exportElem, "move_normals_map", Settings::_nm);
    appendNewBoolElement(dom, exportElem, "move_specular_map", Settings::_sm);

    appendNewStringElement(dom, configElem, "exporter", Settings::_exporter);
    appendNewStringElement(dom, configElem, "selected_filter", Settings::_selectedFilter);

    // unit
    QString unit;
    if (Settings::_unit == Unit_cm)
        unit = "cm";
    else if (Settings::_unit == Unit_m)
        unit = "m";
    appendNewStringElement(dom, configElem, "unit", unit);

    // textures conversion
    QDomElement texConvElem = dom.createElement("textures_conversion");
    configElem.appendChild(texConvElem);

    appendNewBoolElement(dom, texConvElem, "enabled", Settings::_convertTextures);
    appendNewStringElement(dom, texConvElem, "format", Settings::_texFormat);

    // debug.log
    appendNewBoolElement(dom, configElem, "debug", Settings::_debugLog);

    // TW3
    QDomElement TW3Elem = dom.createElement("TW3");
    configElem.appendChild(TW3Elem);

    appendNewStringElement(dom, TW3Elem, "TW3_textures", Settings::_TW3TexPath);
    appendNewBoolElement(dom, TW3Elem, "TW3_loadSkel", Settings::_TW3LoadSkel);
    appendNewBoolElement(dom, TW3Elem, "TW3_loadBestLOD", Settings::_TW3LoadBestLOD);

    // first usage
    appendNewBoolElement(dom, configElem, "first_use", Settings::_firstUse);

    // window state
    WindowState window = Settings::_windowState;
    QString windowGeometryData = QString::fromStdString(window._geometry.toBase64().toStdString());
    appendNewStringElement(dom, configElem, "window_state", windowGeometryData);

    // search settings
    SearchSettings searchSettings = Settings::_searchSettings;

    QDomElement searchSettingsElem = dom.createElement("search_settings");
    configElem.appendChild(searchSettingsElem);

    appendNewBoolElement(dom, searchSettingsElem, "meshes", searchSettings._searchRedMeshes);
    appendNewBoolElement(dom, searchSettingsElem, "rigs", searchSettings._searchRedRigs);
    appendNewBoolElement(dom, searchSettingsElem, "animations", searchSettings._searchRedAnimations);
    appendNewStringElement(dom, searchSettingsElem, "additional_extensions", searchSettings._additionnalExtensions);


    // Write the DOM in a XML
    QString writeDoc = dom.toString().toUtf8();

    QFile file(filename);
    if(file.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&file);
        stream.setCodec("UTF8");
        stream << writeDoc;
        file.close();
    }
    else
    {
        QMessageBox::critical(nullptr, "Error", "Fail to write config.xml");
    }
}

