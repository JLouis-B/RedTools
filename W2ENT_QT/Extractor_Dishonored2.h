#ifndef EXTRACTOR_DISHONORED2_H
#define EXTRACTOR_DISHONORED2_H

#include <QFile>
#include <QFileInfo>
#include <QDir>

class Extractor_Dishonored2 : public QObject
{
    Q_OBJECT

public:
    Extractor_Dishonored2(QString indexFile = "", QString resourcesFile = "", QString folder = "");

public slots :
    void run();
    void quitThread();

signals:
    void onProgress(int);
    void finished();
    void error();

private:
    void extract(QString exportFolder, QString indexFilename, QString resourcesFilename);
    void extractFiles(QFile& indexFile, QFile& resourcesFile, QFile &sharedResourcesFile, QString exportFolder);

    QString _indexFile;
    QString _resourcesFile;
    QString _folder;
    bool _stopped;
    unsigned int _nbProgress;
    int _lastProgression;
};

#endif // EXTRACTOR_DISHONORED2_H
