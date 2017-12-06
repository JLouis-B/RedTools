#ifndef TW2_DZIP_EXTRACTOR_H
#define TW2_DZIP_EXTRACTOR_H

#include <QObject>
#include <QFile>
#include <QBuffer>
#include <QFileInfo>
#include <QDir>
#include <QMap>

// Based on :
// http://www.krinkels.ucoz.ru/_fr/0/scriptw2.txt

// LZF compression explained here :
// https://en.wikibooks.org/wiki/Data_Compression/Dictionary_compression#LZF
// https://github.com/ning/compress/wiki/LZFFormat
// http://forum.xentax.com/viewtopic.php?f=10&t=6634&hilit=xbm&sid=b5dfb3488bf193d58b5e5c07d1a9b6cb

// Reference code to unpack LZF :
// http://svn.gib.me/public/red/trunk/Gibbed.RED.Unpack/Lzf.cs


class Extractor_TW2_DZIP : public QObject
{
    Q_OBJECT
public:
    explicit Extractor_TW2_DZIP(QString file = "", QString folder = "");

private:
    void extractDZIP(QString exportFolder, QString filename);

    QString _file;
    QString _folder;
    bool _stopped;

    void extractDecompressedFile(QFile &file, QString exportFolder);
    bool decompressFile(QFile &compressedFile, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename);

public slots :
    void run();
    void quitThread();

signals:
    void onProgress(int);
    void finished();
    void error();
};

#endif // TW2_DZIP_EXTRACTOR_H
