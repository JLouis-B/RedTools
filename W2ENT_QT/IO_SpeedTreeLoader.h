#ifndef IO_SPEEDTREELOADER_H
#define IO_SPEEDTREELOADER_H

#include <QObject>

#include <IMeshManipulator.h>
#include <ISkinnedMesh.h>
#include <IFileSystem.h>

struct SpeedTreeBezierPoint
{
    irr::u32 pointId;
    irr::core::vector3df point;
};

typedef irr::core::array<SpeedTreeBezierPoint> SpeedTreeBezierCurve;

struct SpeedTree_Tree
{
    SpeedTreeBezierPoint Disturbance;
    SpeedTreeBezierPoint Gravity;
    SpeedTreeBezierPoint Flexibility;
    SpeedTreeBezierPoint FlexibilityProfile;
    SpeedTreeBezierPoint Lenght;
    SpeedTreeBezierPoint Radius;
    SpeedTreeBezierPoint RadiusProfile;
    SpeedTreeBezierPoint StartAngle;
};

class IO_SpeedTreeLoader
{
public:
    IO_SpeedTreeLoader(irr::io::IFileSystem* fileSystem);
    void readSpeedTreeFile (irr::core::stringc filename);

private:
    irr::io::IFileSystem* FileSystem;
    SpeedTreeBezierCurve readSpeedtreeSpline(irr::io::IReadFile* file);
};

#endif // IO_SPEEDTREELOADER_H
