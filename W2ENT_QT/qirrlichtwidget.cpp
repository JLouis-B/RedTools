#include "QIrrlichtWidget.h"
#include <iostream>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;

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
    node->setMaterialFlag(EMF_LIGHTING, false);
    node->setMaterialType(EMT_SOLID);
    node->setMaterialFlag(EMF_BACK_FACE_CULLING, false);

    for (u32 i = 1; i < _IRR_MATERIAL_MAX_TEXTURES_; ++i)
        node->setMaterialTexture(i, 0);
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

    bool sucess = skeleton.applyToModel(newMesh);
    if (sucess)
        feedbackMessage = "Rig sucessfully applied";
    else
        feedbackMessage = "The skeleton can't be applied to the model. Are you sure that you have selected the good w2rig file ?";

    // Apply the skinning
    TW3_DataCache::_instance.setOwner(newMesh);
    TW3_DataCache::_instance.apply();

    newMesh->setDirty();
    newMesh->finalize();

    _currentLodData->_node->setMesh(newMesh);

    setMaterialsSettings(_currentLodData->_node);
    return sucess;
}

QIrrlichtWidget::QIrrlichtWidget (QWidget *parent) :
    QWidget (parent),
    _device (NULL)
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
}

QIrrlichtWidget::~QIrrlichtWidget ()
{
    if (_device)
    {
        _device->closeDevice ();
        _device->drop ();
    }
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

    GUI_Resize::_originalDimensions = (mesh->getBoundingBox().MaxEdge - mesh->getBoundingBox().MinEdge);
    GUI_Resize::_dimensions = GUI_Resize::_originalDimensions;


    // Save the path of normals/specular maps
    for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        const video::SMaterial mat = mesh->getMeshBuffer(i)->getMaterial();
        for (u32 i = 1; i < _IRR_MATERIAL_MAX_TEXTURES_; ++i)
        {
            if(mat.getTexture(i))
                _currentLodData->_additionalTextures[i].insert(mat.getTexture(i)->getName().getPath());
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

IAnimatedMesh* QIrrlichtWidget::loadMesh(QString filename, stringc &feedbackMessage)
{
    if (Settings::_pack0[Settings::_pack0.size() - 1] != '/')
        Settings::_pack0.push_back('/');

    if (Settings::_TW3TexPath[Settings::_TW3TexPath.size() - 1] != '/')
        Settings::_TW3TexPath.push_back('/');

    _device->getSceneManager()->getParameters()->setAttribute("TW_DEBUG_LOG", Settings::_debugLog);
    _device->getSceneManager()->getParameters()->setAttribute("TW_GAME_PATH", Settings::_pack0.toStdString().c_str());
    _device->getSceneManager()->getParameters()->setAttribute("TW_TW3_TEX_PATH", Settings::_TW3TexPath.toStdString().c_str());
    _device->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_SKEL", Settings::_TW3LoadSkel);
    _device->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_BEST_LOD_ONLY", Settings::_TW3LoadBestLOD);

    // Clear the previous data
    TW3_DataCache::_instance.clear();


    const io::path irrFilename = QSTRING_TO_PATH(filename);
    io::path extension;
    core::getFileNameExtension(extension, irrFilename);

    IAnimatedMesh* mesh = 0;
    IrrAssimp assimp(_device->getSceneManager());
    //l.addAndFlush("irrassimp loaded\n");
    if (isLoadableByIrrlicht(irrFilename))
    {
        //l.addAndFlush("irr loader\n");
        mesh = _device->getSceneManager()->getMesh(irrFilename);
        //l.addAndFlush("getMesh\n");

        // Witcher feedback
        if (extension == ".w2mesh" || extension == ".w2ent" || extension == ".w2rig")
        {
            feedbackMessage = _device->getSceneManager()->getParameters()->getAttributeAsString("TW_FEEDBACK");
            _device->getSceneManager()->getParameters()->setAttribute("TW_FEEDBACK", "");
            //l.addAndFlush("feedback is ");
            //l.addAndFlush(feedbackMessage.c_str());
        }

        if (!mesh && feedbackMessage == "")
        {
            feedbackMessage = "\nError : loading of the mesh failed for unknown reason.";
        }
    }
    else if (assimp.isLoadable(irrFilename))
    {
        mesh = assimp.getMesh(irrFilename);

        if (!mesh)
            feedbackMessage = assimp.getError();
    }
    else        // no mesh loader for this file
    {
        feedbackMessage = "\nError : No mesh loader found for this file. Are you sure that this file has an extension loadable by the software ? Check the website for more information.";
    }

    return mesh;
}

bool QIrrlichtWidget::addMesh(QString filename, stringc &feedbackMessage)
{
    _inLoading = true;

    IAnimatedMesh* mesh = loadMesh(filename, feedbackMessage);

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

bool QIrrlichtWidget::setModel(QString filename, stringc &feedbackMessage)
{
    _inLoading = true;

    // Delete the current mesh
    clearLOD();

    IAnimatedMesh* mesh = loadMesh(filename, feedbackMessage);

    // Leave here if there was a problem during the loading
    if (!mesh)
    {
        _inLoading = false;
        return false;
    }

    _currentLodData->_node = _device->getSceneManager()->addAnimatedMeshSceneNode(mesh);
    _currentLodData->_node->setScale(irr::core::vector3df(20, 20, 20));
    _currentLodData->_node->setRotation(irr::core::vector3df(_currentLodData->_node->getRotation().X, _currentLodData->_node->getRotation().Y - 90, _currentLodData->_node->getRotation().Z));

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

void QIrrlichtWidget::init ()
{
    SIrrlichtCreationParameters params;

    // on utilise OpenGL, et on lui donne l'identifiant de la fenêtre de notre widget
    params.DriverType        = EDT_OPENGL;
    params.WindowId          = reinterpret_cast<void*> (winId ());

    // ainsi que la taille de notre widget
    params.WindowSize.Width  = width ();
    params.WindowSize.Height = height ();



    params.AntiAlias         = true;
    params.Bits              = 16;
    params.HighPrecisionFPU  = false;
    params.Stencilbuffer     = true;
    params.Vsync             = true;

    // enfin, on initialise notre moteur de rendu
    _device = createDeviceEx (params);
    _device->getSceneManager()->getParameters()->setAttribute("TW_FEEDBACK", "");

    /* une fois initialisé, on émet le signal onInit, c'est la que nous
     * ajouterons nos modèles et nos textures.
     */
    emit onInit (this);


    setMouseTracking(true);

    if (_device)
    {
        // on créé une caméra pour visualiser la scène

        _camera = _device->getSceneManager()->addCameraSceneNodeMaya(0, Settings::_camRotSpeed, 100, Settings::_camSpeed, -1, 50);
        _camera->setPosition(vector3df (0,30,-40));
        _camera->setTarget(vector3df (0,0,0));
        const f32 aspectRatio = (float)width () / (float)height();
        _camera->setAspectRatio(aspectRatio);
        _camera->setFarValue(10000.f);

        _reWriter = new IO_MeshWriter_RE(_device->getSceneManager(), _device->getFileSystem());

        _device->getSceneManager()->addExternalMeshLoader(new IO_MeshLoader_WitcherMDL(_device->getSceneManager(), _device->getFileSystem()));
        _device->getSceneManager()->addExternalMeshLoader(new IO_MeshLoader_RE(_device->getSceneManager(), _device->getFileSystem()));
        _device->getSceneManager()->addExternalMeshLoader(new IO_MeshLoader_W2ENT(_device->getSceneManager(), _device->getFileSystem()));
        _device->getSceneManager()->addExternalMeshLoader(new IO_MeshLoader_W3ENT(_device->getSceneManager(), _device->getFileSystem()));

        //_device->getSceneManager()->setAmbientLight(irr::video::SColor(255,255,255,255));
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

    // debug only !
    //Log::Instance()->addOutput(LOG_CONSOLE);

    Log::Instance()->create(_device->getSceneManager()->getFileSystem(), QSTRING_TO_PATH(QCoreApplication::applicationDirPath() + "/debug.log"));
}

void QIrrlichtWidget::updateIrrlicht (QIrrlichtWidget *irrWidget)
{
    if (_inLoading)
        return;

    if (_device)
    {
        // le rendu est donc fait ici même

        _device->getTimer ()->tick ();

        SColor color  (255, Settings::_r, Settings::_g, Settings::_b);

        _device->getVideoDriver ()->beginScene (true, true, color);
        _device->getSceneManager ()->drawAll ();
        _device->getVideoDriver ()->endScene ();
    }
}

void QIrrlichtWidget::paintEvent (QPaintEvent *ev)
{
    if (_device)
    {
        /* lorsque le widget demande a être affiché, on emet le signal updateIrrlichtQuery (),
         * ainsi, notre slot updateIrrlicht () sera appelé.
         */
        emit updateIrrlichtQuery (this);
    }
}

void QIrrlichtWidget::timerEvent (QTimerEvent *ev)
{
    if (_device)
    {
        // pareil que pour la méthode paintEvent ()
        emit updateIrrlichtQuery (this);
    }

    ev->accept ();
}

void QIrrlichtWidget::resizeEvent (QResizeEvent *ev)
{
    if (_device)
    {
        // lors d'un redimensionnement, on récupe la nouvelle taille du widget
        dimension2d<u32> widgetSize;

        widgetSize.Width  = ev->size ().width ();
        widgetSize.Height = ev->size ().height ();

        // et on précise à Irrlicht la nouvelle taille.
        _device->getVideoDriver ()->OnResize (widgetSize);

        // update aspect ratio
        const f32 aspectRatio = (float)ev->size ().width () / (float)ev->size ().height();
        _camera->setAspectRatio(aspectRatio);
     }

    QWidget::resizeEvent (ev);
}

void QIrrlichtWidget::mouseMoveEvent(QMouseEvent * event)
{
    if (_device == nullptr)
        return;

    irr::SEvent irrEvent;
    irrEvent.EventType = irr::EET_MOUSE_INPUT_EVENT;

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
    if (_device == 0)
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
    if (_device == 0)
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


QString QIrrlichtWidget::convertTexture(QString filename, QString destDir)
{
    if (!Settings::_convertTextures)
    {
        //std::cout << filename.toStdString().c_str() << " to " << destDir.toStdString().c_str() << std::endl;
        QFile::copy(filename, destDir);
        return filename;
    }


    IImage* image = _device->getVideoDriver()->createImageFromFile(QSTRING_TO_PATH(filename));
    if (image)
    {
        _device->getVideoDriver()->writeImageToFile(image, QSTRING_TO_PATH(destDir));
        image->drop();
    }
    return destDir;
}

void scaleSkeleton(scene::IMesh* mesh, float factor)
{
    if (mesh->getMeshType() != scene::EAMT_SKINNED)
        return;

    const scene::ISkinnedMesh* skinnedMesh = (scene::ISkinnedMesh*)mesh;
    const u32 nbJoints = skinnedMesh->getJointCount();
    for (u32 i = 0; i < nbJoints; ++i)
    {
        ISkinnedMesh::SJoint* joint = skinnedMesh->getAllJoints()[i];
        joint->Animatedposition *= factor;
    }
}

void QIrrlichtWidget::writeFile (QString exportFolder, QString filename, QString extension, core::stringc &feedbackMessage)
{
    if (!_currentLodData->_node && extension != ".re")
        return;


    if (Settings::_copyTextures)
    {
        // Will be exported in a subfolder
        exportFolder = exportFolder + "/" +  filename + "_export/";
        QDir dir;
        dir.mkdir(exportFolder);
    }

    const io::path exportPath = QSTRING_TO_PATH(exportFolder + "/" + filename + extension);
    IWriteFile* file = _device->getFileSystem()->createAndWriteFile(exportPath);
    if (!file)
    {
        feedbackMessage = "fail. Can't create the exported file";
        return;
    }

    if (extension != ".re")
    {
        copyTextures(_currentLodData->_node->getMesh(), exportFolder);
        if(Settings::_nm)
            copyTextures(_currentLodData->_additionalTextures[1], exportFolder);
        if(Settings::_sm)
            copyTextures(_currentLodData->_additionalTextures[2], exportFolder);
    }
    else
    {
        if (_lod0Data._node)
        {
            copyTextures(_lod0Data._node->getMesh(), exportFolder);
            if(Settings::_nm)
                copyTextures(_lod0Data._additionalTextures[1], exportFolder);
            if(Settings::_sm)
                copyTextures(_lod0Data._additionalTextures[2], exportFolder);
        }
        if (_lod1Data._node)
        {
            copyTextures(_lod1Data._node->getMesh(), exportFolder);
            if(Settings::_nm)
                copyTextures(_lod1Data._additionalTextures[1], exportFolder);
            if(Settings::_sm)
                copyTextures(_lod1Data._additionalTextures[2], exportFolder);
        }
        if (_lod2Data._node)
        {
            copyTextures(_lod2Data._node->getMesh(), exportFolder);
            if(Settings::_nm)
                copyTextures(_lod2Data._additionalTextures[1], exportFolder);
            if(Settings::_sm)
                copyTextures(_lod2Data._additionalTextures[2], exportFolder);
        }
    }

    //std::cout << filename.toStdString().c_str() << std::endl;

    IMesh* mesh = 0;
    if (_currentLodData->_node)
        mesh = _currentLodData->_node->getMesh();

    core::vector3df orDim = GUI_Resize::_originalDimensions;
    core::vector3df dim = GUI_Resize::_dimensions;

    if (_lod0Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod0Data._node->getMesh(), dim/orDim);
        scaleSkeleton(_lod0Data._node->getMesh(), (dim/orDim).X);
    }
    if (_lod1Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod1Data._node->getMesh(), dim/orDim);
        scaleSkeleton(_lod1Data._node->getMesh(), (dim/orDim).X);
    }
    if (_lod2Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod2Data._node->getMesh(), dim/orDim);
        scaleSkeleton(_lod2Data._node->getMesh(), (dim/orDim).X);
    }
    if (_collisionsLodData._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_collisionsLodData._node->getMesh(), dim/orDim);
    }

    if ((extension == ".obj" || extension == ".stl" || extension == ".ply" || extension == ".dae" || extension == ".irrmesh" || extension == ".b3d") && mesh)
    {
        irr::scene::EMESH_WRITER_TYPE type = EMWT_OBJ;
        if (extension == ".irrmesh")
            type = irr::scene::EMWT_IRR_MESH;
        else if (extension == ".dae")
            type = irr::scene::EMWT_COLLADA;
        else if (extension == ".stl")
            type = irr::scene::EMWT_STL;
        else if (extension == ".obj")
            type = irr::scene::EMWT_OBJ;
        else if (extension == ".ply")
            type = irr::scene::EMWT_PLY;
        else if (extension == ".b3d")
            type = irr::scene::EMWT_B3D;

        IMeshWriter* mw = 0;
        mw = _device->getSceneManager()->createMeshWriter(type);

        if (mw)
        {
            mw->writeMesh(file, mesh);
            mw->drop();
        }
    }
    else if (extension == ".re")
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
        IrrAssimp assimp(_device->getSceneManager());
        core::stringc extensionFlag;
        if (extension == ".x")
            extensionFlag = "x";
        else if (extension == ".3ds")
            extensionFlag = "3ds";
        else if (extension == ".assbin")
            extensionFlag = "assbin";
        else if (extension == ".assxml")
            extensionFlag = "assxml";
        else if (extension == ".stp")
            extensionFlag = "stp";
        else if (extension == ".gltf")
            extensionFlag = "gltf";
        else if (extension == ".glb")
            extensionFlag = "glb";

        //std::cout << "nb output=" << assimp.getExportFormats().size() << std::endl;

        file->drop();
        file = 0;

        assimp.exportMesh(mesh, extensionFlag, exportPath);
    }



    float scaleFactor = 1.0f / (dim.X/orDim.X);
    if (_lod0Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod0Data._node->getMesh(), irr::core::vector3df(scaleFactor, scaleFactor, scaleFactor));
        scaleSkeleton(_lod0Data._node->getMesh(), scaleFactor);
    }

    if (_lod1Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod1Data._node->getMesh(), irr::core::vector3df(scaleFactor, scaleFactor, scaleFactor));
        scaleSkeleton(_lod1Data._node->getMesh(), scaleFactor);
    }

    if (_lod2Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod2Data._node->getMesh(), irr::core::vector3df(scaleFactor, scaleFactor, scaleFactor));
        scaleSkeleton(_lod2Data._node->getMesh(), scaleFactor);
    }
    if (_collisionsLodData._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_collisionsLodData._node->getMesh(), irr::core::vector3df(scaleFactor, scaleFactor, scaleFactor));
        scaleSkeleton(_collisionsLodData._node->getMesh(), scaleFactor);
    }

    if (file)
        file->drop();

    feedbackMessage = "done";
}


