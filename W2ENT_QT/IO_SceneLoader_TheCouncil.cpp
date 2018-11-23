#include "IO_SceneLoader_TheCouncil.h"


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
#include "Utils_TheCouncil.h"

#include "IO_MeshLoader_TheCouncil_Prefab.h"




IO_SceneLoader_TheCouncil::IO_SceneLoader_TheCouncil(scene::ISceneManager* smgr, io::IFileSystem* fs)
    : SceneManager(smgr), FileSystem(fs)
{

}

bool IO_SceneLoader_TheCouncil::isALoadableFileExtension(const io::path& filename) const
{
    return core::hasFileExtension ( filename, "template" );
}

//! Returns true if the class might be able to load this file.
bool IO_SceneLoader_TheCouncil::isALoadableFileFormat(io::IReadFile *file) const
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

    QJsonArray objects = root["Objects"].toArray();
    foreach (const QJsonValue& objectValue, objects)
    {
        std::cout << "Object" << std::endl;
        QJsonObject object = objectValue.toObject();
        QJsonObject prefabAssetRef = object["Prefab"].toObject()["AssetRef"].toObject();
        QString prefabPath = prefabAssetRef["FilePath"].toString();

        prefabPath = QString(ConfigGamePath.c_str()) + prefabPath;

        QFileInfo prefabFileInfo = findFile(prefabPath);
        if (prefabFileInfo.exists())
        {
            std::cout << "Prefab exist" << std::endl;

            io::IReadFile* meshFile = FileSystem->createAndOpenFile(prefabFileInfo.absoluteFilePath().toStdString().c_str());
            if (meshFile)
            {
                std::cout << "Mesh file" << std::endl;
                IO_MeshLoader_TheCouncil_Prefab prefabLoader(SceneManager, FileSystem);
                scene::IAnimatedMesh* animatedMesh = prefabLoader.createMesh(meshFile);

                meshFile->drop();

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
    }


    return true;
}
