#include "IO_MeshLoader_TheCouncil_Prefab.h"

#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "IWriteFile.h"
#include "Settings.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <iostream>

#include "Utils_Loaders_Irr.h"
#include "Utils_Qt_Irr.h"

#include "IO_MeshLoader_CEF.h"

IO_MeshLoader_TheCouncil_Prefab::IO_MeshLoader_TheCouncil_Prefab(scene::ISceneManager* smgr, io::IFileSystem* fs)
    : SceneManager(smgr), FileSystem(fs)
{

}

bool IO_MeshLoader_TheCouncil_Prefab::isALoadableFileExtension(const io::path& filename) const
{
    return core::hasFileExtension ( filename, "prefab" );
}

scene::IAnimatedMesh* IO_MeshLoader_TheCouncil_Prefab::createMesh(io::IReadFile* file)
{
    if (!file)
        return nullptr;

    AnimatedMesh = nullptr;

    #ifdef _IRR_WCHAR_FILESYSTEM
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsStringW("TW_GAME_PATH");
    #else
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsString("TW_GAME_PATH");
    #endif

    _log = Log::Instance();
    _log->addLine("");
    _log->addLine(formatString("-> File : %s", file->getFileName().c_str()));
    _log->add("_________________________________________________________\n\n\n");
    _log->addLineAndFlush("Start loading");


    c8 fileContent[file->getSize()];
    file->read(fileContent, file->getSize());
    QByteArray qFileContent(fileContent, file->getSize());

    QJsonDocument document = QJsonDocument::fromJson(qFileContent);

    QJsonObject sett2 = document.object();
    QJsonValue meshValue = sett2.value(QString("Mesh"));
    QJsonObject mesh = meshValue.toObject();

    QJsonValue assetRefValue = mesh["AssetRef"];
    QJsonObject assetRef = assetRefValue.toObject();
    QJsonValue filepathValue = assetRef["FilePath"];

    std::cout << "filepath=" << filepathValue.toString().toStdString().c_str() << std::endl;
    QString meshPath = QString(ConfigGamePath.c_str()) + filepathValue.toString();

    int lastPoint = meshPath.lastIndexOf(".");
    meshPath = meshPath.left(lastPoint);
    meshPath += "_3205126866.cef";

    std::cout << "new filepath=" << meshPath.toStdString().c_str() << std::endl;

    io::IReadFile* meshFile = FileSystem->createAndOpenFile(meshPath.toStdString().c_str());
    if (meshFile)
    {
        IO_MeshLoader_CEF cefLoader(SceneManager, FileSystem);
        AnimatedMesh = (scene::ISkinnedMesh*)cefLoader.createMesh(meshFile);

        meshFile->drop();
    }
    else
        std::cout << "File " << meshPath.toStdString().c_str() << " not found !" << std::endl;

    // stop here if no mesh loaded, else we continue with the materials
    if (!AnimatedMesh)
        return AnimatedMesh;

    // material node list
    QJsonValue materialNodeListValue = sett2.value(QString("MaterialNodeList"));
    QJsonArray materialNodeList = materialNodeListValue.toArray();

    foreach (const QJsonValue& materialNodeValue, materialNodeList)
    {
        QJsonObject materialNode = materialNodeValue.toObject();
        QString materialName = materialNode["Name"].toString();
    }



    // node list
    QJsonValue nodeListValue = sett2.value(QString("NodeList"));
    QJsonArray nodeList = nodeListValue.toArray();

    foreach (const QJsonValue& nodeValue, nodeList)
    {
        QJsonObject node = nodeValue.toObject();
        QString nodeName = node["Name"].toString();

        QJsonArray materials = node["MaterialList"].toArray();
        foreach (const QJsonValue& materialValue, materials)
        {
            QJsonObject materialAsset = materialValue.toObject()["AssetRef"].toObject();
            QString materialFilepath = materialAsset["FilePath"].toString();
            std::cout << "materialFilepath=" << materialFilepath.toStdString().c_str() << std::endl;
        }
    }


    return AnimatedMesh;
}

bool IO_MeshLoader_TheCouncil_Prefab::load(io::IReadFile* file)
{

}
