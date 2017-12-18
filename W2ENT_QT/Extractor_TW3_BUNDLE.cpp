#include "Extractor_TW3_BUNDLE.h"
#include "Log.h"
#include "Utils_Loaders_Qt.h"

#include <zlib.h>
#include "snappy-c.h"
#include "DOBOZ/Decompressor.h"
#include "lz4.h"

Extractor_TW3_BUNDLE::Extractor_TW3_BUNDLE(QString file, QString folder) : _file(file), _folder(folder), _stopped(false)
{

}

void Extractor_TW3_BUNDLE::run()
{
    extractBUNDLE(_folder, _file);
}

void Extractor_TW3_BUNDLE::extractBUNDLE(QString exportFolder, QString filename)
{
    Log::Instance()->addLineAndFlush(formatString("BUNDLE: Decompress BUNDLE file %s", filename.toStdString().c_str()));
    QFile bundleFile(filename);
    if (!bundleFile.open(QIODevice::ReadOnly))
    {
        emit error();
    }

    // parsing
    extractDecompressedFile(bundleFile, exportFolder);

    Log::Instance()->addLineAndFlush("BUNDLE: Decompression finished");
    emit finished();
}

// ref code : http://jlouisb.users.sourceforge.net/witcher3_bundleOnly.bms
void Extractor_TW3_BUNDLE::extractDecompressedFile(QFile& file, QString exportFolder)
{
    char magic[9] = "\0";
    file.read(magic, 8); // POTATO70
    Log::Instance()->addLineAndFlush(magic);

    qint32 bundleSize = readInt32(file);
    qint32 dummySize = readInt32(file);
    qint32 dataOff = readInt32(file);

    // header size ?
    int infoOff = 0x20;

    int dataPosition = dataOff + infoOff;

    file.seek(infoOff);
    while (file.pos() < dataPosition)
    {
        std::cout << file.pos() << std::endl;
        QString filename = readStringFixedSize(file, 256);
        QString hash = readStringFixedSize(file, 16);
        //std::cout << "filename : " << filename.toStdString().c_str() << std::endl;

        qint32 zero = readInt32(file);
        qint32 decompressedSize = readInt32(file);
        qint32 compressedSize = readInt32(file);
        qint32 offset = readInt32(file);
        qint64 timestamp = readInt64(file);
        /*
        std::cout << "ZERO= " << zero
                  << ", decompressedSize= " << decompressedSize
                  << ", compressedSize= " << compressedSize
                  << ", OFFSET= " << offset
                  << ", TSAMP= " << timestamp << std::endl;
                  */
        /*
        get ZERO long
        get SIZE long
        get ZSIZE long
        get OFFSET long
        get TSTAMP longlong
        */

        QString z = readStringFixedSize(file, 16);

        /*
         *          get DUMMY long
        get ZIP long
        savepos INFO_OFF
        */
        qint32 dummy = readInt32(file);
        qint32 zip = readInt32(file);

        qint64 back = file.pos();

        file.seek(offset);
        // read the content of the file
        char* fileContent = new char[compressedSize];
        file.read(fileContent, compressedSize);

        if (zip == 0)
        {
            // no compression here
            decompressFileRAW(fileContent, compressedSize, decompressedSize, exportFolder, filename);
        }
        else if (zip == 1)
        {
            // comtype zlib
            decompressFileZLIB(fileContent, compressedSize, decompressedSize, exportFolder, filename);
        }
        else if (zip == 2)
        {
            // comtype snappy
            decompressFileSNAPPY(fileContent, compressedSize, decompressedSize, exportFolder, filename);
        }
        else if (zip == 3)
        {
            // comtype doboz
            decompressFileDOBOZ(fileContent, compressedSize, decompressedSize, exportFolder, filename);
        }
        else if (zip == 4 || zip == 5)
        {
            // comtype lz4
            decompressFileLZ4(fileContent, compressedSize, decompressedSize, exportFolder, filename);
        }
        else
        {
            Log::Instance()->addLineAndFlush(formatString("BUNDLE: NEW COMPRESSION TYPE -> %s", filename.toStdString().c_str()));
        }
        delete[] fileContent;
        file.seek(back);
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
        }
        else
            Log::Instance()->addLineAndFlush(formatString("BUNDLE: Fail to create file %s", fullPath.toStdString().c_str()));
    }
    else
        Log::Instance()->addLineAndFlush(formatString("BUNDLE: Fail to create path %s", dir.absolutePath().toStdString().c_str()));

    return true;
}

bool Extractor_TW3_BUNDLE::decompressFileRAW(char* fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
{
    // Nothing to do, no compression
    return writeDecompressedFile(fileContent, decompressedSize, exportFolder, filename);
}

bool Extractor_TW3_BUNDLE::decompressFileZLIB(char *fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
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

bool Extractor_TW3_BUNDLE::decompressFileSNAPPY(char *fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
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

bool Extractor_TW3_BUNDLE::decompressFileDOBOZ(char *fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
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
    if (result != doboz::RESULT_OK)
        return false;

    bool success = writeDecompressedFile(decompressedFileContent, decompressedSize, exportFolder, filename);
    delete[] decompressedFileContent;
    return success;
}

bool Extractor_TW3_BUNDLE::decompressFileLZ4(char *fileContent, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
{
    char* decompressedFileContent = new char[decompressedSize];
    if (LZ4_decompress_fast(fileContent, decompressedFileContent, decompressedSize) > 0)
    {
        bool success = writeDecompressedFile(decompressedFileContent, decompressedSize, exportFolder, filename);
        delete[] decompressedFileContent;
        return success;
    }
    else
        return false;
}

void Extractor_TW3_BUNDLE::quitThread()
{
    _stopped = true;
}
