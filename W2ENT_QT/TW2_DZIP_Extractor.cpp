#include "TW2_DZIP_Extractor.h"
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
    QFile dzipFile(filename);
    if (!dzipFile.open(QIODevice::ReadOnly))
    {
        emit error();
    }

    // parsing
    extractDecompressedFile(dzipFile, exportFolder);

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
        std::cout << filename << std::endl;

        int unk03, unk04;
        qint64 size, offset, zsize;
        file.read((char*)&unk03, 4);
        file.read((char*)&unk04, 4);

        file.read((char*)&size, 8);
        file.read((char*)&offset, 8);
        file.read((char*)&zsize, 8);

        tablePosition = file.pos();

        /*
        std::cout << "size = " << size << std::endl;
        std::cout << "offset = " << offset << std::endl;
        std::cout << "zsize = " << zsize << std::endl;
        */
        file.seek(offset);
        int offsetAdd;
        file.read((char*)&offsetAdd, 4);
        int realOffset = file.pos() - 4 + offsetAdd;
        zsize -= offsetAdd;
        //std::cout << "new zsize = " << zsize << std::endl;

        file.seek(realOffset);
        //std::cout << "seek to  = " << realOffset << std::endl;

        decompressFile(file, zsize, size, exportFolder, QString(filename));

        int progression = (float)((i+1) * 100) / (float)filesCount;
        emit onProgress(progression);
    }
}

void TW2_DZIP_Extractor::decompressFile(QFile& compressedFile, qint64 compressedSize, qint64 decompressedSize, QString exportFolder, QString filename)
{
    // LZF decompression of the content of the file
    /*
    LZF is a fast compression algorithm that takes very little code space and working memory (assuming we have access to the most recent 8 kByte of decoded text).

    LibLZF by Marc Lehmann is designed to be a very small, very fast, very portable data compression library for the LZF compression algorithm.
    Clipboard


    To do:
    Is LZF also developed by Marc Lehmann ?.

    [3] LibLZF source code [4] [5] [6] [7] [8] [9] [10] [11] [12]

    LZF is used in TuxOnIce and many other applications.

    The LZF decoder inner loop is: [13] [14] [15] For each copy item, the LZF decoder first fetches a byte X from the compressed file, and breaks the byte X into a 3 bit length and a 5 bit high_distance. The decoder has 3 cases: length==0, length==7, and any other length.

            length == 0? (i.e., X in the range 0...31?) Literal: copy the next X+1 literal bytes from the compressed stream and pass them straight through to the current location in the decompressed stream.
            length in 1...6 ? short copy: Use that value as length (later interpreted as a length of 3 to 8 bytes).
            length == 7? long copy: fetch a new length byte and add 7. The new byte could have any value 0 to 255, so the sum is 7 to 262 (which is later interpreted as 9 to 264 bytes).
            Either kind of copy: Fetch a low_distance byte, and combine it with the 5 high_distance bits to get a 13-bit distance. (This implies a 2^13 = 8KByte window).
            Either kind of copy: Find the text that starts that "distance" back from the current end of decoded text, and copy "length+2" characters from that previously-decoded text to end of the decoded text.
        Repeat from the beginning until there is no more items in the compressed file.

    Each LZF "item" is one of these 3 formats:

        000LLLLL <L+1>  ; literal reference
        LLLddddd dddddddd  ; copy L+2 from d bytes before most recently decoded byte
        111ddddd LLLLLLLL dddddddd  ; copy L+2+7 from d bytes before most recently decoded byte
    */

    // read the content of the file
    char* fileContent = new char[compressedSize];
    compressedFile.read(fileContent, compressedSize);
    //std::cout << "fileContent ok" << std::endl;

    char* decompressedFileContent = new char[decompressedSize];
    qint64 compressedPosition = 0, decompressedPosition = 0;
    while (compressedPosition < compressedSize)
    {
        unsigned char byte = fileContent[compressedPosition++];
        unsigned char lengthHeader   = (byte >> 5); //(byte & 0b11100000) >> 5;
        unsigned char high_distance  = (byte & 0b00011111);

        //std::cout << "Compressed cursor=" << compressedPosition << ", decompredd cursor = " << decompressedPosition << std::endl;
        //std::cout << "Chunk : byte=" <<  (int)byte << ", length=" << (int)lengthHeader << ", high_distance=" << (int)high_distance << std::endl;
        if (lengthHeader == 0)
        {
            int length = (int)(byte + 1);
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
            //std::cout << "low distance=" << (int)low_distance << std::endl;
            int distance = (high_distance << 8) | low_distance;
            //std::cout << "distance=" << distance << std::endl;


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
    std::cout << fullPath.toStdString().c_str() << std::endl;
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
            std::cout << "Fail to create file" << std::endl;
    }
    else
        std::cout << "Fail to create path" << std::endl;

    if (decompressedSize != decompressedPosition)
    {
        std::cout << "decompressedSize=" << decompressedSize << std::endl;
        std::cout << "decompressedPosition=" << decompressedPosition << std::endl;
    }

    delete[] fileContent;
    delete[] decompressedFileContent;
}

void TW2_DZIP_Extractor::quitThread()
{
    _stopped = true;
}
