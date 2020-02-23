#include "QIrrlichtWidget.h"

#include <QApplication>
#include <QMessageBox>
#include <QDir>

#include "IO_MeshLoader_W2ENT.h"
#include "IO_MeshLoader_W3ENT.h"
#include "IO_MeshLoader_RE.h"
#include "IO_MeshLoader_WitcherMDL.h"
#include "IO_MeshLoader_CEF.h"
#include "IO_MeshLoader_TheCouncil_Prefab.h"
#include "IO_SceneLoader_TheCouncil.h"

#include "MeshCombiner.h"
#include "Translator.h"
#include "Utils_Qt_Irr.h"
#include "Utils_Qt.h"

#include <iostream>


using namespace irr;


QIrrlichtWidget::QIrrlichtWidget (QWidget *parent) :
    QWidget (parent),
    _device (nullptr),
    _normalsMaterial(nullptr)
{
    // on écrit directement dans al mémoire vidéo du widget
    //setAttribute (Qt::WA_PaintOnScreen);
    setAttribute (Qt::WA_OpaquePaintEvent);
    setFocusPolicy (Qt::StrongFocus);
    setAutoFillBackground (false);

    _currentLOD = LOD_0;
    _currentLodData = &_lod0Data;

    _reWriter = nullptr;
    _camera = nullptr;

    _inLoading = false;
    _normalsRendererEnabled = false;
}

QIrrlichtWidget::~QIrrlichtWidget ()
{
    if (_device)
    {
        _device->closeDevice ();
        _device->drop ();
    }
}

void QIrrlichtWidget::init()
{
    SIrrlichtCreationParameters params;

    params.DriverType        = video::EDT_OPENGL;
    params.WindowId          = reinterpret_cast<void*> (winId ());

    params.WindowSize.Width  = width();
    params.WindowSize.Height = height();



    params.AntiAlias         = true;
    params.Bits              = 16;
    params.HighPrecisionFPU  = false;
    params.Stencilbuffer     = true;
    params.Vsync             = true;

    _device = createDeviceEx(params);
    _device->getSceneManager()->getParameters()->setAttribute("TW_FEEDBACK", "");

    /* une fois initialisé, on émet le signal onInit, c'est la que nous
     * ajouterons nos modèles et nos textures.
     */
    emit onInit (this);


    setMouseTracking(true);

    if (_device)
    {
        _camera = _device->getSceneManager()->addCameraSceneNodeMaya(nullptr, Settings::_cameraRotationSpeed, 100, Settings::_cameraSpeed, -1, 50);
        _camera->setPosition(core::vector3df(0.f, 30.f, -40.f));
        _camera->setTarget(core::vector3df(0.f, 0.f, 0.f));
        const f32 aspectRatio = static_cast<float>(width ()) / height();
        _camera->setAspectRatio(aspectRatio);
        _camera->setFarValue(10000.f);

        _reWriter = new scene::IO_MeshWriter_RE(_device->getSceneManager(), _device->getFileSystem());

        _device->getSceneManager()->addExternalMeshLoader(new IO_MeshLoader_WitcherMDL(_device->getSceneManager(), _device->getFileSystem()));
        _device->getSceneManager()->addExternalMeshLoader(new scene::IO_MeshLoader_RE(_device->getSceneManager(), _device->getFileSystem()));
        _device->getSceneManager()->addExternalMeshLoader(new scene::IO_MeshLoader_W2ENT(_device->getSceneManager(), _device->getFileSystem()));
        _device->getSceneManager()->addExternalMeshLoader(new scene::IO_MeshLoader_W3ENT(_device->getSceneManager(), _device->getFileSystem()));
        _device->getSceneManager()->addExternalMeshLoader(new IO_MeshLoader_CEF(_device->getSceneManager(), _device->getFileSystem()));
        _device->getSceneManager()->addExternalMeshLoader(new IO_MeshLoader_TheCouncil_Prefab(_device->getSceneManager(), _device->getFileSystem()));

        _device->getSceneManager()->addExternalSceneLoader(new IO_SceneLoader_TheCouncil(_device->getSceneManager(), _device->getFileSystem()));

        //_device->getSceneManager()->setAmbientLight(video::SColor(255,255,255,255));
    }

    /* puis on connecte notre slot updateIrrlicht (), qui s'occupe du rendu
     * à notre signal updateIrrlichtQuery ()
     */
    connect (this, SIGNAL (updateIrrlichtQuery (QIrrlichtWidget*)), this, SLOT (updateIrrlicht (QIrrlichtWidget*)));

    // et on lance notre timer
    startTimer (0);

    // creation du fichier de log
    Log::Instance()->setOutput(LOG_NONE);
    if (Settings::_debugLog)
        Log::Instance()->addOutput(LOG_FILE);

    initNormalsMaterial();

#ifdef IS_A_DEVELOPMENT_BUILD
    Log::Instance()->addOutput(LOG_CONSOLE);
#endif

    Log::Instance()->create(_device->getSceneManager()->getFileSystem(), QSTRING_TO_PATH(QCoreApplication::applicationDirPath() + "/debug.log"));
}

