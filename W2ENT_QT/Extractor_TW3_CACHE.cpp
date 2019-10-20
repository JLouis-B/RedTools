#include "Extractor_TW3_CACHE.h"
#include "Log.h"
#include "Utils_Loaders_Qt.h"

#include <QDir>

#include <zlib.h>

Extractor_TW3_CACHE::Extractor_TW3_CACHE(QString file, QString folder)
    : _file(file),
      _folder(folder),
      _stopped(false),
      _lastProgression(0)
{
}

void Extractor_TW3_CACHE::run()
{
    extractCACHE(_folder, _file);
}

void Extractor_TW3_CACHE::extractCACHE(QString exportFolder, QString filename)
{
    Log::Instance()->addLineAndFlush(formatString("CACHE: Decompress CACHE file %s", filename.toStdString().c_str()));
    QFile bundleFile(filename);
    if (!bundleFile.open(QIODevice::ReadOnly))
    {
        emit error();
    }

    // parsing
    extractDecompressedFile(bundleFile, exportFolder);

    Log::Instance()->addLineAndFlush("CACHE: Decompression finished");
    emit finished();
}

// ref code : http://jlouisb.users.sourceforge.net/witcher3_bundleOnly.bms
void Extractor_TW3_CACHE::extractDecompressedFile(QFile& file, QString exportFolder)
{
    _lastProgression = 0;
}

bool Extractor_TW3_CACHE::writeDecompressedFile(char* decompressedFileContent, qint64 decompressedSize, QString exportFolder, QString filename)
{
    // create the file
    QString fullPath = exportFolder + "/" + filename;
    //std::cout << fullPath.toStdString().c_str() << std::endl;
    QFileInfo fileInfo(fullPath);
    QDir dir = fileInfo.absoluteDir();
    if (dir.mkpath(dir.absolutePath()))
    {
        QFile decompressedFile(fullPath);
        if (decompressedFile.open(QIODevice::WriteOnly))
        {
            decompressedFile.write(decompressedFileContent, decompressedSize);
            decompressedFile.close();
        }
        else
            Log::Instance()->addLineAndFlush(formatString("CACHE: Fail to create file %s", fullPath.toStdString().c_str()));
    }
    else
        Log::Instance()->addLineAndFlush(formatString("CACHE: Fail to create path %s", dir.absolutePath().toStdString().c_str()));

    return true;
}


void Extractor_TW3_CACHE::quitThread()
{
    _stopped = true;
}