void QIrrlichtWidget::changeWireframe(bool state)
{
    if (_currentLodData->_node)
        _currentLodData->_node->setMaterialFlag(video::EMF_WIREFRAME, state);
}

void QIrrlichtWidget::changeRigging(bool state)
{
    if (!_currentLodData->_node)
        return;

    if (state)
        _currentLodData->_node->setDebugDataVisible(scene::EDS_SKELETON);
    else
        _currentLodData->_node->setDebugDataVisible(scene::EDS_OFF);

}

unsigned int QIrrlichtWidget::getPolysCount()
{
    if (_currentLodData->_node)
        return _device->getSceneManager()->getMeshManipulator()->getPolyCount(static_cast<IMesh*>(_currentLodData->_node->getMesh()));
    return 0;
}

unsigned int QIrrlichtWidget::getJointsCount()
{
    if (_currentLodData->_node)
        return _currentLodData->_node->getJointCount();
    return 0;
}

irr::core::vector3df QIrrlichtWidget::getMeshDimensions()
{
    if (_currentLodData->_node)
        return _currentLodData->_node->getMesh()->getBoundingBox().MaxEdge - _currentLodData->_node->getMesh()->getBoundingBox().MinEdge;
    return core::vector3df(0, 0, 0);
}

void QIrrlichtWidget::changeOptions()
{
    core::list< ISceneNodeAnimator * > anims = _camera->getAnimators();
    core::list< ISceneNodeAnimator * >::Iterator it;
    for (it = anims.begin(); it != anims.end(); it++)
    {
        if ((*it)->getType() == ESNAT_CAMERA_MAYA)
        {
            break;
        }
    }
    irr::scene::ISceneNodeAnimatorCameraMaya* anim = (irr::scene::ISceneNodeAnimatorCameraMaya*)(*it);
    anim->setMoveSpeed(Settings::_camSpeed);
    anim->setRotateSpeed(Settings::_camRotSpeed);
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

    GUI_Resize::_originalDimensions = (_currentLodData->_node->getMesh()->getBoundingBox().MaxEdge - _currentLodData->_node->getMesh()->getBoundingBox().MinEdge);
    //if (ReSize::_unit == Unit_m)
    //    ReSize::_originalDimensions /= 100.0f;

    GUI_Resize::_dimensions = GUI_Resize::_originalDimensions;
}

