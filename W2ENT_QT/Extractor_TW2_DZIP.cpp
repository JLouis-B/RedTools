#include "Extractor_TW2_DZIP.h"
#include "Log/LoggerManager.h"
#include "Utils_Loaders_Qt.h"
#include "Utils_Qt.h"

#include <QDir>

using namespace UtilsLoadersQt;

Extractor_TW2_DZIP::Extractor_TW2_DZIP(QString file, QString folder)
    : _file(file),
      _folder(folder),
      _stopped(false)
{
}

void Extractor_TW2_DZIP::run()
{
    extractDZIP(_folder, _file);
}

void Extractor_TW2_DZIP::extractDZIP(QString exportFolder, QString filename)
{
    LoggerManager::Instance()->addLineAndFlush(formatString("DZIP: Decompress DZIP file %s", filename.toStdString().c_str()));
    QFile dzipFile(filename);
    if (!dzipFile.open(QIODevice::ReadOnly))
    {
        emit error();
        return;
    }

    exportFolder = UtilsQt::ensureTrailingSlash(QDir::cleanPath(exportFolder));

    // parsing
    extractDecompressedFile(dzipFile, exportFolder);

    LoggerManager::Instance()->addLineAndFlush("DZIP: Decompression finished");
    emit finished();
}

void Extractor_TW2_DZIP::extractDecompressedFile(QFile& file, QString exportFolder)
{
    int lastProgression = 0;

    char magic[5] = "\0";
    file.read(magic, 4);
    //std::cout << magic << std::endl;

    qint32 unk1        = readInt32(file);
    qint32 filesCount  = readInt32(file);
    qint32 unk2        = readInt32(file);
    qint64 tableAdress = readInt64(file);

    quint64 allDataSize = 0;
    quint64 largestCompressedDataSize = 0;
    quint64 largestDecompressedDataSize = 0;
    std::vector<TW2_FileEntry> entries;
    entries.reserve(filesCount);
    file.seek(tableAdress);
    for (qint32 i = 0; i < filesCount; ++i)
    {
        TW2_FileEntry entry;

        quint16 nsize = readUInt16(file);
        entry.filename = readString(file, nsize);
        //std::cout << filename << std::endl;

        qint32 unk3        = readInt32(file);
        qint32 unk4        = readInt32(file);

        entry.decompressedSize = readInt64(file);
        entry.offset           = readInt64(file);
        entry.compressedSize   = readInt64(file);

        entries.push_back(entry);
        allDataSize += entry.compressedSize;
        if (entry.compressedSize > largestCompressedDataSize)
        {
            largestCompressedDataSize = entry.compressedSize;
        }
        if (entry.decompressedSize > largestDecompressedDataSize)
        {
            largestDecompressedDataSize = entry.decompressedSize;
        }
    }

    if (entries.empty())
    {
        return;
    }

    char* fileContent = new char[largestCompressedDataSize];
    char* decompressedBuffer = new char[largestDecompressedDataSize];
    quint64 processedSize = 0;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        const TW2_FileEntry& FileEntry = entries[i];

        file.seek(FileEntry.offset);

        const qint32 offsetAdd = readInt32(file);
        const qint64 realOffset = file.pos() - 4 + offsetAdd;
        quint32 compressedSize = FileEntry.compressedSize - offsetAdd;

        file.seek(realOffset);
        //std::cout << "seek to  = " << realOffset << std::endl;

        if (!decompressFile(file, fileContent, compressedSize, decompressedBuffer, FileEntry.decompressedSize, exportFolder, FileEntry.filename))
            LoggerManager::Instance()->addLineAndFlush(formatString("DZIP: Fail to extract a file : %s", FileEntry.filename.toStdString().c_str()));

        processedSize += FileEntry.compressedSize;
        const int progression = static_cast<int>(static_cast<float>(processedSize * 100.f) / allDataSize);
        if (progression > lastProgression)
        {
            emit onProgress(progression);
            lastProgression = progression;
        }

        if (_stopped)
        {
            LoggerManager::Instance()->addLineAndFlush("DZIP: Decompression stopped");
            break;
        }
    }

    delete[] fileContent;
    delete[] decompressedBuffer;
}

bool Extractor_TW2_DZIP::decompressFile(QFile& compressedFile, char* fileContent, qint64 compressedSize, char* decompressedBuffer, qint64 decompressedSize, QString exportFolder, QString filename)
{
    // Check TW2_DZIP_Extractor to find some good ref about this

    // read the content of the file
    compressedFile.read(fileContent, compressedSize);

    qint64 compressedPosition = 0, decompressedPosition = 0;
    while (compressedPosition < compressedSize)
    {
        unsigned char byte = fileContent[compressedPosition++];
        unsigned char lengthHeader   = (byte & 0b11100000) >> 5;
        unsigned char high_distance  = (byte & 0b00011111);

        //std::cout << "Compressed cursor=" << compressedPosition << ", decompredd cursor = " << decompressedPosition << std::endl;
        //std::cout << "Chunk : byte=" <<  (int)byte << ", length=" << (int)lengthHeader << ", high_distance=" << (int)high_distance << std::endl;
        if (lengthHeader == 0)
        {
            // get the length of the content
            int length = (int)(byte + 1);

            // 2 files in this case, don't know why
            if (decompressedPosition + length > decompressedSize)
            {
                return false;
            }

            // copy 'length bytes'
            memcpy(&decompressedBuffer[decompressedPosition], &fileContent[compressedPosition], length);
            decompressedPosition += length;
            compressedPosition += length;
        }
        else
        {
            int length = lengthHeader;
            if (lengthHeader == 7)
            {
                unsigned char lengthByte = fileContent[compressedPosition++];
                length += lengthByte;
            }
            length += 2;

            unsigned char low_distance = fileContent[compressedPosition++];
            int distance = (high_distance << 8) | low_distance;
            //std::cout << "distance=" << distance << std::endl;

            // 2 files in this case, don't know why
            if (decompressedPosition + length > decompressedSize)
            {
                return false;
            }

            int positionBack = decompressedPosition - 1 - distance;
            if (positionBack + length > decompressedPosition)
            {
                while (length > 0)
                {
                    decompressedBuffer[decompressedPosition++] = decompressedBuffer[positionBack++];
                    length--;
                }
            }
            else
            {
                memcpy(&decompressedBuffer[decompressedPosition], &decompressedBuffer[positionBack], length);
                decompressedPosition += length;
            }
        }
    }


    // create the file
    QString fullPath = exportFolder + filename;
    //std::cout << fullPath.toStdString().c_str() << std::endl;
    QFileInfo fileInfo(fullPath);
    QDir dir = fileInfo.absoluteDir();
    if (dir.mkpath(dir.absolutePath()))
    {
        QFile decompressedFile(fullPath);
        if (decompressedFile.open(QIODevice::WriteOnly))
        {
            decompressedFile.write(decompressedBuffer, decompressedSize);
            decompressedFile.close();
        }
        else
            LoggerManager::Instance()->addLineAndFlush(formatString("DZIP: Fail to create file %s", fullPath.toStdString().c_str()));
    }
    else
        LoggerManager::Instance()->addLineAndFlush(formatString("DZIP: Fail to create path %s", dir.absolutePath().toStdString().c_str()));

    // not supposed to happen
    if (decompressedSize != decompressedPosition)
    {
        LoggerManager::Instance()->addLineAndFlush(formatString("DZIP: decompressedSize != decompressedPosition : %s", fullPath.toStdString().c_str()));
    }

    return true;
}

void Extractor_TW2_DZIP::quitThread()
{
    _stopped = true;
}
