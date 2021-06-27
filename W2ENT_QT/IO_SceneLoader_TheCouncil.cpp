#include "IO_SceneLoader_TheCouncil.h"

#include <ISceneManager.h>
#include <IAnimatedMeshSceneNode.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "Settings.h"
#include "Utils_Loaders_Irr.h"
#include "Utils_Qt_Irr.h"
#include "Utils_TheCouncil.h"

#include "IO_MeshLoader_TheCouncil_Prefab.h"




IO_SceneLoader_TheCouncil::IO_SceneLoader_TheCouncil(scene::ISceneManager* smgr, io::IFileSystem* fs)
    : SceneManager(smgr),
      FileSystem(fs),
      RootNode(nullptr),
      _log(nullptr)
{
}

bool IO_SceneLoader_TheCouncil::isALoadableFileExtension(const io::path& filename) const
{
    return core::hasFileExtension ( filename, "template" );
}

//! Returns true if the class might be able to load this file.
bool IO_SceneLoader_TheCouncil::isALoadableFileFormat(io::IReadFile* file) const
{
    // todo: check inside the file
    return true;
}

bool IO_SceneLoader_TheCouncil::loadScene(io::IReadFile* file, scene::ISceneUserDataSerializer* userDataSerializer, scene::ISceneNode* rootNode)
{
    if (!file)
        return false;

    RootNode = rootNode;

    #ifdef _IRR_WCHAR_FILESYSTEM
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsStringW("TW_GAME_PATH");
    #else
        ConfigGamePath = SceneManager->getParameters()->getAttributeAsString("TW_GAME_PATH");
    #endif

    _log = LoggerManager::Instance();
    _log->addLine("");
    _log->addLine(formatString("-> File : %s", file->getFileName().c_str()));
    _log->add("_________________________________________________________\n\n\n");
    _log->addLineAndFlush("Start loading");


    c8 fileContent[file->getSize()];
    file->read(fileContent, file->getSize());
    QByteArray qFileContent(fileContent, file->getSize());

    QJsonDocument document = QJsonDocument::fromJson(qFileContent);

    QJsonObject root = document.object();

    QJsonArray objects = root["Objects"].toArray();
    foreach (const QJsonValue& objectValue, objects)
    {
        _log->addLineAndFlush("Object");
        QJsonObject object = objectValue.toObject();

        if (object.contains("Prefab"))
        {
            QJsonObject prefabAssetRef = object["Prefab"].toObject()["AssetRef"].toObject();
            QString prefabPath = prefabAssetRef["FilePath"].toString();

            prefabPath = QString(ConfigGamePath.c_str()) + prefabPath;

            QFileInfo prefabFileInfo = findFile(prefabPath, TheCouncil_JSON);
            if (prefabFileInfo.exists())
            {
                _log->addLineAndFlush("Prefab found");

                io::IReadFile* meshFile = FileSystem->createAndOpenFile(prefabFileInfo.absoluteFilePath().toStdString().c_str());
                if (meshFile)
                {
                    _log->addLineAndFlush("File successfully opened");
                    IO_MeshLoader_TheCouncil_Prefab prefabLoader(SceneManager, FileSystem);
                    scene::IAnimatedMesh* animatedMesh = prefabLoader.createMesh(meshFile);
                    meshFile->drop();


                    if (animatedMesh == nullptr)
                    {
                        _log->addLineAndFlush(formatString("Error: Loading of the prefab %s has failed", prefabFileInfo.absoluteFilePath().toStdString().c_str()));
                    }
                    else
                    {
                        scene::IAnimatedMeshSceneNode* node = SceneManager->addAnimatedMeshSceneNode(animatedMesh, RootNode);

                        QJsonObject transform = object["Transform"].toObject()["DefaultValue"].toObject();
                        QJsonObject translation = transform["Translation"].toObject();
                        core::vector3df position;
                        position.X = translation["x"].toDouble();
                        position.Y = translation["y"].toDouble();
                        position.Z = translation["z"].toDouble();
                        node->setPosition(position);

                        QJsonObject rotation = transform["Rotation"].toObject();
                        core::quaternion quat;
                        quat.X = rotation["x"].toDouble();
                        quat.Y = rotation["y"].toDouble();
                        quat.Z = rotation["z"].toDouble();
                        quat.W = rotation["w"].toDouble();
                        core::vector3df euler;
                        quat.toEuler(euler);
                        euler = core::vector3df(-euler.X, -euler.Y, euler.Z) * 180.f / 3.14f;

                        node->setRotation(euler);

                        QJsonObject scale = transform["Scale"].toObject();
                        core::vector3df scaleV;
                        scaleV.X = scale["x"].toDouble();
                        scaleV.Y = scale["y"].toDouble();
                        scaleV.Z = scale["z"].toDouble();
                        node->setScale(scaleV);

                        node->setMaterialFlag(video::EMF_LIGHTING, false);
                    }
                }
                else
                {
                    _log->addLineAndFlush(formatString("Error: Fail to open %s", prefabFileInfo.absoluteFilePath().toStdString().c_str()));
                }
            }
            else
            {
                _log->addLineAndFlush(formatString("Error: %s not found", prefabFileInfo.absoluteFilePath().toStdString().c_str()));
            }
        }
    }


    return true;
}
