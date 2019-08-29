#ifndef QIRRLICHTWIDGET_HPP
#define QIRRLICHTWIDGET_HPP

#include <irrlicht.h>

#include <QWidget>
#include <QResizeEvent>
#include <QDebug>
#include <set>

#include "IO_MeshWriter_RE.h"
#include "settings.h"
#include "Log.h"

#ifdef COMPILE_WITH_ASSIMP
    #include "IrrAssimp/IrrAssimp.h"
#endif


class NormalsDebuggerShaderCallBack : public video::IShaderConstantSetCallBack
{
public:
    NormalsDebuggerShaderCallBack() : WorldViewProjID(-1), TransWorldID(-1), InvWorldID(-1), FirstUpdate(true)
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
        _additionalTextures.resize(_IRR_MATERIAL_MAX_TEXTURES_);
    }

    scene::IAnimatedMeshSceneNode* _node;
    QVector<std::set<io::path> > _additionalTextures;
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

        bool setMesh(QString filename, core::stringc &feedbackMessage);
        bool addMesh(QString filename, core::stringc &feedbackMessage);

        void exportMesh(QString exportFolder, QString filename, ExporterInfos exporter, core::stringc &feedbackMessage);

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

        QString convertTexture(QString filename, QString destDir);

        bool loadRig(const io::path filename, core::stringc &feedbackMessage);
        bool loadAnims(const io::path filename, core::stringc &feedbackMessage);
        bool fileIsOpenableByIrrlicht(QString filename);

        bool loadTW1Anims(const io::path filename, core::stringc &feedbackMessage);

        bool loadTheCouncilTemplate(const io::path filename, core::stringc &feedbackMessage);

    signals:
        void onInit (QIrrlichtWidget* irrWidget);
        void updateIrrlichtQuery (QIrrlichtWidget* irrWidget);

    public slots:
        void updateIrrlicht(QIrrlichtWidget* irrWidget);

        void enableWireframe(bool enabled);
        void enableRigging(bool enabled);
        void enableNormals(bool enabled);

    protected:
        virtual void paintEvent(QPaintEvent* event);
        virtual void timerEvent(QTimerEvent* event);
        virtual void resizeEvent(QResizeEvent* event);
        virtual void keyPressEvent(QKeyEvent* event);
        virtual void keyReleaseEvent(QKeyEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void mouseReleaseEvent(QMouseEvent* event);
        virtual void mouseMoveEvent(QMouseEvent* event);

    private:
        IrrlichtDevice* _device;
        scene::ICameraSceneNode* _camera;

        LOD _currentLOD;

        scene::IO_MeshWriter_RE* _reWriter;

        void copyTextures(scene::IMesh* mesh, QString exportFolder);
        void copyTextures(std::set<io::path> paths, QString exportFolder);

        LOD_data _lod0Data;
        LOD_data _lod1Data;
        LOD_data _lod2Data;
        LOD_data _collisionsLodData;

        LOD_data* _currentLodData;

        bool _inLoading;

        bool isLoadableByIrrlicht(io::path filename);
        void loadMeshPostProcess();
        scene::IAnimatedMesh* loadMesh(QString filename, core::stringc &feedbackMessage);

        void initNormalsMaterial();
        NormalsDebuggerShaderCallBack* _normalsMaterial;
        s32 _normalsMaterialType;
        bool _normalsRendererEnabled;
};

#endif // QIRRLICHTWIDGET_HPP
