#include "Extractor_TW3_BUNDLE.h"
#include "Log/LoggerManager.h"
#include "Utils_Loaders_Qt.h"

#include <QDir>

#include <iostream>
#include <zlib.h>
#include <snappy-c.h>
#include <lz4.h>
#include "DOBOZ/Decompressor.h"

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


    const quint32 dataPosition = file.pos() + fileTableSize;

    // TODO: Add an option in the UI to choose legacy, and refacto the 2 codes
    const bool bUseNewBundleFileVersion = true;
    const quint32 entrySize = bUseNewBundleFileVersion ? 304 : 320;

    const quint32 entryCount = fileTableSize / entrySize;
    if (fileTableSize % entrySize != 0)
    {
        LoggerManager::Instance()->addLineAndFlush("BUNDLE: Something wrong with entries tables");
    }

    std::vector<TW3_BundleEntry> entries;
    entries.reserve(entryCount);
    for (quint32 i = 0; i < entryCount; ++i)
    {
        TW3_BundleEntry entry;
        if (bUseNewBundleFileVersion)
        {
            entry.filename = readString(file, 256);
            //std::cout << "filename : " << filename.toStdString().c_str() << std::endl;

            const QString hash = readString(file, 16);

            entry.offset = readUInt64(file);
            entry.decompressedSize = readUInt32(file);
            entry.compressedSize = readUInt32(file);
            const quint32 crc32 = readUInt32(file);

            entry.compressionType = readInt32(file);
            relativeSeek(file, 8);
        }
        else
        {
            //std::cout << file.pos() << std::endl;
            entry.filename = readString(file, 256);
            QString hash = readString(file, 16);
            //std::cout << "filename : " << filename.toStdString().c_str() << std::endl;

            const qint32 zero = readInt32(file);
            entry.decompressedSize = readUInt32(file);
            entry.compressedSize = readUInt32(file);
            entry.offset = readUInt32(file);
            const qint64 timestamp = readInt64(file);
            /*
                std::cout << "ZERO= " << zero
                  << ", decompressedSize= " << decompressedSize
                  << ", compressedSize= " << compressedSize
                  << ", OFFSET= " << offset
                  << ", TSAMP= " << timestamp << std::endl;
            */

            const qint32 dummy = readInt32(file);
            entry.compressionType = readInt32(file);
        }
        entries.push_back(entry);
    }

    for (size_t i = 0; i < entries.size(); ++i)
    {
        const TW3_BundleEntry& BundleEntry = entries[i];

        file.seek(BundleEntry.offset);
        // read the content of the file
        char* fileContent = new char[BundleEntry.compressedSize];
        file.read(fileContent, BundleEntry.compressedSize);

        switch (BundleEntry.compressionType)
        {
        case 0:
            decompressFileRAW(fileContent, BundleEntry.compressedSize, BundleEntry.decompressedSize, exportFolder, BundleEntry.filename);
            break;
        case 1:
            // comtype zlib
            decompressFileZLIB(fileContent, BundleEntry.compressedSize, BundleEntry.decompressedSize, exportFolder, BundleEntry.filename);
            break;
        case 2:
            // comtype snappy
            decompressFileSNAPPY(fileContent, BundleEntry.compressedSize, BundleEntry.decompressedSize, exportFolder, BundleEntry.filename);
            break;
        case 3:
            // comtype doboz
            decompressFileDOBOZ(fileContent, BundleEntry.compressedSize, BundleEntry.decompressedSize, exportFolder, BundleEntry.filename);
            break;
        case 4:
        case 5:
            // comtype lz4 (4 & 5)
            decompressFileLZ4(fileContent, BundleEntry.compressedSize, BundleEntry.decompressedSize, exportFolder, BundleEntry.filename);
            break;
        default:
            LoggerManager::Instance()->addLineAndFlush(formatString("BUNDLE: NEW COMPRESSION TYPE -> %s (type : %d)", BundleEntry.filename.toStdString().c_str(), BundleEntry.compressionType));
            break;
        }
        delete[] fileContent;


        const int progression = static_cast<int>(static_cast<float>(i + 1) / entryCount * 100.f);
        if (progression > _lastProgression + 2)
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
}

bool Extractor_TW3_BUNDLE::writeDecompressedFile(char* decompressedFileContent, qint64 decompressedSize, QString exportFolder, QString filename)
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

bool Extractor_TW3_BUNDLE::decompressFileRAW(char* fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
{
    // Nothing to do, no compression
    return writeDecompressedFile(fileContent, decompressedSize, exportFolder, filename);
}

bool Extractor_TW3_BUNDLE::decompressFileZLIB(char* fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
{
    char* decompressedFileContent = new char[decompressedSize];

    // STEP 2.
    // inflate b into c
    // zlib struct
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "b" as the input and "c" as the compressed output
    infstream.avail_in = (uInt)compressedSize; // size of input
    infstream.next_in = (Bytef *)fileContent; // input char array
    infstream.avail_out = (uInt)decompressedSize; // size of output
    infstream.next_out = (Bytef *)decompressedFileContent; // output char array

    // the actual DE-compression work.
    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    bool success = writeDecompressedFile(decompressedFileContent, decompressedSize, exportFolder, filename);
    delete[] decompressedFileContent;
    return success;
}

bool Extractor_TW3_BUNDLE::decompressFileSNAPPY(char* fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
{
    // don't seem necessary
    // size_t decompressedSnappySize;
    // snappy_status status = snappy_uncompressed_length(fileContent, compressedSize, &decompressedSnappySize);

    //std::cout << "decompressedSize= " << decompressedSize << std::endl;
    //std::cout << "decompressedSnappySize= " << decompressedSnappySize << std::endl;

    char* decompressedFileContent = new char[decompressedSize];
    size_t decompressedSnappySize = decompressedSize;
    snappy_status status = snappy_uncompress(fileContent, compressedSize, decompressedFileContent, &decompressedSnappySize);
    if (status == SNAPPY_OK)
    {
        bool success = writeDecompressedFile(decompressedFileContent, decompressedSnappySize, exportFolder, filename);
        delete[] decompressedFileContent;
        return success;
    }
    else
    {
        delete[] decompressedFileContent;
        return false;
    }
}

bool Extractor_TW3_BUNDLE::decompressFileDOBOZ(char* fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
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

    char* decompressedFileContent = new char[decompressedSize];
    doboz::Result result = decompressor.decompress(fileContent, compressedSize, decompressedFileContent, decompressedSize);
    if (result == doboz::RESULT_OK)
    {
        bool success = writeDecompressedFile(decompressedFileContent, decompressedSize, exportFolder, filename);
        delete[] decompressedFileContent;
        return success;
    }
    else
    {
        delete[] decompressedFileContent;
        return false;
    }
}

bool Extractor_TW3_BUNDLE::decompressFileLZ4(char* fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
{
    char* decompressedFileContent = new char[decompressedSize];
    if (LZ4_decompress_fast(fileContent, decompressedFileContent, decompressedSize) > 0)
    {
        bool success = writeDecompressedFile(decompressedFileContent, decompressedSize, exportFolder, filename);
        delete[] decompressedFileContent;
        return success;
    }
    else
    {
        delete[] decompressedFileContent;
        return false;
    }
}

void Extractor_TW3_BUNDLE::quitThread()
{
    _stopped = true;
}