void QIrrlichtWidget::initNormalsMaterial()
{
    io::path vsFileName = "shaders/normals.vert";
    io::path psFileName = "shaders/normals.frag";

    _normalsMaterial = new NormalsDebuggerShaderCallBack();
    _normalsMaterial->SetDevice(_device);

    video::IGPUProgrammingServices* gpu = _device->getVideoDriver()->getGPUProgrammingServices();
    _normalsMaterialType = gpu->addHighLevelShaderMaterialFromFiles(
                vsFileName, "vertexMain", video::EVST_VS_1_1,
                psFileName, "pixelMain", video::EPST_PS_1_1,
                _normalsMaterial, video::EMT_SOLID, 0);

    if (_normalsMaterialType == -1)
    {
        QMessageBox::critical(this, "Shader error" , "Compilation of the shaders has failed. 'Display/Normals' will be broken.");
    }
    else
    {
        core::array<video::SOverrideMaterial::SMaterialTypeReplacement> overrideMaterialTypes;
        overrideMaterialTypes.push_back(video::SOverrideMaterial::SMaterialTypeReplacement(-1, _normalsMaterialType));
        _device->getVideoDriver()->getOverrideMaterial().MaterialTypes = overrideMaterialTypes;
    }
}

io::IFileSystem *QIrrlichtWidget::getFileSystem()
{
    return _device->getFileSystem();
}

void QIrrlichtWidget::updateIrrlicht(QIrrlichtWidget* irrWidget)
{
    if (_inLoading)
        return;

    if (_device)
    {
        _device->getTimer()->tick ();

        _device->getVideoDriver()->beginScene(true, true, QCOLOR_TO_IRRCOLOR(Settings::_backgroundColor));

        if (_normalsRendererEnabled)
        {
            _device->getVideoDriver()->getOverrideMaterial().EnablePasses = scene::ESNRP_SKY_BOX + scene::ESNRP_SOLID + scene::ESNRP_TRANSPARENT + scene::ESNRP_TRANSPARENT_EFFECT + scene::ESNRP_SHADOW;
        }
        else
        {
            _device->getVideoDriver()->getOverrideMaterial().EnablePasses = 0;
        }

        _device->getSceneManager()->drawAll();
        _device->getVideoDriver()->endScene();
    }
}

void QIrrlichtWidget::paintEvent(QPaintEvent* event)
{
    if (_device)
    {
        /* lorsque le widget demande a être affiché, on emet le signal updateIrrlichtQuery (),
         * ainsi, notre slot updateIrrlicht () sera appelé.
         */
        emit updateIrrlichtQuery (this);
    }
}

void QIrrlichtWidget::timerEvent(QTimerEvent* event)
{
    if (_device)
    {
        // pareil que pour la méthode paintEvent ()
        emit updateIrrlichtQuery (this);
    }

    event->accept ();
}

void QIrrlichtWidget::resizeEvent(QResizeEvent* event)
{
    if (_device)
    {
        // lors d'un redimensionnement, on récupe la nouvelle taille du widget
        core::dimension2d<u32> widgetSize;

        widgetSize.Width  = event->size().width();
        widgetSize.Height = event->size().height();

        // et on précise à Irrlicht la nouvelle taille.
        _device->getVideoDriver()->OnResize(widgetSize);

        // update aspect ratio
        const f32 aspectRatio = static_cast<float>(event->size().width()) / event->size().height();
        _camera->setAspectRatio(aspectRatio);
     }

    QWidget::resizeEvent(event);
}

void QIrrlichtWidget::keyPressEvent(QKeyEvent* event)
{
    if (_device == nullptr)
        return;

    SEvent irrEvent;
    irrEvent.EventType = EET_KEY_INPUT_EVENT;

    irrEvent.KeyInput.PressedDown = true;
    irrEvent.KeyInput.Key = (EKEY_CODE)QKEY_TO_IRRKEY(event->key());

    if (_device->postEventFromUser( irrEvent ))
        event->accept();
}

void QIrrlichtWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (_device == nullptr)
        return;

    SEvent irrEvent;
    irrEvent.EventType = EET_KEY_INPUT_EVENT;

    irrEvent.KeyInput.PressedDown = false;
    irrEvent.KeyInput.Key = (EKEY_CODE)QKEY_TO_IRRKEY(event->key());

    if (_device->postEventFromUser( irrEvent ))
        event->accept();
}

void QIrrlichtWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (_device == nullptr)
        return;

    SEvent irrEvent;
    irrEvent.EventType = EET_MOUSE_INPUT_EVENT;

    irrEvent.MouseInput.Event = irr::EMIE_MOUSE_MOVED;
    irrEvent.MouseInput.X = _device->getCursorControl()->getPosition().X;
    irrEvent.MouseInput.Y = _device->getCursorControl()->getPosition().Y;

    irrEvent.MouseInput.Wheel = 0.0f; // Zero is better than undefined


    u32 buttonState = 0;
    if (QApplication::mouseButtons () & Qt::LeftButton) {
        buttonState |= EMBSM_LEFT;
    }
    if (QApplication::mouseButtons () & Qt::RightButton) {
        buttonState |= EMBSM_RIGHT;
    }
    if (QApplication::mouseButtons () & Qt::MiddleButton) {
        buttonState |= EMBSM_MIDDLE;
    }
    irrEvent.MouseInput.ButtonStates = buttonState;

    if (_device->postEventFromUser( irrEvent ))
        event->accept();

}

void QIrrlichtWidget::mousePressEvent( QMouseEvent* event )
{
    if (!_device)
        return;

    // If there is a mouse event, we should report it to Irrlicht for the Maya camera
    irr::SEvent irrEvent;
    irrEvent.EventType = irr::EET_MOUSE_INPUT_EVENT;

    if (event->button() == Qt::LeftButton)
        irrEvent.MouseInput.Event = irr::EMIE_LMOUSE_PRESSED_DOWN;
    else if (event->button() == Qt::RightButton)
        irrEvent.MouseInput.Event = irr::EMIE_RMOUSE_PRESSED_DOWN;
    else if (event->button() == Qt::MiddleButton)
        irrEvent.MouseInput.Event = irr::EMIE_MMOUSE_PRESSED_DOWN;

    irrEvent.MouseInput.X = _device->getCursorControl()->getPosition().X;
    irrEvent.MouseInput.Y = _device->getCursorControl()->getPosition().Y;
    irrEvent.MouseInput.Wheel = 0.0f; // Zero is better than undefined

    if(_device->postEventFromUser( irrEvent ))
        event->accept();
}

void QIrrlichtWidget::mouseReleaseEvent( QMouseEvent* event )
{
    if (!_device)
        return;

    // If there is a mouse event, we should report it to Irrlicht for the Maya camera
    irr::SEvent irrEvent;
    irrEvent.EventType = irr::EET_MOUSE_INPUT_EVENT;

    if (event->button() == Qt::LeftButton)
        irrEvent.MouseInput.Event = irr::EMIE_LMOUSE_LEFT_UP;
    else if (event->button() == Qt::RightButton)
        irrEvent.MouseInput.Event = irr::EMIE_RMOUSE_LEFT_UP;
    else if (event->button() == Qt::MiddleButton)
        irrEvent.MouseInput.Event = irr::EMIE_MMOUSE_LEFT_UP;

    irrEvent.MouseInput.X = _device->getCursorControl()->getPosition().X;
    irrEvent.MouseInput.Y = _device->getCursorControl()->getPosition().Y;
    irrEvent.MouseInput.Wheel = 0.0f; // Zero is better than undefined

    if(_device->postEventFromUser( irrEvent ))
        event->accept();
}

bool QIrrlichtWidget::isLoadableByIrrlicht(io::path filename)
{
    for (u32 i = 0; i < _device->getSceneManager()->getMeshLoaderCount(); ++i)
    {
        if (_device->getSceneManager()->getMeshLoader(i)->isALoadableFileExtension(filename))
            return true;
    }
    return false;
}

void setMaterialsSettings(scene::IAnimatedMeshSceneNode* node)
{
    // materials with normal maps are not handled
    for (u32 i = 0; i < node->getMaterialCount(); ++i)
    {
        video::SMaterial& material = node->getMaterial(i);
        if (    material.MaterialType == video::EMT_NORMAL_MAP_SOLID
            ||  material.MaterialType == video::EMT_PARALLAX_MAP_SOLID)
        {
            material.MaterialType = video::EMT_SOLID;
        }
        else if (material.MaterialType == video::EMT_NORMAL_MAP_TRANSPARENT_ADD_COLOR
            ||   material.MaterialType == video::EMT_PARALLAX_MAP_TRANSPARENT_ADD_COLOR)
        {
            material.MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;
        }
        else if (material.MaterialType == video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA
            ||   material.MaterialType == video::EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA)
        {
            material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
        }
    }

    node->setMaterialFlag(video::EMF_LIGHTING, false);
    node->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);

    for (u32 i = 1; i < _IRR_MATERIAL_MAX_TEXTURES_; ++i)
        node->setMaterialTexture(i, nullptr);
}

bool QIrrlichtWidget::loadAnims(const io::path filename, core::stringc &feedbackMessage)
{
    io::IReadFile* file = _device->getFileSystem()->createAndOpenFile(filename);
    if (!file)
    {
        feedbackMessage = "Error : The file can't be opened.";
        return false;
    }

    scene::IO_MeshLoader_W3ENT loader(_device->getSceneManager(), _device->getFileSystem());

    scene::ISkinnedMesh* newMesh = copySkinnedMesh(_device->getSceneManager(), _currentLodData->_node->getMesh(), true);

    // use the loader to add the animation to the new model
    loader.meshToAnimate = newMesh;
    scene::IAnimatedMesh* mesh = loader.createMesh(file);
    file->drop();

    if (mesh)
        mesh->drop();


    newMesh->setDirty();
    newMesh->finalize();

    _currentLodData->_node->setMesh(newMesh);

    setMaterialsSettings(_currentLodData->_node);
    return true;

}

