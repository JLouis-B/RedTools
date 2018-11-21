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

    QJsonObject root = document.object();

    QJsonObject mesh = root["Mesh"].toObject();
    QJsonObject assetRef = mesh["AssetRef"].toObject();
    QString meshFilepath = assetRef["FilePath"].toString();

    _log->addLineAndFlush(formatString("filepath = %s", meshFilepath.toStdString().c_str()));

    QString fullMeshFilepath = QString(ConfigGamePath.c_str()) + meshFilepath;

    int lastPoint = fullMeshFilepath.lastIndexOf(".");
    fullMeshFilepath = fullMeshFilepath.left(lastPoint);
    fullMeshFilepath += "_3205126866.cef";

    _log->addLineAndFlush(formatString("new filepath = %s", fullMeshFilepath.toStdString().c_str()));

    io::IReadFile* meshFile = FileSystem->createAndOpenFile(fullMeshFilepath.toStdString().c_str());
    if (meshFile)
    {
        IO_MeshLoader_CEF cefLoader(SceneManager, FileSystem);
        AnimatedMesh = (scene::ISkinnedMesh*)cefLoader.createMesh(meshFile);

        meshFile->drop();
    }
    else
        std::cout << "File " << fullMeshFilepath.toStdString().c_str() << " not found !" << std::endl;

    // stop here if no mesh loaded, else we continue with the materials
    if (!AnimatedMesh)
        return AnimatedMesh;


    // material node list
    QJsonArray materialNodeList = root.value(QString("MaterialNodeList")).toArray();
    foreach (const QJsonValue& materialNodeValue, materialNodeList)
    {
        QJsonObject materialNode = materialNodeValue.toObject();
        QString materialName = materialNode["Name"].toString();
    }


    // node list
    QJsonArray nodeList = root.value(QString("NodeList")).toArray();
    foreach (const QJsonValue& nodeValue, nodeList)
    {
        QJsonObject node = nodeValue.toObject();
        QString nodeName = node["Name"].toString();

        QJsonArray materials = node["MaterialList"].toArray();
        foreach (const QJsonValue& materialValue, materials)
        {
            QJsonObject materialAsset = materialValue.toObject()["AssetRef"].toObject();
            QString materialFilepath = materialAsset["FilePath"].toString();
            _log->addLineAndFlush(formatString("filepath = %s", materialFilepath.toStdString().c_str()));

            QString materialPath = QString(ConfigGamePath.c_str()) + materialFilepath;

            int lastPoint = materialPath.lastIndexOf(".");
            materialPath = materialPath.left(lastPoint);
            materialPath += "_0.mat";

            _log->addLineAndFlush(formatString("new filepath = %s", materialPath.toStdString().c_str()));

            video::SMaterial mat = readMaterialFile(materialPath);
        }
    }


    return AnimatedMesh;
}

video::SMaterial IO_MeshLoader_TheCouncil_Prefab::readMaterialFile(QString path)
{
    video::SMaterial mat;

    QString fileContent;
    QFile file;
    file.setFileName(path);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    fileContent = file.readAll();
    file.close();

    QJsonDocument d = QJsonDocument::fromJson(fileContent.toUtf8());
    QJsonObject sett2 = d.object();

    QJsonObject materialData = sett2.value(QString("Data")).toObject();
    QJsonArray materialTextures = materialData["Textures"].toArray();
    foreach (const QJsonValue& materialTexture, materialTextures)
    {
        QString UID = materialTexture["UID"].toString();
        if (UID == "tTexture_Albedo")
        {
            QJsonObject texObjectAssetRef = materialTexture["Value"].toObject()["AssetRef"].toObject();
            QString textureFilePath = texObjectAssetRef["FilePath"].toString();

            _log->addLineAndFlush(formatString("filepath = %s", textureFilePath.toStdString().c_str()));

            QString texturePath = QString(ConfigGamePath.c_str()) + textureFilePath;

            int lastPoint = texturePath.lastIndexOf(".");
            texturePath = texturePath.left(lastPoint);
            texturePath += "_0.texture.dds";

            _log->addLineAndFlush(formatString("new filepath = %s", texturePath.toStdString().c_str()));

            video::ITexture* texture = nullptr;
            if (FileSystem->existFile(texturePath.toStdString().c_str()))
                texture = SceneManager->getVideoDriver()->getTexture(texturePath.toStdString().c_str());
            else
                _log->addLineAndFlush("Texture not found !");

            if (texture)
            {
                mat.setTexture(0, texture);
                _log->addLineAndFlush("Texture OK !");
            }
            else
                _log->addLineAndFlush("Texture not supported !");
        }
    }


    return mat;
}
