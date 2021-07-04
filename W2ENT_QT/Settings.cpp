#include "Settings.h"

#include <QtXml>
#include <QMessageBox>

QString Settings::_theme;

QString Settings::_language = QString();

double Settings::_cameraRotationSpeed = DEFAULT_CAM_ROT_SPEED;
double Settings::_cameraSpeed = DEFAULT_CAM_SPEED;

QColor Settings::_backgroundColor;


Export_Mode Settings::_mode = Export_BaseDir;
QString Settings::_exportDest = QString();

bool Settings::_copyTexturesEnabled = true;
bool Settings::_copyTexturesSlot1 = false;
bool Settings::_copyTexturesSlot2 = false;

bool Settings::_debugLog = false;

bool Settings::_convertTexturesEnabled = false;
QString Settings::_convertTexturesFormat = ".jpg";

QString Settings::_baseDir = QString();

bool Settings::_TW1LoadStaticMesh = true;
bool Settings::_TW1LoadSkinnedMesh = true;
bool Settings::_TW1LoadPaintedMesh = true;

bool Settings::_TW2LoadBestLODEnabled = true;

QString Settings::_TW3TexPath = QString();
bool Settings::_TW3LoadSkeletonEnabled = false;
bool Settings::_TW3LoadBestLODEnabled = false;

QString Settings::_formats = "All files/The witcher 1/2/3, Irrlicht and Assimp supported files(*);;The Witcher 2/3 3D models (*.w2ent , *.w2mesh);;The Witcher 1 3D models (*.mdb)";

bool Settings::_firstUse = true;

QString Settings::_exporter = QString();
QString Settings::_selectedFilter = "All files/The witcher 1/2/3, Irrlicht and Assimp supported files(*)";

Unit Settings::_unit = Unit_m;

WindowState Settings::_windowState;
SearchSettings Settings::_searchSettings;

QString Settings::getExportFolder()
{
    QString exportFolder;
    if (Settings::_mode == Export_BaseDir)
        exportFolder = Settings::_baseDir;
    else
        exportFolder = Settings::_exportDest;

    if (exportFolder.size() > 0 && exportFolder[exportFolder.size()-1] != '\\')
        exportFolder.push_back('\\');

    return exportFolder;
}

QString Settings::getAppVersion()
{
    QString appVersion = QString::number(VERSION_MAJOR) + "." + QString::number(VERSION_MINOR);
#ifdef IS_A_DEVELOPMENT_BUILD
    appVersion += " - DEV BUILD";
#endif
    return appVersion;
}

