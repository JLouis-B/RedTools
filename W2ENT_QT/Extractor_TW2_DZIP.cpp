#include "Extractor_TW2_DZIP.h"
#include "Log.h"
#include "Utils_Loaders_Qt.h"

#include <QDir>

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
    Log::Instance()->addLineAndFlush(formatString("DZIP: Decompress DZIP file %s", filename.toStdString().c_str()));
    QFile dzipFile(filename);
    if (!dzipFile.open(QIODevice::ReadOnly))
    {
        emit error();
    }

    // parsing
    extractDecompressedFile(dzipFile, exportFolder);

    Log::Instance()->addLineAndFlush("DZIP: Decompression finished");
    emit finished();
}

void Extractor_TW2_DZIP::extractDecompressedFile(QFile& file, QString exportFolder)
{
    char magic[5] = "\0";
    file.read(magic, 4);
    //std::cout << magic << std::endl;

    qint32 unk1        = readInt32(file);
    qint32 filesCount  = readInt32(file);
    qint32 unk2        = readInt32(file);
    qint64 tableAdress = readInt64(file);

    qint64 tablePosition = tableAdress;

    for (int i = 0; i < filesCount; ++i)
    {
        file.seek(tablePosition);

        quint16 nsize = readUInt16(file);
        QString filename = readString(file, nsize);
        //std::cout << filename << std::endl;

        qint32 unk3        = readInt32(file);
        qint32 unk4        = readInt32(file);

        qint64 decompressedSize = readInt64(file);
        qint64 offset           = readInt64(file);
        qint64 compressedSize   = readInt64(file);

        tablePosition = file.pos();


        //std::cout << "compressedSize = " << compressedSize << std::endl;
        //std::cout << "offset = " << offset << std::endl;
        //std::cout << "decompressedSize = " << decompressedSize << std::endl;

        file.seek(offset);
        qint32 offsetAdd = readInt32(file);
        qint64 realOffset = file.pos() - 4 + offsetAdd;
        compressedSize -= offsetAdd;

        file.seek(realOffset);
        //std::cout << "seek to  = " << realOffset << std::endl;

        if (!decompressFile(file, compressedSize, decompressedSize, exportFolder, QString(filename)))
            Log::Instance()->addLineAndFlush(formatString("DZIP: Fail to extract a file : %s", filename.toStdString().c_str()));

        int progression = static_cast<int>(static_cast<float>((i+1) * 100) / filesCount);
        emit onProgress(progression);

        if (_stopped)
        {
            Log::Instance()->addLineAndFlush("DZIP: Decompression stopped");
            break;
        }
    }
}

bool Extractor_TW2_DZIP::decompressFile(QFile& compressedFile, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
{
    // Check TW2_DZIP_Extractor to find some good ref about this

    // read the content of the file
    char* fileContent = new char[compressedSize];
    compressedFile.read(fileContent, compressedSize);

    char* decompressedFileContent = new char[decompressedSize];
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
                delete[] fileContent;
                delete[] decompressedFileContent;
                return false;
            }

            // copy 'length bytes'
            memcpy(&decompressedFileContent[decompressedPosition], &fileContent[compressedPosition], length);
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
                delete[] fileContent;
                delete[] decompressedFileContent;
                return false;
            }

            int positionBack = decompressedPosition - 1 - distance;
            if (positionBack + length > decompressedPosition)
            {
                while (length > 0)
                {
                    decompressedFileContent[decompressedPosition++] = decompressedFileContent[positionBack++];
                    length--;
                }
            }
            else
            {
                memcpy(&decompressedFileContent[decompressedPosition], &decompressedFileContent[positionBack], length);
                decompressedPosition += length;
            }
        }
    }


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
            Log::Instance()->addLineAndFlush(formatString("DZIP: Fail to create file %s", fullPath.toStdString().c_str()));
    }
    else
        Log::Instance()->addLineAndFlush(formatString("DZIP: Fail to create path %s", dir.absolutePath().toStdString().c_str()));

    // not supposed to happen
    if (decompressedSize != decompressedPosition)
    {
        Log::Instance()->addLineAndFlush(formatString("DZIP: decompressedSize != decompressedPosition : %s", fullPath.toStdString().c_str()));
    }

    delete[] fileContent;
    delete[] decompressedFileContent;

    return true;
}

void Extractor_TW2_DZIP::quitThread()
{
    _stopped = true;
}