void QIrrlichtWidget::clearLOD()
{
    // Delete the current w2ent/w2mesh
    _currentLodData->clearLodData();

    _device->getSceneManager()->getMeshCache()->clearUnusedMeshes();//->clear();

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

void QIrrlichtWidget::copyTextures(scene::IMesh* mesh, QString exportFolder)
{
    for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        IMeshBuffer* buf = mesh->getMeshBuffer(i);

        if (buf->getMaterial().getTexture(0))
        {
            QString filename = PATH_TO_QSTRING(buf->getMaterial().getTexture(0)->getName().getPath());

            int index = filename.lastIndexOf(".");
            QString basePath = filename.left(index);
            //std::cout << "BasePath="<< basePath.toStdString().c_str() << std::endl;

            int indice = basePath.size() - basePath.lastIndexOf('/');
            basePath = basePath.right(indice-1);


            // The extension of the texture
            QString targetExtension;
            if (Settings::_convertTextures)
                targetExtension = Settings::_texFormat;
            else
            {
                io::path extension;
                core::getFileNameExtension(extension, buf->getMaterial().getTexture(0)->getName());
                targetExtension = PATH_TO_QSTRING(extension);
            }


            QString fullPath = exportFolder + basePath + targetExtension;

            //std::cout << "-> la : " << texturePath.toStdString().c_str() << std::endl;
            QString texPath;
            if (Settings::_copyTextures)
                texPath = convertTexture(PATH_TO_QSTRING(buf->getMaterial().getTexture(0)->getName().getPath()), fullPath);
            else
            {
                QString tmpPath = PATH_TO_QSTRING(buf->getMaterial().getTexture(0)->getName().getPath());
                index = tmpPath.lastIndexOf(".");
                basePath = filename.left(index);
                texPath = convertTexture(PATH_TO_QSTRING(buf->getMaterial().getTexture(0)->getName().getPath()), basePath + targetExtension);
            }

            video::ITexture* tex = _device->getSceneManager()->getVideoDriver()->getTexture(QSTRING_TO_PATH(texPath));
            buf->getMaterial().setTexture(0, tex);
        }
    }
}


