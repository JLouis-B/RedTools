#include "Extractor_TW3_BUNDLE.h"
#include "Log/LoggerManager.h"
#include "Utils_Loaders_Qt.h"

#include <QDir>

#include <iostream>
#include <zlib.h>
#include <snappy-c.h>
#include <lz4.h>
#include "DOBOZ/Decompressor.h"
#include "Utils_Qt.h"

using namespace UtilsLoadersQt;

Extractor_TW3_BUNDLE::Extractor_TW3_BUNDLE(QString file, QString folder)
    : _file(file),
      _folder(folder),
      _stopped(false),
      _lastProgression(0)
{
}

void Extractor_TW3_BUNDLE::run()
{
    extractBUNDLE(_folder, _file);
}

void Extractor_TW3_BUNDLE::extractBUNDLE(QString exportFolder, QString filename)
{
    LoggerManager::Instance()->addLineAndFlush(formatString("BUNDLE: Decompress BUNDLE file %s", filename.toStdString().c_str()));
    QFile bundleFile(filename);
    if (!bundleFile.open(QIODevice::ReadOnly))
    {
        emit error();
        return;
    }

    // parsing
    exportFolder = UtilsQt::ensureTrailingSlash(QDir::cleanPath(exportFolder));
    extractDecompressedFile(bundleFile, exportFolder);

    LoggerManager::Instance()->addLineAndFlush("BUNDLE: Decompression finished");
    emit finished();
}

void Extractor_TW3_BUNDLE::extractDecompressedFile(QFile& file, QString exportFolder)
{
    _lastProgression = 0;

    char magic[9] = "\0";
    file.read(magic, 8); // POTATO70
    LoggerManager::Instance()->addLineAndFlush(magic);

    // Size of the file
    const quint64 bundleFileSize = readUInt64(file);

    // Size of the next part (the list and infos of all files contained)
    const quint32 fileTableSize = readUInt32(file);

    relativeSeek(file, 12);
    // End of header


    // TODO: Add an option in the UI to choose legacy, and refacto the 2 codes
    const bool bUseNewBundleFileVersion = true;
    const quint32 entrySize = bUseNewBundleFileVersion ? 304 : 320;

    const quint32 entryCount = fileTableSize / entrySize;
    if (fileTableSize % entrySize != 0)
    {
        LoggerManager::Instance()->addLineAndFlush("BUNDLE: Something wrong with entries tables");
    }

    quint64 allDataSize = 0;
    quint64 largestCompressedDataSize = 0;
    quint64 largestDecompressedDataSize = 0;
    std::vector<TW3_BundleEntry> entries;
    entries.reserve(entryCount);
    for (quint32 i = 0; i < entryCount; ++i)
    {
        TW3_BundleEntry entry;
        if (bUseNewBundleFileVersion)
        {
            entry.filename = readString(file, 256);
            const QString hash = readString(file, 16);
            //std::cout << "filename : " << filename.toStdString().c_str() << std::endl;

            entry.offset = readUInt64(file);
            entry.decompressedSize = readUInt32(file);
            entry.compressedSize = readUInt32(file);
            const quint32 crc32 = readUInt32(file);

            entry.compressionType = readInt32(file);
            relativeSeek(file, 8);
        }
        else
        {
            entry.filename = readString(file, 256);
            const QString hash = readString(file, 16);
            //std::cout << "filename : " << filename.toStdString().c_str() << std::endl;

            relativeSeek(file, 4); // 0
            entry.decompressedSize = readUInt32(file);
            entry.compressedSize = readUInt32(file);
            entry.offset = readUInt32(file);
            const qint64 timestamp = readInt64(file);

            relativeSeek(file, 4); // dummy
            entry.compressionType = readInt32(file);
        }
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
        const TW3_BundleEntry& BundleEntry = entries[i];

        file.seek(BundleEntry.offset);

        // read the content of the file
        if (file.read(fileContent, BundleEntry.compressedSize) != BundleEntry.compressedSize)
        {
            LoggerManager::Instance()->addLineAndFlush(formatString("BUNDLE: Fail to read the compressed file -> %s)", BundleEntry.filename.toStdString().c_str()));
            continue;
        }

        bool bSuccess = true;
        switch (BundleEntry.compressionType)
        {
        case 0:
            bSuccess = decompressFileRAW(fileContent, BundleEntry.compressedSize, decompressedBuffer, BundleEntry.decompressedSize, exportFolder, BundleEntry.filename);
            break;
        case 1:
            // comtype zlib
            bSuccess = decompressFileZLIB(fileContent, BundleEntry.compressedSize, decompressedBuffer, BundleEntry.decompressedSize, exportFolder, BundleEntry.filename);
            break;
        case 2:
            // comtype snappy
            bSuccess = decompressFileSNAPPY(fileContent, BundleEntry.compressedSize, decompressedBuffer, BundleEntry.decompressedSize, exportFolder, BundleEntry.filename);
            break;
        case 3:
            // comtype doboz
            bSuccess = decompressFileDOBOZ(fileContent, BundleEntry.compressedSize, decompressedBuffer, BundleEntry.decompressedSize, exportFolder, BundleEntry.filename);
            break;
        case 4:
        case 5:
            // comtype lz4 (4 & 5)
            bSuccess = decompressFileLZ4(fileContent, BundleEntry.compressedSize, decompressedBuffer, BundleEntry.decompressedSize, exportFolder, BundleEntry.filename);
            break;
        default:
            bSuccess = false;
            LoggerManager::Instance()->addLineAndFlush(formatString("BUNDLE: NEW COMPRESSION TYPE -> %s (type : %d)", BundleEntry.filename.toStdString().c_str(), BundleEntry.compressionType));
            break;
        }

        if (!bSuccess)
        {
            LoggerManager::Instance()->addLineAndFlush(formatString("BUNDLE: Fail to extract file -> %s)", BundleEntry.filename.toStdString().c_str()));
        }

        processedSize += BundleEntry.compressedSize;
        const int progression = static_cast<int>(static_cast<float>(processedSize) / allDataSize * 100.f);
        if (progression > _lastProgression + 1)
        {
            emit onProgress(progression);
            _lastProgression = progression;
        }

        if (_stopped)
        {
            LoggerManager::Instance()->addLineAndFlush("BUNDLE: Decompression stopped");
            break;
        }
    }
    delete[] fileContent;
    delete[] decompressedBuffer;
}

