#include "qirrlichtwidget.h"
#include <iostream>

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;

void initLoadData(LOD_data* lodData)
{
    lodData->_node = 0;
    lodData->_skinned = false;
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
    node->setMaterialFlag(EMF_LIGHTING, false);
    node->setMaterialType(EMT_SOLID);

    node->setMaterialTexture(1, NULL);
    node->setMaterialTexture(2, NULL);
    node->setMaterialTexture(3, NULL);


}

bool QIrrlichtWidget::loadRig(core::stringc filename, core::stringc &feedbackMessage)
{
    io::IReadFile* file = _device->getFileSystem()->createAndOpenFile(filename);
    if (!file)
    {
        feedbackMessage = "Error : The file can't be opened.";
        return false;
    }

    scene::CW3ENTMeshFileLoader loader(_device->getSceneManager(), _device->getFileSystem());
    scene::IAnimatedMesh* mesh = loader.createMesh(file);

    if (mesh)
        mesh->drop();

    CSkeleton skeleton = loader.Skeleton;

    scene::ISkinnedMesh* newMesh = skeleton.copySkinnedMesh(_device->getSceneManager(), (scene::ISkinnedMesh*)_currentLodData->_node->getMesh());

    bool sucess = skeleton.applyToModel(newMesh);
    if (sucess)
        feedbackMessage = "Rig sucessfully applied";
    else
        feedbackMessage = "The skeleton can't be applied to the model. Are you sure that you have selected the good w2rig file ?";

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

    initLoadData(&_lod0Data);
    initLoadData(&_lod1Data);
    initLoadData(&_lod2Data);
    initLoadData(&_collisionsLodData);

    _currentLodData = &_lod0Data;

    _reWriter = 0;
    _camera = 0;

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
    io::path irrFilename = QSTRING_TO_PATH(filename.toStdString().c_str());

    io::IReadFile* file = _device->getFileSystem()->createAndOpenFile(irrFilename);
    if (!file)
        return false;

    file->drop();
    return true;
}

