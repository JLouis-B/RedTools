#ifndef TW1BIFEXTRACTOR_H
#define TW1BIFEXTRACTOR_H

// Based on the specification from http://witcher.wikia.com/wiki/KEY_BIF_V1.1_format

#include <QFile>
#include <QMap>

class ResourceId
{
public:
    ResourceId(unsigned int bifId = 0, unsigned int resourceId = 0) : _bifId(bifId), _resourceId(resourceId)
    {
    }

    ~ResourceId()
    {
    }

    unsigned int _bifId;
    unsigned int _resourceId;
};

bool operator< (const ResourceId& a, const ResourceId& b);

class Extractor_TW1_BIF : public QObject
{
    Q_OBJECT

    QMap<ResourceId, QString> _resources;
    QMap<unsigned short, QString> _fileTypes;
    unsigned int _nbProgress;
    int _lastProgression;

    QString getExtensionFromResourceType(unsigned short resourceType);
    void createResourceTypeMap();

    QString _file;
    QString _folder;
    bool _stopped;


public:
    Extractor_TW1_BIF(QString file = "", QString folder = "");
    void extractBIF(QString exportFolder, QString filename, unsigned int bifId = 0);
    void extractKeyBIF(QString exportFolder, QString filename);

public slots :
    void run();
    void quitThread();

signals:
    void onProgress(int);
    void finished();
    void error();
};

#endif // TW1BIFEXTRACTOR_H
