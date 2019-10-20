#include "Extractor_Dishonored2.h"

#include "Log.h"
#include "Utils_Loaders_Qt.h"

#include <iostream>

#include <QVector>

#include <zlib.h>

Extractor_Dishonored2::Extractor_Dishonored2(QString indexFile, QString resourcesFile, QString folder)
    : _indexFile(indexFile),
      _resourcesFile(resourcesFile),
      _folder(folder),
      _stopped(false),
      _nbProgress(0),
      _lastProgression(0)
{
}

void Extractor_Dishonored2::run()
{
    extract(_folder, _indexFile, _resourcesFile);
}


void Extractor_Dishonored2::extract(QString exportFolder, QString indexFilename, QString resourcesFilename)
{
    Log::Instance()->addLineAndFlush(formatString("Dishonored extractor: Decompress file %s", indexFilename.toStdString().c_str()));
    QFile indexFile(indexFilename);
    if (!indexFile.open(QIODevice::ReadOnly))
    {
        emit error();
    }

    QFile resourcesFile(resourcesFilename);
    if (!resourcesFile.open(QIODevice::ReadOnly))
    {
        emit error();
    }

    QFileInfo inf(indexFile);
    QString sharedResourcesFilename = inf.absolutePath() + "/shared_2_3.sharedrsc";
    QFile sharedResourcesFile(sharedResourcesFilename);
    if (!sharedResourcesFile.open(QIODevice::ReadOnly))
    {
        emit error();
    }

    // parsing
    extractFiles(indexFile, resourcesFile, sharedResourcesFile, exportFolder);

    Log::Instance()->addLineAndFlush("CPK: Decompression finished");
    emit finished();
}

void Extractor_Dishonored2::extractFiles(QFile& indexFile, QFile& resourcesFile, QFile& sharedResourcesFile, QString exportFolder)
{
    indexFile.seek(4);
    quint32 fileSize = __builtin_bswap32(readUInt32(indexFile));
    std::cout << "file size = " << fileSize << std::endl;

    relativeSeek(indexFile, 24);

    quint32 filesCount = __builtin_bswap32(readUInt32(indexFile));
    std::cout << "fileCount = " << filesCount << std::endl;
    for (unsigned int i = 0; i < filesCount; ++i)
    {
        std::cout << "@ = " << indexFile.pos() << std::endl;

        bool useSharedResources = false;

        // read file info
        quint32 idx = __builtin_bswap32(readUInt32(indexFile));

        quint32 string1Size = readUInt32(indexFile);
        QString string1 = readString(indexFile, string1Size);
        std::cout << "-> " << string1.toStdString().c_str() << std::endl;

        quint32 string2Size = readUInt32(indexFile);
        QString string2 = readString(indexFile, string2Size);
        std::cout << "-> " << string2.toStdString().c_str() << std::endl;

        quint32 string3Size = readUInt32(indexFile);
        QString string3 = readString(indexFile, string3Size);
        std::cout << "-> " << string3.toStdString().c_str() << std::endl;

        quint64 offset = __builtin_bswap64(readUInt64(indexFile));
        quint32 size = __builtin_bswap32(readUInt32(indexFile));
        quint32 zSize = __builtin_bswap32(readUInt32(indexFile));
        std::cout << "offset = " << offset << std::endl;
        std::cout << "size = " << size << std::endl;
        std::cout << "zSize = " << zSize << std::endl << std::endl;

        
        relativeSeek(indexFile, 4);
        quint32 flags = __builtin_bswap32(readUInt32(indexFile));
        quint16 flags2 = __builtin_bswap16(readUInt16(indexFile));

        std::cout << "f1 = " << flags << ", f2 = " << flags2 << std::endl;
        if (flags & 32)
        {
            if (flags2 == 0x8000)
            {
                useSharedResources = true;
            }
        }

        if (size == 0)
            continue;

        // zlib decompression 
        QByteArray fileContent;
        if (!useSharedResources)
        {
            resourcesFile.seek(offset);
            fileContent = resourcesFile.read(zSize);
        }
        else
        {
            sharedResourcesFile.seek(offset);
            fileContent = sharedResourcesFile.read(zSize);
            std::cout << "USE SHARED" << std::endl;
        }

        char* decompressedFileContent = new char[size];

        if (size != zSize)
        {
            // STEP 2.
            // inflate b into c
            // zlib struct
            z_stream infstream;
            infstream.zalloc = Z_NULL;
            infstream.zfree = Z_NULL;
            infstream.opaque = Z_NULL;
            // setup "b" as the input and "c" as the compressed output
            infstream.avail_in = (uInt)zSize; // size of input
            infstream.next_in = (Bytef *)fileContent.data(); // input char array
            infstream.avail_out = (uInt)size; // size of output
            infstream.next_out = (Bytef *)decompressedFileContent; // output char array

            // the actual DE-compression work.
            inflateInit(&infstream);
            inflate(&infstream, Z_NO_FLUSH);
            inflateEnd(&infstream);
        }


        // create the file
        QString filename = string3;
        const QString newFileFilename = exportFolder + "/" + filename;
        QFileInfo newFileInfo(newFileFilename);

        QDir dir = newFileInfo.absoluteDir();
        if (!dir.mkpath(newFileInfo.absoluteDir().absolutePath()))
        {
            Log::Instance()->addLineAndFlush(formatString("Dishonored2Extractor: Fail to mkdir %s", newFileInfo.absoluteDir().absolutePath().toStdString().c_str()));
            return;
        }


        QFile newFile(newFileFilename);
        if (!newFile.open(QIODevice::WriteOnly))
        {
            Log::Instance()->addLineAndFlush(formatString("Dishonored2Extractor: Fail to create a new file %s", newFileFilename.toStdString().c_str()));
            return;
        }

        // compressed
        if (size != zSize)
        {
            newFile.write(decompressedFileContent, size);
        }
        else
        {
            newFile.write(fileContent.data(), size);
        }
        newFile.close();

        delete[] decompressedFileContent;

        // threading related stuffsss
        _nbProgress++;
        int progression = static_cast<int>(static_cast<float>(_nbProgress) / filesCount * 100.f);
        if (progression > _lastProgression)
        {
            _lastProgression = progression;
            emit onProgress(progression);
        }

        if (_stopped)
            break;
    }
}


void Extractor_Dishonored2::quitThread()
{
    _stopped = true;
}