QString Settings::getFilters()
{
    QRegExp rx("(\\;;)");
    QStringList filters = Settings::_formats.split(rx);

    QString filter;

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

QByteArray stringToByteArray(QString input)
{
    QByteArray data = QByteArray::fromStdString(input.toStdString());
    data = QByteArray::fromBase64(data);
    return data;
}

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

        if (nodeName == "language")
            Settings::_language = node.toElement().text();
        else if (nodeName == "theme")
        {
            Settings::_theme = node.toElement().text();
        }
        else if (nodeName == "base_directory")
        {
            Settings::_baseDir = node.toElement().text();
        }
        else if (nodeName == "camera")
        {
            Settings::_cameraSpeed = node.firstChildElement("speed").text().toDouble();
            Settings::_cameraRotationSpeed = node.firstChildElement("rotation_speed").text().toDouble();
        }
        else if (nodeName == "background_color")
        {
            QColor backgroundColor;
            backgroundColor.setRed(node.firstChildElement("r").text().toInt());
            backgroundColor.setGreen(node.firstChildElement("g").text().toInt());
            backgroundColor.setBlue(node.firstChildElement("b").text().toInt());
            Settings::_backgroundColor = backgroundColor;
        }
        else if (nodeName == "textures_conversion")
        {
            Settings::_convertTexturesEnabled = node.firstChildElement("enabled").text().toInt();
            Settings::_convertTexturesFormat = node.firstChildElement("format").text();
        }
        else if (nodeName == "export")
        {
            QString exportType = node.firstChildElement("type").text();
            if (exportType == "base_directory")
                Settings::_mode = Export_BaseDir;
            else if (exportType == "custom")
                Settings::_mode = Export_Custom;

            Settings::_exportDest = node.firstChildElement("destination").text();

            Settings::_copyTexturesEnabled = node.firstChildElement("copy_textures").text().toInt();
            Settings::_copyTexturesSlot1 = node.firstChildElement("copy_normals_map").text().toInt();
            Settings::_copyTexturesSlot2 = node.firstChildElement("copy_specular_map").text().toInt();
        }
        else if (nodeName == "unit")
        {
            QString unit = node.toElement().text();
            if (unit == "m")
                Settings::_unit = Unit_m;
            else if (unit == "cm")
                Settings::_unit = Unit_cm;
        }
        else if (nodeName == "debug_log")
        {
            Settings::_debugLog = node.toElement().text().toInt();
        }
        else if (nodeName == "TW1")
        {
            Settings::_TW1LoadStaticMesh = node.firstChildElement("TW1_loadStaticMeshes").text().toInt();
            Settings::_TW1LoadSkinnedMesh = node.firstChildElement("TW1_loadSkinnedMeshes").text().toInt();
            Settings::_TW1LoadPaintedMesh = node.firstChildElement("TW1_loadPaintedMeshes").text().toInt();
        }
        else if (nodeName == "TW2")
        {
            Settings::_TW2LoadBestLODEnabled = node.firstChildElement("TW2_loadBestLOD").text().toInt();
        }
        else if (nodeName == "TW3")
        {
            Settings::_TW3TexPath = node.firstChildElement("TW3_textures").text();
            Settings::_TW3LoadSkeletonEnabled = node.firstChildElement("TW3_loadSkel").text().toInt();
            Settings::_TW3LoadBestLODEnabled = node.firstChildElement("TW3_loadBestLOD").text().toInt();
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
            QByteArray windowGeometry = stringToByteArray(node.toElement().text());

            WindowState window;
            window._geometry = windowGeometry;
            window._initialised = true;
            Settings::_windowState = window;
        }
        else if (nodeName == "search_settings")
        {
            SearchSettings searchSettings;
            if (!node.firstChildElement("window_geometry").isNull())
                searchSettings._windowGeometry = stringToByteArray(node.firstChildElement("window_geometry").text());
            else
                searchSettings._windowGeometry = QByteArray();

            searchSettings._checkFolderNames = node.firstChildElement("check_folder_names").text().toInt();
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

void appendNewByteArrayElement(QDomDocument& dom, QDomElement& parent, QString name, QByteArray value)
{
    QString base64data = QString::fromStdString(value.toBase64().toStdString());
    appendNewStringElement(dom, parent, name, base64data);
}

void Settings::saveToXML(QString filename)
{
    QDomDocument dom("config");
    QDomElement configElem = dom.createElement("config");
    dom.appendChild(configElem);

    // theme
    appendNewStringElement(dom, configElem, "theme", Settings::_theme);

    // language
    appendNewStringElement(dom, configElem, "language", Settings::_language);

    // base directory
    appendNewStringElement(dom, configElem, "base_directory", Settings::_baseDir);

    // backgroud color
    appendNewColorElement(dom, configElem, "background_color", Settings::_backgroundColor);

    // camera data
    QDomElement cameraElem = dom.createElement("camera");
    configElem.appendChild(cameraElem);
    appendNewDoubleElement(dom, cameraElem, "speed", Settings::_cameraSpeed);
    appendNewDoubleElement(dom, cameraElem, "rotation_speed", Settings::_cameraRotationSpeed);

    // export settings
    QDomElement exportElem = dom.createElement("export");
    configElem.appendChild(exportElem);

    QString exportType = Settings::_mode == Export_BaseDir ? "base_directory" : "custom";
    appendNewStringElement(dom, exportElem, "type", exportType);
    appendNewStringElement(dom, exportElem, "destination", Settings::_exportDest);
    appendNewBoolElement(dom, exportElem, "copy_textures", Settings::_copyTexturesEnabled);
    appendNewBoolElement(dom, exportElem, "copy_normals_map", Settings::_copyTexturesSlot1);
    appendNewBoolElement(dom, exportElem, "copy_specular_map", Settings::_copyTexturesSlot2);

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

    appendNewBoolElement(dom, texConvElem, "enabled", Settings::_convertTexturesEnabled);
    appendNewStringElement(dom, texConvElem, "format", Settings::_convertTexturesFormat);

    // debug.log
    appendNewBoolElement(dom, configElem, "debug_log", Settings::_debugLog);

    // TW1
    QDomElement TW1Elem = dom.createElement("TW1");
    configElem.appendChild(TW1Elem);
    appendNewBoolElement(dom, TW1Elem, "TW1_loadStaticMeshes", Settings::_TW1LoadStaticMesh);
    appendNewBoolElement(dom, TW1Elem, "TW1_loadSkinnedMeshes", Settings::_TW1LoadSkinnedMesh);
    appendNewBoolElement(dom, TW1Elem, "TW1_loadPaintedMeshes", Settings::_TW1LoadPaintedMesh);

    // TW2
    QDomElement TW2Elem = dom.createElement("TW2");
    configElem.appendChild(TW2Elem);
    appendNewBoolElement(dom, TW2Elem, "TW2_loadBestLOD", Settings::_TW2LoadBestLODEnabled);

    // TW3
    QDomElement TW3Elem = dom.createElement("TW3");
    configElem.appendChild(TW3Elem);
    appendNewStringElement(dom, TW3Elem, "TW3_textures", Settings::_TW3TexPath);
    appendNewBoolElement(dom, TW3Elem, "TW3_loadSkel", Settings::_TW3LoadSkeletonEnabled);
    appendNewBoolElement(dom, TW3Elem, "TW3_loadBestLOD", Settings::_TW3LoadBestLODEnabled);

    // first usage
    appendNewBoolElement(dom, configElem, "first_use", Settings::_firstUse);

    // window state
    WindowState window = Settings::_windowState;
    appendNewByteArrayElement(dom, configElem, "window_state", window._geometry);

    // search settings
    SearchSettings searchSettings = Settings::_searchSettings;

    QDomElement searchSettingsElem = dom.createElement("search_settings");
    configElem.appendChild(searchSettingsElem);

    appendNewByteArrayElement(dom, searchSettingsElem, "window_geometry", searchSettings._windowGeometry);
    appendNewBoolElement(dom, searchSettingsElem, "check_folder_names", searchSettings._checkFolderNames);
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

