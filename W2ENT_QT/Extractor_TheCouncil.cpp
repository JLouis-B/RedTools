#include "Extractor_TheCouncil.h"
#include "Log/LoggerManager.h"
#include "Utils_Loaders_Qt.h"

#include <QVector>
#include <QDir>

Extractor_TheCouncil::Extractor_TheCouncil(QString file, QString folder)
    : _file(file),
      _folder(folder),
      _stopped(false),
      _nbProgress(0),
      _lastProgression(0)

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
    }

    // parsing
    extractCPKFile(cpkFile, exportFolder);

    LoggerManager::Instance()->addLineAndFlush("CPK: Decompression finished");
    emit finished();
}

void Extractor_TheCouncil::extractCPKFile(QFile& file, QString exportFolder)
{
    char magic[5] = "\0";
    file.read(magic, 4); // 55 98 43 01

    QVector<TheCouncilFileEntry> filesToUnpack;

    quint32 nbFiles = readUInt32(file);
    QString root = readString(file, 512);
    for(quint32 i = 0; i < nbFiles; ++i)
    {
        TheCouncilFileEntry entry;
        entry.size = readUInt32(file);
        entry.offset = readUInt64(file);
        entry.filename = readString(file, 512);
        LoggerManager::Instance()->addLineAndFlush(formatString("-> %s", entry.filename.toStdString().c_str()));

        filesToUnpack.push_back(entry);
    }


    for(int i = 0; i < filesToUnpack.size(); ++i)
    {
        TheCouncilFileEntry entry = filesToUnpack[i];
        file.seek(entry.offset);
        QByteArray fileContent = file.read(entry.size);


        QString filename = entry.filename;
        const QString newFileFilename = exportFolder + "/" + root + filename;
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



        _nbProgress++;
        int progression = static_cast<int>(static_cast<float>(_nbProgress) / filesToUnpack.size() * 100.f);
        if (progression > _lastProgression)
        {
            _lastProgression = progression;
            emit onProgress(progression);
        }

        if (_stopped)
            break;
    }
}


void Extractor_TheCouncil::quitThread()
{
    _stopped = true;
}
