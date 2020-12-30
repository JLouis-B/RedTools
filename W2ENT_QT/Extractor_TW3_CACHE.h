#ifndef EXTRACTOR_TW3_CACHE_H
#define EXTRACTOR_TW3_CACHE_H

#include <QFile>

class Extractor_TW3_CACHE : public QObject
{
    Q_OBJECT
public:
    explicit Extractor_TW3_CACHE(QString file = "", QString folder = "");

private:
    void extractCACHE(QString exportFolder, QString filename);

    QString _file;
    QString _folder;
    bool _stopped;

    void extractDecompressedFile(QFile &file, QString exportFolder);

    bool writeDecompressedFile(char* decompressedFileContent, qint64 decompressedSize, QString exportFolder, QString filename);
    int _lastProgression;


public slots :
    void run();
    void quitThread();

signals:
    void onProgress(int);
    void finished();
    void error();
};

#endif // EXTRACTOR_TW3_CACHE_H
