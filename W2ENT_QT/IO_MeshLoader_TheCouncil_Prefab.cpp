#include "IO_MeshLoader_TheCouncil_Prefab.h"

#include <ISceneManager.h>
#include <IVideoDriver.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <iostream>

#include "Settings.h"
#include "Utils_TheCouncil.h"
#include "Utils_Loaders_Irr.h"
#include "Utils_Qt_Irr.h"

#include "IO_MeshLoader_CEF.h"

IO_MeshLoader_TheCouncil_Prefab::IO_MeshLoader_TheCouncil_Prefab(scene::ISceneManager* smgr, io::IFileSystem* fs)
    : SceneManager(smgr),
      FileSystem(fs)
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
    if (!mesh.contains("AssetRef")) // no mesh
    {
        return SceneManager->createSkinnedMesh();
    }

    QJsonObject assetRef = mesh["AssetRef"].toObject();
    QString meshFilepath = assetRef["FilePath"].toString();

    _log->addLineAndFlush(formatString("filepath = %s", meshFilepath.toStdString().c_str()));

    QString fullMeshFilepath = QString(ConfigGamePath.c_str()) + meshFilepath;

    QFileInfo meshFileInfo = findFile(fullMeshFilepath, TheCouncil_CEF);
    core::array<core::stringc> bufferNames;
    if (meshFileInfo.exists())
    {
        //_log->addLineAndFlush(formatString("new filepath = %s", fullMeshFilepath.toStdString().c_str()));

        io::IReadFile* meshFile = FileSystem->createAndOpenFile(meshFileInfo.absoluteFilePath().toStdString().c_str());
        if (meshFile)
        {
            IO_MeshLoader_CEF cefLoader(SceneManager, FileSystem);
            AnimatedMesh = (scene::ISkinnedMesh*)cefLoader.createMesh(meshFile);
            bufferNames = cefLoader.bufferNames;

            meshFile->drop();
        }
        else
            _log->addLineAndFlush(formatString("Error: Fail to open %s !", fullMeshFilepath.toStdString().c_str()));
    }
    else
        _log->addLineAndFlush(formatString("Error: %s not found", fullMeshFilepath.toStdString().c_str()));

    // stop here if no mesh loaded, else we continue with the materials
    if (!AnimatedMesh)
        return nullptr;


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

            QFileInfo materialFileInfo = findFile(materialPath, TheCouncil_JSON);
            if (materialFileInfo.exists())
            {
                _log->addLineAndFlush(formatString("new filepath = %s", materialFileInfo.absoluteFilePath().toStdString().c_str()));

                video::SMaterial mat = readMaterialFile(materialFileInfo.absoluteFilePath());

                for (u32 i = 0; i < bufferNames.size(); ++i)
                {
                    if (QString(bufferNames[i].c_str()) == nodeName)
                    {
                        AnimatedMesh->getMeshBuffer(i)->getMaterial() = mat;
                        break;
                    }
                }
            }
        }
    }


    return AnimatedMesh;
}

video::SMaterial IO_MeshLoader_TheCouncil_Prefab::readMaterialFile(QString path)
{
    video::SMaterial mat;

    QFile file;
    file.setFileName(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        _log->addLineAndFlush(formatString("Fail to open material file : %s", path.toStdString().c_str()));
        _log->addLineAndFlush(file.errorString().toStdString().c_str());
        return mat;
    }

    QString fileContent = file.readAll();
    file.close();

    QJsonDocument d = QJsonDocument::fromJson(fileContent.toUtf8());
    QJsonObject sett2 = d.object();

    QJsonObject materialData = sett2.value(QString("Data")).toObject();
    QJsonArray materialTextures = materialData["Textures"].toArray();

    int idxTex = 0;
    bool diffuseTextureFound = false;
    foreach (const QJsonValue& materialTexture, materialTextures)
    {
        QString UID = materialTexture["UID"].toString();
        if (idxTex == 0
                || UID == "tTexture_Albedo"
                || UID == "tTexture_Diff"
                || UID == "tTexture_DFF")
        {
            QJsonObject texObjectAssetRef = materialTexture["Value"].toObject()["AssetRef"].toObject();
            QString textureFilePath = texObjectAssetRef["FilePath"].toString();

            _log->addLineAndFlush(formatString("idxTex = %d, UID = %s", idxTex, UID.toStdString().c_str()));
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

            diffuseTextureFound = true;
        }

        ++idxTex;
    }

    if (!diffuseTextureFound)
        _log->addLineAndFlush("Warning: No diffuse texture found in the material");

    return mat;
}