bool QIrrlichtWidget::setModel(QString filename, stringc &feedbackMessage)
{
    _inLoading = true;

    if (Settings::_pack0[Settings::_pack0.size() - 1] != '/')
        Settings::_pack0.push_back('/');

    if (Settings::_TW3TexPath[Settings::_TW3TexPath.size() - 1] != '/')
        Settings::_TW3TexPath.push_back('/');

    _device->getSceneManager()->getParameters()->setAttribute("TW_DEBUG_LOG", Settings::_debugLog);
    _device->getSceneManager()->getParameters()->setAttribute("TW_GAME_PATH", Settings::_pack0.toStdString().c_str());
    _device->getSceneManager()->getParameters()->setAttribute("TW_TW3_TEX_PATH", Settings::_TW3TexPath.toStdString().c_str());
    _device->getSceneManager()->getParameters()->setAttribute("TW_TW3_LOAD_SKEL", Settings::_TW3LoadSkel);



    const io::path irrFilename = QSTRING_TO_PATH(filename);
    io::path extension;
    core::getFileNameExtension(extension, irrFilename);

    // Delete the current mesh
    clearLOD();

    /*
    Log l(_device->getSceneManager(), "preload.log");
    if (!l.works())
        QMessageBox::critical(0, "fail to create the log file", "fail to create the log file");

    l.enable(Settings::_debugLog);
    l.addAndPush("change work dir\n");
    */

    IAnimatedMesh* mesh = 0;
    IrrAssimp assimp(_device->getSceneManager());
    //l.addAndPush("irrassimp loaded\n");
    if (isLoadableByIrrlicht(irrFilename))
    {
        //l.addAndPush("irr loader\n");
        mesh = _device->getSceneManager()->getMesh(irrFilename);
        //l.addAndPush("getMesh\n");

        // Witcher feedback
        if (extension == "w2mesh" || extension == "w2ent" || extension == "w2rig")
        {
            feedbackMessage = _device->getSceneManager()->getParameters()->getAttributeAsString("TW_FEEDBACK");
            _device->getSceneManager()->getParameters()->setAttribute("TW_FEEDBACK", "");
            //l.addAndPush("feedback is ");
            //l.addAndPush(feedbackMessage.c_str());
        }
        else if (!mesh)
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

    // Leave here if there was a problem during the loading
    if (!mesh)
    {
        _inLoading = false;
        return false;
    }

    ReSize::_originalDimensions = (mesh->getBoundingBox().MaxEdge - mesh->getBoundingBox().MinEdge);
    ReSize::_dimensions = (mesh->getBoundingBox().MaxEdge - mesh->getBoundingBox().MinEdge);

    if (ReSize::_unit == Unit_m)
    {
        ReSize::_originalDimensions /= 100.0f;
        ReSize::_dimensions /= 100.0f;
    }

    // Save the path of normals/specular maps
    for (unsigned int i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        IMeshBuffer* buf = mesh->getMeshBuffer(i);
        if(buf->getMaterial().getTexture(1))
            _currentLodData->_normalMaps.insert(buf->getMaterial().getTexture(1)->getName().getPath());
        if(buf->getMaterial().getTexture(2))
            _currentLodData->_specularMaps.insert(buf->getMaterial().getTexture(2)->getName().getPath());
    }

    _currentLodData->_node = _device->getSceneManager()->addAnimatedMeshSceneNode(mesh);
    setMaterialsSettings(_currentLodData->_node);

    _currentLodData->_node->setScale(irr::core::vector3df(20, 20, 20));
    _currentLodData->_node->setRotation(irr::core::vector3df(_currentLodData->_node->getRotation().X, _currentLodData->_node->getRotation().Y - 90, _currentLodData->_node->getRotation().Z));

    // for debug only
    // loadedNode->setDebugDataVisible(EDS_BBOX_ALL);

    _camera->setTarget(_currentLodData->_node->getPosition());

    // DEBUG
    /*
    std::cout << "Dimensions : x=" << mesh->getBoundingBox().MaxEdge.X - mesh->getBoundingBox().MinEdge.X
              << ", y=" << mesh->getBoundingBox().MaxEdge.Y - mesh->getBoundingBox().MinEdge.Y
              << ", z=" << mesh->getBoundingBox().MaxEdge.Z - mesh->getBoundingBox().MinEdge.Z << std::endl;
    */

    // Is it a skinned mesh ?
    if(mesh->getMeshType() == scene::EAMT_SKINNED) //if (dynamic_cast<ISkinnedMesh*>(mesh))
    {
        _currentLodData->_skinned = true;
    }
    else
        _currentLodData->_skinned = false;

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

        _reWriter = new CREMeshWriter(_device->getSceneManager(), _device->getFileSystem());

        _device->getSceneManager()->addExternalMeshLoader(new CREMeshFileLoader(_device->getSceneManager(), _device->getFileSystem()));
        _device->getSceneManager()->addExternalMeshLoader(new CW2ENTMeshFileLoader(_device->getSceneManager(), _device->getFileSystem()));
        _device->getSceneManager()->addExternalMeshLoader(new CW3ENTMeshFileLoader(_device->getSceneManager(), _device->getFileSystem()));

        //_device->getSceneManager()->setAmbientLight(irr::video::SColor(255,255,255,255));
    }

    /* puis on connecte notre slot updateIrrlicht (), qui s'occupe du rendu
     * à notre signal updateIrrlichtQuery ()
     */
    connect (this, SIGNAL (updateIrrlichtQuery (QIrrlichtWidget*)), this, SLOT (updateIrrlicht (QIrrlichtWidget*)));

    // et on lance notre timer
    startTimer (0);
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
     }

    QWidget::resizeEvent (ev);
}

