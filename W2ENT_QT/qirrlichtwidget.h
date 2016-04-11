#ifndef QIRRLICHTWIDGET_HPP
#define QIRRLICHTWIDGET_HPP

#include <QWidget>
#include <QResizeEvent>
#include <irrlicht.h>
#include <QDebug>
#include <set>

#include "translator.h"
#include "resize.h"
#include "options.h"
#include "utils.h"
#include "meshcombiner.h"

#include "log.h"


// QIrrlichtWidget from : http://labo-gamedev.com/news/9/


#include "CW2ENTMeshFileLoader.h"
#include "CW3ENTMeshFileLoader.h"
#include "CREMeshFileLoader.h"
#include "CREMeshWriter.h"
#include "CWitcherMDLMeshFileLoader.h"

// Assimp is used to load many file formats. You can disable it with this define
#define COMPILE_WITH_ASSIMP
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
    LOD_data() : _node(0)
    {
        clearLodData();
    }

    void clearLodData()
    {
        if (_node)
        {
            _node->remove();
            _node = 0;
        }

        _additionalTextures.clear();
        _additionalTextures.resize(_IRR_MATERIAL_MAX_TEXTURES_);
    }

    scene::IAnimatedMeshSceneNode* _node;
    QVector<std::set<io::path> > _additionalTextures;
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
        void writeFile (QString exportFolder, QString filename, QString extension, core::stringc &feedbackMessage);
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
        void mousePressEvent( QMouseEvent* event );
        void mouseReleaseEvent( QMouseEvent* event );
        void mouseMoveEvent(QMouseEvent * event);

    private:
        IrrlichtDevice *_device;
        scene::ICameraSceneNode *_camera;

        LOD _currentLOD;

        scene::CREMeshWriter* _reWriter;

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
