#ifndef EXTRACTOR_THECOUNCIL_H
#define EXTRACTOR_THECOUNCIL_H

#include <QFile>
#include <QBuffer>
#include <QFileInfo>
#include <QDir>
#include <QMap>

struct TheCouncilFileEntry
{
    QString filename;
    quint64 offset;
    quint32 size;
};

class Extractor_TheCouncil : public QObject
{
    Q_OBJECT

    unsigned int _nbProgress;
    int _lastProgression;

    QString _file;
    QString _folder;
    bool _stopped;

    void extract(QString exportFolder, QString filename);
    void extractCPKFile(QFile &file, QString exportFolder);

public:
    Extractor_TheCouncil(QString file = "", QString folder = "");

public slots :
    void run();
    void quitThread();

signals:
    void onProgress(int);
    void finished();
    void error();
};

#endif // EXTRACTOR_THECOUNCIL_H
