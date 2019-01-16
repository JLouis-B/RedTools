#ifndef EXTRACTOR_TW3_BUNDLE_H
#define EXTRACTOR_TW3_BUNDLE_H

#include <QFile>

class Extractor_TW3_BUNDLE : public QObject
{
    Q_OBJECT
public:
    explicit Extractor_TW3_BUNDLE(QString file = "", QString folder = "");

private:
    void extractBUNDLE(QString exportFolder, QString filename);

    QString _file;
    QString _folder;
    bool _stopped;

    void extractDecompressedFile(QFile &file, QString exportFolder);
    bool decompressFileRAW(char *fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename);
    bool decompressFileZLIB(char *fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename);
    bool decompressFileSNAPPY(char *fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename);
    bool decompressFileDOBOZ(char *fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename);
    bool decompressFileLZ4(char *fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename);

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

#endif // EXTRACTOR_TW3_BUNDLE_H
