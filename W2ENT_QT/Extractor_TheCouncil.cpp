#include "Extractor_TheCouncil.h"
#include "Log/LoggerManager.h"
#include "Utils_Loaders_Qt.h"
#include "Utils_Qt.h"

#include <QVector>
#include <QDir>

using namespace UtilsLoadersQt;

Extractor_TheCouncil::Extractor_TheCouncil(QString file, QString folder)
    : _file(file),
      _folder(folder),
      _stopped(false)

{
}

void Extractor_TheCouncil::run()
{
    extract(_folder, _file);
}


void Extractor_TheCouncil::extract(QString exportFolder, QString filename)
{
    LoggerManager::Instance()->addLineAndFlush(formatString("CPK: Decompress CPK file %s", filename.toStdString().c_str()));
    QFile cpkFile(filename);
    if (!cpkFile.open(QIODevice::ReadOnly))
    {
        emit error();
        return;
    }

    exportFolder = UtilsQt::ensureTrailingSlash(QDir::cleanPath(exportFolder));

    // parsing
    extractCPKFile(cpkFile, exportFolder);

    LoggerManager::Instance()->addLineAndFlush("CPK: Decompression finished");
    emit finished();
}

void Extractor_TheCouncil::extractCPKFile(QFile& file, QString exportFolder)
{
    int lastProgression = 0;

    char magic[5] = "\0";
    file.read(magic, 4); // 55 98 43 01

    quint64 allDataSize = 0;
    const quint32 filesCount = readUInt32(file);
    const QString root = readString(file, 512);
    QVector<TheCouncilFileEntry> filesToUnpack;
    filesToUnpack.reserve(filesCount);
    for(quint32 i = 0; i < filesCount; ++i)
    {
        TheCouncilFileEntry entry;
        entry.size = readUInt32(file);
        entry.offset = readUInt64(file);
        entry.filename = readString(file, 512);
        LoggerManager::Instance()->addLineAndFlush(formatString("-> %s", entry.filename.toStdString().c_str()));

        filesToUnpack.push_back(entry);
        allDataSize += entry.size;
    }

    quint64 processedSize = 0;
    for(int i = 0; i < filesToUnpack.size(); ++i)
    {
        TheCouncilFileEntry entry = filesToUnpack[i];
        file.seek(entry.offset);
        QByteArray fileContent = file.read(entry.size);

        const QString newFileFilename = exportFolder + root + entry.filename;
        QFileInfo newFileInfo(newFileFilename);

        QDir dir = newFileInfo.absoluteDir();
        if (!dir.mkpath(newFileInfo.absoluteDir().absolutePath()))
        {
            LoggerManager::Instance()->addLineAndFlush(formatString("CPK: Fail to mkdir %s", newFileInfo.absoluteDir().absolutePath().toStdString().c_str()));
            return;
        }

        QFile newFile(newFileFilename);
        if (!newFile.open(QIODevice::WriteOnly))
        {
            LoggerManager::Instance()->addLineAndFlush(formatString("CPK: Fail to create a new file %s", newFileFilename.toStdString().c_str()));
            return;
        }
        newFile.write(fileContent);
        newFile.close();

        processedSize += entry.size;
        const int progression = static_cast<int>(static_cast<float>(processedSize) / allDataSize * 100.f);
        if (progression > lastProgression + 1)
        {
            emit onProgress(progression);
            lastProgression = progression;
        }

        if (_stopped)
            break;
    }
}


void Extractor_TheCouncil::quitThread()
{
    _stopped = true;
}