bool QIrrlichtWidget::loadTW1Anims(const io::path filename, core::stringc &feedbackMessage)
{
    io::IReadFile* file = _device->getFileSystem()->createAndOpenFile(filename);
    if (!file)
    {
        feedbackMessage = "Error : The file can't be opened.";
        return false;
    }


    IO_MeshLoader_WitcherMDL loader(_device->getSceneManager(), _device->getFileSystem());

    scene::ISkinnedMesh* newMesh = copySkinnedMesh(_device->getSceneManager(), _currentLodData->_node->getMesh(), true);

    // use the loader to add the animation to the new model
    loader.meshToAnimate = newMesh;
    scene::IAnimatedMesh* mesh = loader.createMesh(file);
    file->drop();

    _currentLodData->_node->setMesh(newMesh);

    setMaterialsSettings(_currentLodData->_node);

    return true;
}

bool QIrrlichtWidget::loadRig(const io::path filename, core::stringc &feedbackMessage)
{
    io::IReadFile* file = _device->getFileSystem()->createAndOpenFile(filename);
    if (!file)
    {
        feedbackMessage = "Error : The file can't be opened.";
        return false;
    }

    scene::IO_MeshLoader_W3ENT loader(_device->getSceneManager(), _device->getFileSystem());
    scene::IAnimatedMesh* mesh = loader.createMesh(file);
    file->drop();

    if (mesh)
        mesh->drop();

    TW3_CSkeleton skeleton = loader.Skeleton;

    scene::ISkinnedMesh* newMesh = copySkinnedMesh(_device->getSceneManager(), _currentLodData->_node->getMesh(), false);

    bool success = skeleton.applyToModel(newMesh);
    if (success)
        feedbackMessage = "Rig successfully applied";
    else
        feedbackMessage = "The skeleton can't be applied to the model. Are you sure that you have selected the good w2rig file ?";

    // Apply the skinning
    TW3_DataCache::_instance.setOwner(newMesh);
    TW3_DataCache::_instance.apply();

    newMesh->setDirty();
    newMesh->finalize();

    _currentLodData->_node->setMesh(newMesh);

    setMaterialsSettings(_currentLodData->_node);
    return success;
}

bool QIrrlichtWidget::loadTheCouncilTemplate(const io::path filename, core::stringc &feedbackMessage)
{
    _device->getSceneManager()->getParameters()->setAttribute("TW_DEBUG_LOG", Settings::_debugLog);
    _device->getSceneManager()->getParameters()->setAttribute("TW_GAME_PATH", cleanPath(Settings::_baseDir).toStdString().c_str());

    bool success = _device->getSceneManager()->loadScene(filename);
    if (success)
        feedbackMessage = "Template sucessfully loaded";
    else
        feedbackMessage = "Fail to load template";

    return success;
}

bool QIrrlichtWidget::fileIsOpenableByIrrlicht(QString filename)
{
    const io::path irrFilename = QSTRING_TO_PATH(filename);

    io::IReadFile* file = _device->getFileSystem()->createAndOpenFile(irrFilename);
    if (!file)
        return false;

    file->drop();
    return true;
}

void QIrrlichtWidget::loadMeshPostProcess()
{
    const scene::IAnimatedMesh* mesh = _currentLodData->_node->getMesh();

    MeshSize::_scaleFactor = 1.f;

    // Save the path of normals/specular maps
    _currentLodData->_additionalTextures.resize(mesh->getMeshBufferCount());
    for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        const video::SMaterial material = mesh->getMeshBuffer(i)->getMaterial();
        _currentLodData->_additionalTextures[i].resize(_IRR_MATERIAL_MAX_TEXTURES_-1);
        for (u32 j = 1; j < _IRR_MATERIAL_MAX_TEXTURES_; ++j)
        {
            QString texturePath = QString();
            const video::ITexture* texture = material.getTexture(j);
            if (texture)
                texturePath = PATH_TO_QSTRING(texture->getName().getPath());

            _currentLodData->_additionalTextures[i][j-1] = texturePath;
        }
    }

    setMaterialsSettings(_currentLodData->_node);

    // DEBUG
    /*
    std::cout << "Dimensions : x=" << mesh->getBoundingBox().MaxEdge.X - mesh->getBoundingBox().MinEdge.X
              << ", y=" << mesh->getBoundingBox().MaxEdge.Y - mesh->getBoundingBox().MinEdge.Y
              << ", z=" << mesh->getBoundingBox().MaxEdge.Z - mesh->getBoundingBox().MinEdge.Z << std::endl;
    */
}

