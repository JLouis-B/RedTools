#include "TW2_DZIP_Extractor.h"
#include "log.h"
#include <iostream>

TW2_DZIP_Extractor::TW2_DZIP_Extractor(QString file, QString folder) : _file(file), _folder(folder)
{
    _stopped = false;
}

void TW2_DZIP_Extractor::run()
{
    extractDZIP(_folder, _file);
}

void TW2_DZIP_Extractor::extractDZIP(QString exportFolder, QString filename)
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

void TW2_DZIP_Extractor::extractDecompressedFile(QFile& file, QString exportFolder)
{
    char magic[5] = "\0";
    file.read(magic, 4);
    //std::cout << magic << std::endl;

    int unk1, filesCount, unk2;
    qint64 tableAdress;
    file.read((char*)&unk1, 4);
    file.read((char*)&filesCount, 4);
    file.read((char*)&unk2, 4);
    file.read((char*)&tableAdress, 8);

    qint64 tablePosition = tableAdress;

    for (int i = 0; i < filesCount; ++i)
    {
        file.seek(tablePosition);

        unsigned short nsize;
        file.read((char*)&nsize, 2);

        char filename[nsize] = "\0";
        file.read(filename, nsize);
        //std::cout << filename << std::endl;

        int unk03, unk04;
        qint64 compressedSize, offset, decompressedSize;
        file.read((char*)&unk03, 4);
        file.read((char*)&unk04, 4);

        file.read((char*)&decompressedSize, 8);
        file.read((char*)&offset, 8);
        file.read((char*)&compressedSize, 8);

        tablePosition = file.pos();


        //std::cout << "compressedSize = " << compressedSize << std::endl;
        //std::cout << "offset = " << offset << std::endl;
        //std::cout << "decompressedSize = " << decompressedSize << std::endl;

        file.seek(offset);
        int offsetAdd;
        file.read((char*)&offsetAdd, 4);
        qint64 realOffset = file.pos() - 4 + offsetAdd;
        compressedSize -= offsetAdd;

        file.seek(realOffset);
        //std::cout << "seek to  = " << realOffset << std::endl;

        if (!decompressFile(file, compressedSize, decompressedSize, exportFolder, QString(filename)))
            Log::Instance()->addLineAndFlush(formatString("DZIP: Fail to extract a file : %s", filename));

        int progression = (float)((i+1) * 100) / (float)filesCount;
        emit onProgress(progression);

        if (_stopped)
        {
            Log::Instance()->addLineAndFlush("DZIP: Decompression stopped");
            break;
        }
    }
}

bool TW2_DZIP_Extractor::decompressFile(QFile& compressedFile, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
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

void TW2_DZIP_Extractor::quitThread()
{
    _stopped = true;
}
