#ifndef QIRRLICHTWIDGET_HPP
#define QIRRLICHTWIDGET_HPP

#include <QWidget>
#include <QResizeEvent>
#include <irrlicht.h>
#include <QDebug>
#include <set>

#include "Translator.h"
#include "GUI_Resize.h"
#include "GUI_Options.h"
#include "Utils_Qt_Irr.h"
#include "MeshCombiner.h"

#include "Log.h"


// QIrrlichtWidget from : http://labo-gamedev.com/news/9/


#include "IO_MeshLoader_W2ENT.h"
#include "IO_MeshLoader_W3ENT.h"
#include "IO_MeshLoader_RE.h"
#include "IO_MeshWriter_RE.h"
#include "IO_MeshLoader_WitcherMDL.h"

#ifdef COMPILE_WITH_ASSIMP
    #include "IrrAssimp/IrrAssimp.h"
#endif

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
        explicit QIrrlichtWidget (QWidget *parent = 0);
        ~QIrrlichtWidget ();

        bool setModel(QString filename, core::stringc &feedbackMessage);
        bool addMesh(QString filename, core::stringc &feedbackMessage);

        void init ();
        void writeFile (QString exportFolder, QString filename, ExporterInfos exporter, core::stringc &feedbackMessage);
        void changeWireframe(bool state);
        void changeRigging(bool state);

        unsigned int getPolysCount();
        unsigned int getJointsCount();
        core::vector3df getMeshDimensions();
        void changeOptions();
        void changeLOD(LOD newLOD);
        void clearLOD();
        void clearAllLODs();

        io::IFileSystem* getFileSystem();


        QString getFilename();
        QString getPath();

        bool isEmpty (LOD lod);

        QString convertTexture(QString filename, QString destDir);

        bool loadRig(const io::path filename, core::stringc &feedbackMessage);
        bool loadAnims(const io::path filename, core::stringc &feedbackMessage);
        bool fileIsOpenableByIrrlicht(QString filename);


    signals:
        void onInit (QIrrlichtWidget *irrWidget);
        void updateIrrlichtQuery (QIrrlichtWidget *irrWidget);

    public slots:
        void updateIrrlicht (QIrrlichtWidget *irrWidget);

    protected:
        virtual void paintEvent (QPaintEvent *ev);
        virtual void timerEvent (QTimerEvent *ev);
        virtual void resizeEvent (QResizeEvent *ev);
        void keyPressEvent(QKeyEvent * event);
        void keyReleaseEvent(QKeyEvent * event);
        void mousePressEvent( QMouseEvent* event );
        void mouseReleaseEvent( QMouseEvent* event );
        void mouseMoveEvent(QMouseEvent * event);

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

};

#endif // QIRRLICHTWIDGET_HPP