void QIrrlichtWidget::mouseMoveEvent(QMouseEvent * event)
{
    irr::SEvent irrEvent;
    irrEvent.EventType = irr::EET_MOUSE_INPUT_EVENT;

    irrEvent.MouseInput.Event = irr::EMIE_MOUSE_MOVED;
    irrEvent.MouseInput.X = _device->getCursorControl()->getPosition().X;
    irrEvent.MouseInput.Y = _device->getCursorControl()->getPosition().Y;

    irrEvent.MouseInput.Wheel = 0.0f; // Zero is better than undefined


    u32 buttonState = 0;
    if (QApplication::mouseButtons ()  & Qt::LeftButton) {
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
    // If there is a mouse event, we should report it to Irrlicht for the Maya camera
    irr::SEvent irrEvent;

    irrEvent.EventType = irr::EET_MOUSE_INPUT_EVENT;

    if ( _device != 0 )
    {
        if (event->button() == Qt::LeftButton)
            irrEvent.MouseInput.Event = irr::EMIE_RMOUSE_PRESSED_DOWN;
        else if (event->button() == Qt::RightButton)
            irrEvent.MouseInput.Event = irr::EMIE_LMOUSE_PRESSED_DOWN;
        else if (event->button() == Qt::MiddleButton)
            irrEvent.MouseInput.Event = irr::EMIE_MMOUSE_PRESSED_DOWN;

        irrEvent.MouseInput.X = _device->getCursorControl()->getPosition().X;
        irrEvent.MouseInput.Y = _device->getCursorControl()->getPosition().Y;
        irrEvent.MouseInput.Wheel = 0.0f; // Zero is better than undefined

        if(_device->postEventFromUser( irrEvent ))
            event->accept();
    }
}

void QIrrlichtWidget::mouseReleaseEvent( QMouseEvent* event )
{
    // If there is a mouse event, we should report it to Irrlicht for the Maya camera
    irr::SEvent irrEvent;

    irrEvent.EventType = irr::EET_MOUSE_INPUT_EVENT;

    if (_device)
    {
        if (event->button() == Qt::LeftButton)
            irrEvent.MouseInput.Event = irr::EMIE_RMOUSE_LEFT_UP;
        else if (event->button() == Qt::RightButton)
        {
            irrEvent.MouseInput.Event = irr::EMIE_LMOUSE_LEFT_UP;
        }
        else if (event->button() == Qt::MiddleButton)
        {
            irrEvent.MouseInput.Event = irr::EMIE_MMOUSE_LEFT_UP;
        }
        irrEvent.MouseInput.X = _device->getCursorControl()->getPosition().X;
        irrEvent.MouseInput.Y = _device->getCursorControl()->getPosition().Y;
        irrEvent.MouseInput.Wheel = 0.0f; // Zero is better than undefined

        if(_device->postEventFromUser( irrEvent ))
            event->accept();
    }
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

void scaleSkeleton(scene::ISkinnedMesh* mesh, double factor)
{
    const u32 nbJoints = mesh->getJointCount();
    for (u32 i = 0; i < nbJoints; ++i)
    {
        ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i];
        //joint->Animatedposition *= factor;
    }
}

void QIrrlichtWidget::writeFile (QString exportFolder, QString filename, QString extension)
{
    if (!_currentLodData->_node && extension != ".re")
        return;

    if (Settings::_moveTexture)
    {
        // Will be exported in a subfolder
        exportFolder = exportFolder + "/" +  filename + "_export/";
        QDir dir;
        dir.mkdir(exportFolder);
    }

    if (extension != ".re")
    {
        copyTextures(_currentLodData->_node->getMesh(), exportFolder);
        if(Settings::_nm)
            copyTextures(_currentLodData->_normalMaps, exportFolder);
        if(Settings::_sm)
            copyTextures(_currentLodData->_specularMaps, exportFolder);
    }
    else
    {
        if (_lod0Data._node)
        {
            copyTextures(_lod0Data._node->getMesh(), exportFolder);
            if(Settings::_nm)
                copyTextures(_lod0Data._normalMaps, exportFolder);
            if(Settings::_sm)
                copyTextures(_lod0Data._specularMaps, exportFolder);
        }
        if (_lod1Data._node)
        {
            copyTextures(_lod1Data._node->getMesh(), exportFolder);
            if(Settings::_nm)
                copyTextures(_lod1Data._normalMaps, exportFolder);
            if(Settings::_sm)
                copyTextures(_lod1Data._specularMaps, exportFolder);
        }
        if (_lod2Data._node)
        {
            copyTextures(_lod2Data._node->getMesh(), exportFolder);
            if(Settings::_nm)
                copyTextures(_lod2Data._normalMaps, exportFolder);
            if(Settings::_sm)
                copyTextures(_lod2Data._specularMaps, exportFolder);
        }
    }

    //std::cout << filename.toStdString().c_str() << std::endl;
    IWriteFile* file = _device->getFileSystem()->createAndWriteFile(QSTRING_TO_PATH(exportFolder + "/" + filename + extension));

    IMesh* mesh = 0;
    if (_currentLodData->_node)
        mesh = _currentLodData->_node->getMesh();

    irr::core::vector3df orDim = ReSize::_originalDimensions;
    irr::core::vector3df dim = ReSize::_dimensions;

    if (ReSize::_unit == Unit_cm)
        dim = dim / 100.0f;

    if (_lod0Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod0Data._node->getMesh(), dim/orDim);
        if(_lod0Data._skinned)
            scaleSkeleton((irr::scene::ISkinnedMesh*)_lod0Data._node->getMesh(), (dim/orDim).X);
    }
    if (_lod1Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod1Data._node->getMesh(), dim/orDim);
        if(_lod1Data._skinned)
            scaleSkeleton((irr::scene::ISkinnedMesh*)_lod1Data._node->getMesh(), (dim/orDim).X);
    }
    if (_lod2Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod2Data._node->getMesh(), dim/orDim);
        if(_lod2Data._skinned)
            scaleSkeleton((irr::scene::ISkinnedMesh*)_lod2Data._node->getMesh(), (dim/orDim).X);
    }
    if (_collisionsLodData._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_collisionsLodData._node->getMesh(), dim/orDim);
    }


    if ((extension == ".obj" || extension == ".stl" || extension == ".ply" || extension == ".dae" || extension == ".irrmesh" || extension == ".b3d") && mesh)
    {
        irr::scene::EMESH_WRITER_TYPE type;
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
            _reWriter->setLOD1((irr::scene::ISkinnedMesh*)_lod1Data._node->getMesh());
        if (_lod2Data._node)
            _reWriter->setLOD2((irr::scene::ISkinnedMesh*)_lod2Data._node->getMesh());
        if (_collisionsLodData._node)
        {
            _reWriter->setCollisionMesh(_collisionsLodData._node->getMesh());
        }
        _reWriter->writeAnimatedMesh(file, _lod0Data._node->getMesh(), _lod0Data._skinned);
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

        std::cout << "nb output=" << assimp.getExportFormats().size() << std::endl;

        io::path path = file->getFileName();

        file->drop();
        file = 0;

        assimp.exportMesh(mesh, extensionFlag, path);
    }



    float scaleFactor = 1.0f / (dim.X/orDim.X);
    if (_lod0Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod0Data._node->getMesh(), irr::core::vector3df(scaleFactor, scaleFactor, scaleFactor));
        if(_lod0Data._skinned)
            scaleSkeleton((irr::scene::ISkinnedMesh*)_lod0Data._node->getMesh(), scaleFactor);
    }

    if (_lod1Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod1Data._node->getMesh(), irr::core::vector3df(scaleFactor, scaleFactor, scaleFactor));
        if(_lod1Data._skinned)
            scaleSkeleton((irr::scene::ISkinnedMesh*)_lod1Data._node->getMesh(), scaleFactor);
    }

    if (_lod2Data._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_lod2Data._node->getMesh(), irr::core::vector3df(scaleFactor, scaleFactor, scaleFactor));
        if(_lod2Data._skinned)
            scaleSkeleton((irr::scene::ISkinnedMesh*)_lod2Data._node->getMesh(), scaleFactor);
    }
    if (_collisionsLodData._node)
    {
        _device->getSceneManager()->getMeshManipulator()->scale(_collisionsLodData._node->getMesh(), irr::core::vector3df(scaleFactor, scaleFactor, scaleFactor));
        if(_collisionsLodData._skinned)
            scaleSkeleton((irr::scene::ISkinnedMesh*)_collisionsLodData._node->getMesh(), scaleFactor);
    }

    if (file)
        file->drop();
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

    ReSize::_originalDimensions = (_currentLodData->_node->getMesh()->getBoundingBox().MaxEdge - _currentLodData->_node->getMesh()->getBoundingBox().MinEdge);
    ReSize::_dimensions = (_currentLodData->_node->getMesh()->getBoundingBox().MaxEdge - _currentLodData->_node->getMesh()->getBoundingBox().MinEdge);

    if (ReSize::_unit == Unit_m)
    {
        ReSize::_originalDimensions /= 100.0f;
        ReSize::_dimensions /= 100.0f;
    }
}