scene::IAnimatedMesh* QIrrlichtWidget::loadMesh(QString filename, core::stringc &feedbackMessage)
{
    _device->getSceneManager()->getParameters()->setAttribute("TW_DEBUG_LOG", Settings::_debugLog);
    _device->getSceneManager()->getParameters()->setAttribute("TW_GAME_PATH", cleanPath(Settings::_baseDir).toStdString().c_str());
    _device->getSceneManager()->getParameters()->setAttribute("TW_TW3_TEX_PATH", cleanPath(Settings::_TW3TexPath).toStdString().c_str());
    _device->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_SKEL", Settings::_TW3LoadSkeletonEnabled);
    _device->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_BEST_LOD_ONLY", Settings::_TW3LoadBestLODEnabled);

    // Clear the previous data
    TW3_DataCache::_instance.clear();


    const io::path irrFilename = QSTRING_TO_PATH(filename);
    io::path extension;
    core::getFileNameExtension(extension, irrFilename);

    scene::IAnimatedMesh* mesh = nullptr;

#ifdef COMPILE_WITH_ASSIMP
    IrrAssimp assimp(_device->getSceneManager());
#endif

    if (isLoadableByIrrlicht(irrFilename))
    {
        mesh = _device->getSceneManager()->getMesh(irrFilename);

        // Witcher feedback
        if (extension == ".w2mesh" || extension == ".w2ent" || extension == ".w2rig")
        {
            feedbackMessage = _device->getSceneManager()->getParameters()->getAttributeAsString("TW_FEEDBACK");
            _device->getSceneManager()->getParameters()->setAttribute("TW_FEEDBACK", "");
        }

        if (!mesh && feedbackMessage == "")
        {
            feedbackMessage = "\nError : loading of the mesh failed for unknown reason.";
        }
    }
#ifdef COMPILE_WITH_ASSIMP
    else if (assimp.isLoadable(irrFilename))
    {
        mesh = assimp.getMesh(irrFilename);

        if (!mesh)
            feedbackMessage = assimp.getError();
    }
#endif
    else        // no mesh loader for this file
    {
        feedbackMessage = "\nError : No mesh loader found for this file. Are you sure that this file has an extension loadable by the software ? Check the website for more information.";
    }

    return mesh;
}

bool QIrrlichtWidget::addMesh(QString filename, core::stringc &feedbackMessage)
{
    _inLoading = true;

    scene::IAnimatedMesh* mesh = loadMesh(filename, feedbackMessage);

    // Leave here if there was a problem during the loading
    if (!mesh)
    {
        _inLoading = false;
        return false;
    }

    scene::ISkinnedMesh* newMesh = copySkinnedMesh(_device->getSceneManager(), _currentLodData->_node->getMesh(), true);
    combineMeshes(newMesh, mesh, true);
    newMesh->finalize();
    _currentLodData->_node->setMesh(newMesh);

    loadMeshPostProcess();

    // No error and no feedback message specified, we will simply say 'done'
    if (feedbackMessage == "")
        feedbackMessage = "done";

    _inLoading = false;
    return true;
}

bool QIrrlichtWidget::setMesh(QString filename, core::stringc &feedbackMessage)
{
    _inLoading = true;

    // Delete the current mesh
    clearLOD();

    scene::IAnimatedMesh* mesh = loadMesh(filename, feedbackMessage);

    // Leave here if there was a problem during the loading
    if (!mesh)
    {
        _inLoading = false;
        return false;
    }

    _currentLodData->_node = _device->getSceneManager()->addAnimatedMeshSceneNode(mesh);
    _currentLodData->_node->setScale(core::vector3df(20, 20, 20));
    _currentLodData->_node->setRotation(core::vector3df(_currentLodData->_node->getRotation().X, _currentLodData->_node->getRotation().Y - 90, _currentLodData->_node->getRotation().Z));

    // for debug only
    // loadedNode->setDebugDataVisible(EDS_BBOX_ALL);

    _camera->setTarget(_currentLodData->_node->getPosition());
    loadMeshPostProcess();

    // No error and no feedback message specified, we will simply say 'done'
    if (feedbackMessage == "")
        feedbackMessage = "done";

    _inLoading = false;
    return true;
}

void scaleSkeleton(scene::IMesh* mesh, float factor)
{
    if (mesh->getMeshType() != scene::EAMT_SKINNED)
        return;

    const scene::ISkinnedMesh* skinnedMesh = (scene::ISkinnedMesh*)mesh;
    const u32 nbJoints = skinnedMesh->getJointCount();
    for (u32 i = 0; i < nbJoints; ++i)
    {
        scene::ISkinnedMesh::SJoint* joint = skinnedMesh->getAllJoints()[i];
        joint->Animatedposition *= factor;
    }
}

