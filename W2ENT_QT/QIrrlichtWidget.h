#ifndef QIRRLICHTWIDGET_HPP
#define QIRRLICHTWIDGET_HPP

#include <IrrlichtDevice.h>
#include <IMaterialRendererServices.h>
#include <IShaderConstantSetCallBack.h>
#include <IAnimatedMeshSceneNode.h>
#include <ICameraSceneNode.h>

#include <QWidget>
#include <QResizeEvent>
#include <QDebug>
#include <QSet>

#include "IO_MeshWriter_RE.h"
#include "Settings.h"
#include "Log/LoggerManager.h"
#include "Log/IrrFileLogger.h"

#ifdef COMPILE_WITH_ASSIMP
    #include "IrrAssimp/IrrAssimp.h"
#endif

using namespace irr;

class NormalsDebuggerShaderCallBack : public video::IShaderConstantSetCallBack
{
public:
    NormalsDebuggerShaderCallBack() :
        Device(nullptr),
        WorldViewProjID(-1),
        TransWorldID(-1),
        InvWorldID(-1),
        FirstUpdate(true)
    {
    }

    virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userData)
    {
        video::IVideoDriver* driver = services->getVideoDriver();

        // get shader constants id.
        if (FirstUpdate)
        {
            WorldViewProjID = services->getVertexShaderConstantID("mWorldViewProj");
            TransWorldID = services->getVertexShaderConstantID("mTransWorld");
            InvWorldID = services->getVertexShaderConstantID("mInvWorld");

            FirstUpdate = false;
        }

        // set inverted world matrix
        // if we are using highlevel shaders (the user can select this when
        // starting the program), we must set the constants by name.
        core::matrix4 invWorld = driver->getTransform(video::ETS_WORLD);
        invWorld.makeInverse();
        services->setVertexShaderConstant(InvWorldID, invWorld.pointer(), 16);

        // set clip matrix
        core::matrix4 worldViewProj;
        worldViewProj = driver->getTransform(video::ETS_PROJECTION);
        worldViewProj *= driver->getTransform(video::ETS_VIEW);
        worldViewProj *= driver->getTransform(video::ETS_WORLD);
        services->setVertexShaderConstant(WorldViewProjID, worldViewProj.pointer(), 16);

        // set transposed world matrix
        core::matrix4 world = driver->getTransform(video::ETS_WORLD);
        world = world.getTransposed();
        services->setVertexShaderConstant(TransWorldID, world.pointer(), 16);
    }

    void SetDevice(IrrlichtDevice* device)
    {
        Device = device;
    }

private:
    IrrlichtDevice* Device;

    s32 WorldViewProjID;
    s32 TransWorldID;
    s32 InvWorldID;

    bool FirstUpdate;
};

enum LOD
{
    LOD_0,
    LOD_1,
    LOD_2,
    Collision
};

struct LOD_data
{
    LOD_data() : _node(nullptr)
    {
        clearLodData();
    }

    void clearLodData()
    {
        if (_node)
        {
            _node->remove();
            _node = nullptr;
        }

        _additionalTextures.clear();
    }

    scene::IAnimatedMeshSceneNode* _node;
    QVector<QVector<QString> > _additionalTextures;

    QSet<QString> getTexturesSetForLayer(int layer)
    {
        Q_ASSERT(layer >= 1 && layer < _IRR_MATERIAL_MAX_TEXTURES_);
        QSet<QString> texturesSet;
        for (int i = 0; i < _additionalTextures.size(); ++i)
        {
            QString path = _additionalTextures[i][layer-1];
            if (!path.isEmpty())
                texturesSet.insert(path);
        }
        return texturesSet;
    }
};

struct IrrlichtExporterInfos
{
    IrrlichtExporterInfos(scene::EMESH_WRITER_TYPE irrExporter = scene::EMWT_OBJ, s32 irrFlags = scene::EMWF_NONE)
        : _irrExporter(irrExporter)
        , _irrFlags(irrFlags)
    {}

    scene::EMESH_WRITER_TYPE _irrExporter;
    s32 _irrFlags;
};

enum ExportType
{
    Exporter_Irrlicht,
    Exporter_Redkit,
    Exporter_Assimp
};

struct ExporterInfos
{
    ExportType _exporterType;
    QString _exporterName;
    QString _extension;

    // Assimp specifics
    QString _assimpExporterId;

    // Irrlicht specifics
    IrrlichtExporterInfos _irrlichtInfos;
};

class QIrrlichtWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit QIrrlichtWidget (QWidget *parent = nullptr);
        ~QIrrlichtWidget ();

        void init();

        io::IFileSystem* getFileSystem();

        bool loadAndReplaceMesh(QString filename);
        bool addMesh(QString filename);

        void exportMesh(QString exportFolderPath, QString filename, ExporterInfos exporter);

        unsigned int getPolysCount();
        unsigned int getJointsCount();
        core::vector3df getMeshDimensions();
        void changeOptions();
        void changeLOD(LOD newLOD);
        void clearLOD();
        void clearAllLODs();

        QString getFilename();
        QString getPath();

        bool isEmpty (LOD lod);

        bool loadRig(const io::path filename);
        bool loadAnims(const io::path filename);
        bool fileIsOpenableByIrrlicht(QString filename);

        bool loadTW1Anims(const io::path filename);

        bool loadTheCouncilTemplate(const io::path filename);

    signals:
        void onInit (QIrrlichtWidget* irrWidget);
        void updateIrrlichtQuery (QIrrlichtWidget* irrWidget);

    public slots:
        void updateIrrlicht(QIrrlichtWidget* irrWidget);

        void enableWireframe(bool enabled);
        void enableRigging(bool enabled);
        void enableNormals(bool enabled);

    protected:
        virtual void paintEvent(QPaintEvent* event) override;
        virtual void timerEvent(QTimerEvent* event) override;
        virtual void resizeEvent(QResizeEvent* event) override;
        virtual void keyPressEvent(QKeyEvent* event) override;
        virtual void keyReleaseEvent(QKeyEvent* event) override;
        virtual void mousePressEvent(QMouseEvent* event) override;
        virtual void mouseReleaseEvent(QMouseEvent* event) override;
        virtual void mouseMoveEvent(QMouseEvent* event) override;

    private:
        IrrlichtDevice* _device;
        scene::ICameraSceneNode* _camera;

        LOD _currentLOD;

        scene::IO_MeshWriter_RE* _reWriter;

        bool convertAndCopyTexture(QString texturePath, QString exportFolder, bool shouldCopyTextures, QString& outputTexturePath);
        void convertAndCopyTextures(scene::IMesh* mesh, QString exportFolder, bool shouldCopyTextures);
        void convertAndCopyTextures(QSet<QString> paths, QString exportFolder, bool shouldCopyTextures);

        LOD_data _lod0Data;
        LOD_data _lod1Data;
        LOD_data _lod2Data;
        LOD_data _collisionsLodData;

        LOD_data* _currentLodData;

        bool _inLoading;

        bool isLoadableByIrrlicht(io::path filename);
        void loadMeshPostProcess();
        scene::IAnimatedMesh* loadMesh(QString filename);

        void initNormalsMaterial();
        NormalsDebuggerShaderCallBack* _normalsMaterial;
        s32 _normalsMaterialType;
        bool _normalsRendererEnabled;

        IrrFileLogger* _irrFileLogger;
        void createIrrFileLogger();
        void destroyIrrFileLogger();
};

#endif // QIRRLICHTWIDGET_HPP