void QIrrlichtWidget::clearLOD()
{
    // Delete the current w2ent/w2mesh
    if(_currentLodData->_node)
    {
        _currentLodData->_node->remove();
        _currentLodData->_node = 0;

        _device->getSceneManager()->getMeshCache()->clearUnusedMeshes();//->clear();

        _currentLodData->_normalMaps.clear();
        _currentLodData->_specularMaps.clear();
        _currentLodData->_skinned = false;
    }

    if (!_lod0Data._node && !_lod1Data._node && !_lod2Data._node && !_collisionsLodData._node)
    {
        _device->getVideoDriver()->removeAllTextures();
        _device->getVideoDriver()->removeAllHardwareBuffers();
    }
}


void clearLodData(LOD_data* data)
{
    if (!data->_node)
        return;

    data->_node->remove();
    data->_node = 0;
    data->_normalMaps.clear();
    data->_specularMaps.clear();
    data->_skinned = false;
}

void QIrrlichtWidget::clearAllLODs()
{
    clearLodData(&_lod0Data);
    clearLodData(&_lod1Data);
    clearLodData(&_lod2Data);
    clearLodData(&_collisionsLodData);

    _device->getSceneManager()->getMeshCache()->clearUnusedMeshes();//->clear();

    _device->getVideoDriver()->removeAllTextures();
    _device->getVideoDriver()->removeAllHardwareBuffers();
}