void QIrrlichtWidget::exportMesh(QString exportFolderPath, QString filename, ExporterInfos exporter, core::stringc &feedbackMessage)
{
    if (exporter._exporterType != Exporter_Redkit && (!_currentLodData->_node || !_currentLodData->_node->getMesh()))
    {
        QMessageBox::critical(this, "Export error", "QIrrlichtWidget::writeFile : node or mesh in null");
        return;
    }


    if (Settings::_copyTexturesEnabled)
    {
        // Will be exported in a subfolder
        exportFolderPath = exportFolderPath + filename + "_export/";
        QDir dir;
        dir.mkdir(exportFolderPath);
    }

    const io::path exportMeshPath = QSTRING_TO_PATH(exportFolderPath + filename + exporter._extension);
    io::IWriteFile* file = _device->getFileSystem()->createAndWriteFile(exportMeshPath);
    if (!file)
    {
        feedbackMessage = "fail. Can't create the exported file";
        return;
    }

    if (exporter._exporterType != Exporter_Redkit)
    {
        convertAndCopyTextures(_currentLodData->_node->getMesh(), exportFolderPath, Settings::_copyTexturesEnabled);
        convertAndCopyTextures(_currentLodData->getTexturesSetForLayer(1), exportFolderPath, Settings::_copyTexturesEnabled && Settings::_copyTexturesSlot1);
        convertAndCopyTextures(_currentLodData->getTexturesSetForLayer(2), exportFolderPath, Settings::_copyTexturesEnabled && Settings::_copyTexturesSlot2);
    }
    else
    {
        // TODO: merge the sets of the LODs and call copyTextures once to avoid to copy the same texture many times
        if (_lod0Data._node)
        {
            convertAndCopyTextures(_lod0Data._node->getMesh(), exportFolderPath, Settings::_copyTexturesEnabled);
            convertAndCopyTextures(_lod0Data.getTexturesSetForLayer(1), exportFolderPath, Settings::_copyTexturesEnabled && Settings::_copyTexturesSlot1);
            convertAndCopyTextures(_lod0Data.getTexturesSetForLayer(2), exportFolderPath, Settings::_copyTexturesEnabled && Settings::_copyTexturesSlot2);
        }
        if (_lod1Data._node)
        {
            convertAndCopyTextures(_lod1Data._node->getMesh(), exportFolderPath, Settings::_copyTexturesEnabled);
            convertAndCopyTextures(_lod1Data.getTexturesSetForLayer(1), exportFolderPath, Settings::_copyTexturesEnabled && Settings::_copyTexturesSlot1);
            convertAndCopyTextures(_lod1Data.getTexturesSetForLayer(2), exportFolderPath, Settings::_copyTexturesEnabled && Settings::_copyTexturesSlot2);
        }
        if (_lod2Data._node)
        {
            convertAndCopyTextures(_lod2Data._node->getMesh(), exportFolderPath, Settings::_copyTexturesEnabled);
            convertAndCopyTextures(_lod2Data.getTexturesSetForLayer(1), exportFolderPath, Settings::_copyTexturesEnabled && Settings::_copyTexturesSlot1);
            convertAndCopyTextures(_lod2Data.getTexturesSetForLayer(2), exportFolderPath, Settings::_copyTexturesEnabled && Settings::_copyTexturesSlot2);
        }
    }

    //std::cout << filename.toStdString().c_str() << std::endl;

    float scaleFactor = MeshSize::_scaleFactor;
    // Set the export size
    if (scaleFactor != 1.f)
    {
        const core::vector3df scaleFactorVector = core::vector3df(scaleFactor, scaleFactor, scaleFactor);
        if (_lod0Data._node)
        {
            _device->getSceneManager()->getMeshManipulator()->scale(_lod0Data._node->getMesh(), scaleFactorVector);
            scaleSkeleton(_lod0Data._node->getMesh(), scaleFactor);
        }
        if (_lod1Data._node)
        {
            _device->getSceneManager()->getMeshManipulator()->scale(_lod1Data._node->getMesh(), scaleFactorVector);
            scaleSkeleton(_lod1Data._node->getMesh(), scaleFactor);
        }
        if (_lod2Data._node)
        {
            _device->getSceneManager()->getMeshManipulator()->scale(_lod2Data._node->getMesh(), scaleFactorVector);
            scaleSkeleton(_lod2Data._node->getMesh(), scaleFactor);
        }
        if (_collisionsLodData._node)
        {
            _device->getSceneManager()->getMeshManipulator()->scale(_collisionsLodData._node->getMesh(), scaleFactorVector);
            scaleSkeleton(_collisionsLodData._node->getMesh(), scaleFactor);
        }
    }

    if (exporter._exporterType == Exporter_Irrlicht)
    {
        scene::IMeshWriter* mw = nullptr;
        mw = _device->getSceneManager()->createMeshWriter(exporter._irrlichtInfos._irrExporter);

        if (mw)
        {
            mw->writeMesh(file, _currentLodData->_node->getMesh(), exporter._irrlichtInfos._irrFlags);
            mw->drop();
        }
    }
    else if (exporter._exporterType == Exporter_Redkit)
    {
        _reWriter->clearLODS();
        if (_lod1Data._node)
            _reWriter->setLOD1(_lod1Data._node->getMesh());
        if (_lod2Data._node)
            _reWriter->setLOD2(_lod2Data._node->getMesh());
        if (_collisionsLodData._node)
        {
            _reWriter->setCollisionMesh(_collisionsLodData._node->getMesh());
        }
        _reWriter->writeMesh(file, _lod0Data._node->getMesh());
    }
    else
    {
#ifdef COMPILE_WITH_ASSIMP
        IrrAssimp assimp(_device->getSceneManager());
        assimp.exportMesh(_currentLodData->_node->getMesh(), exporter._assimpExporterId.toStdString().c_str(), exportFolderPath.toStdString().c_str());
#else
        QMessageBox::critical(this, "Export error", "COMPILE_WITH_ASSIMP is not enabled, this export isn't available");
#endif
    }

    // Reset the scale
    if (scaleFactor != 1.f)
    {
        const f32 scaleFactorInverse = 1.0f / scaleFactor;
        const core::vector3df scaleVectorInverseVector = core::vector3df(scaleFactorInverse, scaleFactorInverse, scaleFactorInverse);
        if (_lod0Data._node)
        {
            _device->getSceneManager()->getMeshManipulator()->scale(_lod0Data._node->getMesh(), scaleVectorInverseVector);
            scaleSkeleton(_lod0Data._node->getMesh(), scaleFactorInverse);
        }

        if (_lod1Data._node)
        {
            _device->getSceneManager()->getMeshManipulator()->scale(_lod1Data._node->getMesh(), scaleVectorInverseVector);
            scaleSkeleton(_lod1Data._node->getMesh(), scaleFactorInverse);
        }

        if (_lod2Data._node)
        {
            _device->getSceneManager()->getMeshManipulator()->scale(_lod2Data._node->getMesh(), scaleVectorInverseVector);
            scaleSkeleton(_lod2Data._node->getMesh(), scaleFactorInverse);
        }
        if (_collisionsLodData._node)
        {
            _device->getSceneManager()->getMeshManipulator()->scale(_collisionsLodData._node->getMesh(), scaleVectorInverseVector);
            scaleSkeleton(_collisionsLodData._node->getMesh(), scaleFactorInverse);
        }
    }

    if (file)
        file->drop();

    feedbackMessage = "done";
}


