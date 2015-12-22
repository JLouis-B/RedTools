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
    irr::scene::IAnimatedMeshSceneNode* _node;//IAnimated
    std::set<irr::io::path> _normalMaps;
    std::set<irr::io::path> _specularMaps;
    bool _skinned;
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
        void writeFile (QString exportFolder, QString filename, QString extension);
        void changeWireframe(bool state);
        void changeRigging(bool state);

        unsigned int getPolysCount();
        unsigned int getJointsCount();
        irr::core::vector3df getMeshDimensions();
        void changeOptions();
        void changeLOD(LOD newLOD);
        void clearLOD();
        void clearAllLODs();

        irr::io::IFileSystem* getFileSystem();


        QString getFilename();
        QString getPath();

        bool isEmpty (LOD lod);

        QString convertTexture(QString filename, QString destDir);

        bool loadRig(core::stringc filename, core::stringc &feedbackMessage);
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
        irr::IrrlichtDevice *_device;
        irr::scene::ICameraSceneNode *_camera;

        LOD _currentLOD;

        irr::scene::CREMeshWriter* _reWriter;

        void copyTextures(irr::scene::IMesh* mesh, QString exportFolder);
        void copyTextures(std::set<irr::io::path> paths, QString exportFolder);

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