void QIrrlichtWidget::copyTextures(irr::scene::IMesh* mesh, QString exportFolder)
{
    for (unsigned int i = 0; i < mesh->getMeshBufferCount(); ++i)
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
            if (Settings::_moveTexture)
                texPath = convertTexture(PATH_TO_QSTRING(buf->getMaterial().getTexture(0)->getName().getPath()), fullPath);
            else
            {
                QString tmpPath = PATH_TO_QSTRING(buf->getMaterial().getTexture(0)->getName().getPath());
                index = tmpPath.lastIndexOf(".");
                basePath = filename.left(index);
                texPath = convertTexture(PATH_TO_QSTRING(buf->getMaterial().getTexture(0)->getName().getPath()), basePath + targetExtension);
            }

            video::ITexture* tex = _device->getSceneManager()->getVideoDriver()->getTexture(texPath.toStdString().c_str());
            buf->getMaterial().setTexture(0, tex);
        }
    }
}


void QIrrlichtWidget::copyTextures(std::set<irr::io::path> paths, QString exportFolder)
{
    std::set<irr::io::path>::iterator it;
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
        if (Settings::_moveTexture)
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

bool QIrrlichtWidget::isEmpty (LOD lod)
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
}





IFileSystem *QIrrlichtWidget::getFileSystem()
{
    return _device->getFileSystem();
}