bool Extractor_TW3_BUNDLE::writeDecompressedFile(char* decompressedFileContent, qint64 decompressedSize, QString exportFolder, QString filename)
{
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
            decompressedFile.write(decompressedFileContent, decompressedSize);
            decompressedFile.close();
            return true;
        }
        else
        {
            LoggerManager::Instance()->addLineAndFlush(formatString("BUNDLE: Fail to create file %s", fullPath.toStdString().c_str()));
        }
    }
    else
    {
        LoggerManager::Instance()->addLineAndFlush(formatString("BUNDLE: Fail to create path %s", dir.absolutePath().toStdString().c_str()));
    }

    return false;
}

bool Extractor_TW3_BUNDLE::decompressFileRAW(char* fileContent, qint64 compressedSize, char* decompressedBuffer, qint64 decompressedSize, QString exportFolder, QString filename)
{
    // Nothing to do, no compression
    return writeDecompressedFile(fileContent, compressedSize, exportFolder, filename);
}

bool Extractor_TW3_BUNDLE::decompressFileZLIB(char* fileContent, qint64 compressedSize, char* decompressedBuffer, qint64 decompressedSize, QString exportFolder, QString filename)
{
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree  = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in  = (uInt)compressedSize;
    infstream.next_in   = (Bytef*)fileContent;
    infstream.avail_out = (uInt)decompressedSize;
    infstream.next_out  = (Bytef*)decompressedBuffer;

    // the actual DE-compression work.
    if (inflateInit(&infstream) != Z_OK)
    {
        return false;
    }

    const int result = inflate(&infstream, Z_FINISH);
    inflateEnd(&infstream);

    if (result != Z_STREAM_END)
    {
        return false;
    }

    return writeDecompressedFile(decompressedBuffer, infstream.total_out, exportFolder, filename);
}

bool Extractor_TW3_BUNDLE::decompressFileSNAPPY(char* fileContent, qint64 compressedSize, char* decompressedBuffer, qint64 decompressedSize, QString exportFolder, QString filename)
{
    // don't seem necessary
    // size_t decompressedSnappySize;
    // snappy_status status = snappy_uncompressed_length(fileContent, compressedSize, &decompressedSnappySize);

    //std::cout << "decompressedSize= " << decompressedSize << std::endl;
    //std::cout << "decompressedSnappySize= " << decompressedSnappySize << std::endl;

    size_t decompressedSnappySize = decompressedSize;
    snappy_status status = snappy_uncompress(fileContent, compressedSize, decompressedBuffer, &decompressedSnappySize);
    if (status == SNAPPY_OK)
    {
        bool success = writeDecompressedFile(decompressedBuffer, decompressedSnappySize, exportFolder, filename);
        return success;
    }
    else
    {
        return false;
    }
}

bool Extractor_TW3_BUNDLE::decompressFileDOBOZ(char* fileContent, qint64 compressedSize, char* decompressedBuffer, qint64 decompressedSize, QString exportFolder, QString filename)
{
    doboz::Decompressor decompressor;

    // don't seem necessary
    /*
    doboz::CompressionInfo compressionInfo;
    doboz::Result result = decompressor.getCompressionInfo(fileContent, compressedSize, compressionInfo);
    if (result != doboz::RESULT_OK)
        return false;
    size_t outputSize = static_cast<size_t>(compressionInfo.uncompressedSize);
    */

    doboz::Result result = decompressor.decompress(fileContent, compressedSize, decompressedBuffer, decompressedSize);
    if (result == doboz::RESULT_OK)
    {
        bool success = writeDecompressedFile(decompressedBuffer, decompressedSize, exportFolder, filename);
        return success;
    }
    else
    {
        return false;
    }
}

bool Extractor_TW3_BUNDLE::decompressFileLZ4(char* fileContent, qint64 compressedSize, char* decompressedBuffer, qint64 decompressedSize, QString exportFolder, QString filename)
{
    if (LZ4_decompress_safe(fileContent, decompressedBuffer, compressedSize, decompressedSize) > 0)
    {
        bool success = writeDecompressedFile(decompressedBuffer, decompressedSize, exportFolder, filename);
        return success;
    }
    else
    {
        return false;
    }
}

void Extractor_TW3_BUNDLE::quitThread()
{
    _stopped = true;
}