void QIrrlichtWidget::copyTextures(std::set<path> paths, QString exportFolder)
{
    std::set<io::path>::iterator it;
    for (it = paths.begin(); it != paths.end(); ++it)
    {
        QString filename = PATH_TO_QSTRING(*it);

        int index = filename.lastIndexOf(".");
        QString basePath = filename.left(index);
        //std::cout << "BasePath="<< basePath.toStdString().c_str() << std::endl;


        int indice = basePath.size() - basePath.lastIndexOf('/');
        basePath = basePath.right(indice-1);

        QString targetExtension = ".dds";
        if (Settings::_convertTextures)
            targetExtension = Settings::_texFormat;


        QString fullPath = exportFolder + basePath + targetExtension;

        //std::cout << "-> la : " << texturePath.toStdString().c_str() << std::endl;
        QString texPath;
        if (Settings::_copyTextures)
            texPath = convertTexture(PATH_TO_QSTRING(*it), fullPath);
        else
        {
            QString tmpPath = PATH_TO_QSTRING(*it);
            index = tmpPath.lastIndexOf(".");
            basePath = filename.left(index);
            texPath = convertTexture(PATH_TO_QSTRING(*it), basePath + targetExtension);
        }


        //convertTexture((*it).c_str(), texPath);
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
            return (_lod0Data._node == 0);
        break;
        case LOD_1:
            return (_lod1Data._node == 0);
        break;
        case LOD_2:
            return (_lod2Data._node == 0);
        break;
        case Collision:
            return (_collisionsLodData._node == 0);
        break;
    }
    return false;
}





IFileSystem *QIrrlichtWidget::getFileSystem()
{
    return _device->getFileSystem();
}