void QIrrlichtWidget::enableWireframe(bool enabled)
{
    if (_currentLodData->_node)
        _currentLodData->_node->setMaterialFlag(video::EMF_WIREFRAME, enabled);
}

void QIrrlichtWidget::enableRigging(bool enabled)
{
    if (!_currentLodData->_node)
        return;

    if (enabled)
        _currentLodData->_node->setDebugDataVisible(scene::EDS_SKELETON);
    else
        _currentLodData->_node->setDebugDataVisible(scene::EDS_OFF);

}

void QIrrlichtWidget::enableNormals(bool enabled)
{
    _normalsRendererEnabled = enabled;
}

unsigned int QIrrlichtWidget::getPolysCount()
{
    if (_currentLodData->_node)
        return _device->getSceneManager()->getMeshManipulator()->getPolyCount(static_cast<scene::IMesh*>(_currentLodData->_node->getMesh()));
    return 0;
}

unsigned int QIrrlichtWidget::getJointsCount()
{
    if (_currentLodData->_node)
        return _currentLodData->_node->getJointCount();
    return 0;
}

core::vector3df QIrrlichtWidget::getMeshDimensions()
{
    if (_currentLodData->_node)
        return _currentLodData->_node->getMesh()->getBoundingBox().MaxEdge - _currentLodData->_node->getMesh()->getBoundingBox().MinEdge;
    return core::vector3df(0.f, 0.f, 0.f);
}

void QIrrlichtWidget::changeOptions()
{
    core::list<scene::ISceneNodeAnimator*> anims = _camera->getAnimators();
    core::list<scene::ISceneNodeAnimator*>::Iterator it;
    for (it = anims.begin(); it != anims.end(); it++)
    {
        if ((*it)->getType() == scene::ESNAT_CAMERA_MAYA)
        {
            break;
        }
    }
    scene::ISceneNodeAnimatorCameraMaya* anim = (scene::ISceneNodeAnimatorCameraMaya*)(*it);
    anim->setMoveSpeed(Settings::_cameraSpeed);
    anim->setRotateSpeed(Settings::_cameraRotationSpeed);
}

void QIrrlichtWidget::changeLOD(LOD newLOD)
{
    if (_currentLodData->_node)
        _currentLodData->_node->setVisible(false);

    _currentLOD = newLOD;


    switch (_currentLOD)
    {
        case LOD_0:
            _currentLodData = &_lod0Data;
        break;
        case LOD_1:
            _currentLodData = &_lod1Data;
        break;
        case LOD_2:
            _currentLodData = &_lod2Data;
        break;
        case Collision:
            _currentLodData = &_collisionsLodData;
        break;
    }


    if (!_currentLodData->_node)
        return;

    _currentLodData->_node->setVisible(true);
}

void QIrrlichtWidget::clearLOD()
{
    // Delete the current mesh
    _currentLodData->clearLodData();

    _device->getSceneManager()->getMeshCache()->clearUnusedMeshes();

    if (!_lod0Data._node && !_lod1Data._node && !_lod2Data._node && !_collisionsLodData._node)
    {
        _device->getVideoDriver()->removeAllTextures();
        _device->getVideoDriver()->removeAllHardwareBuffers();
    }
}

void QIrrlichtWidget::clearAllLODs()
{
    _lod0Data.clearLodData();
    _lod1Data.clearLodData();
    _lod2Data.clearLodData();
    _collisionsLodData.clearLodData();

    _device->getSceneManager()->getMeshCache()->clearUnusedMeshes();//->clear();

    _device->getVideoDriver()->removeAllTextures();
    _device->getVideoDriver()->removeAllHardwareBuffers();
}

// Convert and copy a single texture
bool QIrrlichtWidget::convertAndCopyTexture(QString texturePath, QString exportFolder, bool shouldCopyTextures, QString& outputTexturePath)
{
    QFileInfo pathInfo(texturePath);
    if (!pathInfo.exists())
        return false; // TODO: Log something in this case ?

    if (Settings::_convertTexturesEnabled) // Convert and generate the new file in the export folder
    {
        video::IImage* image = _device->getVideoDriver()->createImageFromFile(QSTRING_TO_PATH(texturePath));
        if (image)
        {
            outputTexturePath = exportFolder + pathInfo.baseName() + Settings::_convertTexturesFormat;
            if (!shouldCopyTextures) // we convert the texture but we keep it in it's original folder
                outputTexturePath = pathInfo.absolutePath() + '\\' + pathInfo.baseName() + Settings::_convertTexturesFormat;

            _device->getVideoDriver()->writeImageToFile(image, QSTRING_TO_PATH(outputTexturePath));
            image->drop();
        }
    }
    else if (shouldCopyTextures) // We just have to copy the original texture file in this case
    {
        outputTexturePath = exportFolder + pathInfo.fileName();
        QFile::copy(texturePath, outputTexturePath);
    }
    return true;
}

// convert and copy the diffuse textures of a mesh
void QIrrlichtWidget::convertAndCopyTextures(scene::IMesh* mesh, QString exportFolder, bool shouldCopyTextures)
{
    for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        scene::IMeshBuffer* buffer = mesh->getMeshBuffer(i);
        video::ITexture* diffuseTexture = buffer->getMaterial().getTexture(0);
        if (diffuseTexture)
        {
            QString texturePath = PATH_TO_QSTRING(diffuseTexture->getName().getPath());
            QString outputTexturePath;
            if (convertAndCopyTexture(texturePath, exportFolder, shouldCopyTextures, outputTexturePath)) // TODO: Log something if file not exist ?
            {
                // We apply the nex texture to the mesh, so the exported file will use it
                // TODO: Restore the original texture on the mesh after the export ?
                video::ITexture* tex = _device->getSceneManager()->getVideoDriver()->getTexture(QSTRING_TO_PATH(outputTexturePath));
                buffer->getMaterial().setTexture(0, tex);
            }
        }
    }
}

// convert and copy a list of textures
void QIrrlichtWidget::convertAndCopyTextures(QSet<QString> paths, QString exportFolder, bool shouldCopyTextures)
{
    QSet<QString>::iterator it;
    for (it = paths.begin(); it != paths.end(); ++it)
    {
        QString texturePath = (*it);
        QFileInfo pathInfo(texturePath);

        QString outputTexturePath;
        convertAndCopyTexture(texturePath, exportFolder, shouldCopyTextures, outputTexturePath);
    }
}

QString QIrrlichtWidget::getFilename()
{
    if (_currentLodData->_node)
        return PATH_TO_QSTRING(_device->getFileSystem()->getFileBasename(_device->getSceneManager()->getMeshCache()->getMeshFilename(_currentLodData->_node->getMesh())));
    else
        return "";
}

QString QIrrlichtWidget::getPath()
{
    if (_currentLodData->_node)
        return PATH_TO_QSTRING(_device->getFileSystem()->getAbsolutePath(_device->getSceneManager()->getMeshCache()->getMeshFilename(_currentLodData->_node->getMesh())));
    else
        return "";
}

bool QIrrlichtWidget::isEmpty(LOD lod)
{
    switch (lod)
    {
        case LOD_0:
            return (_lod0Data._node == nullptr);
        break;
        case LOD_1:
            return (_lod1Data._node == nullptr);
        break;
        case LOD_2:
            return (_lod2Data._node == nullptr);
        break;
        case Collision:
            return (_collisionsLodData._node == nullptr);
        break;
    }
    return false;
}
