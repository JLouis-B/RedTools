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
    // two step ? LZF decompression + parsing ?
    // Decompression
    QByteArray decompressed = decompressLZF(filename);
    std::cout << "buff size = " << decompressed.size() << std::endl;

    // parsing
    QBuffer decompressedStream(&decompressed);
    extractDecompressedFile(decompressedStream, exportFolder);

    emit finished();
}

// seem to be useless yet
QByteArray TW2_DZIP_Extractor::decompressLZF(QString filename)
{
    QByteArray decompressed;
    QFile dzipFile(filename);
    if (!dzipFile.open(QIODevice::ReadOnly))
    {
        emit error();
        return decompressed;
    }

    while (!dzipFile.atEnd())
    {
        decompressed.push_back(dzipFile.read(1));
    }
    return decompressed;
}

void TW2_DZIP_Extractor::extractDecompressedFile(QBuffer& buffer, QString exportFolder)
{
    buffer.open(QIODevice::ReadOnly);
    char magic[5] = "\0";
    buffer.read(magic, 4);

    std::cout << magic << std::endl;

    int unk1, filesCount, unk2;
    qint64 tableAdress;
    buffer.read((char*)&unk1, 4);
    buffer.read((char*)&filesCount, 4);
    buffer.read((char*)&unk2, 4);
    buffer.read((char*)&tableAdress, 8);

    qint64 tablePosition = tableAdress;

    for (int i = 0; i < filesCount; ++i)
    {
        buffer.seek(tablePosition);

        short nsize;
        buffer.read((char*)&nsize, 2);

        char filename[nsize+1] = "\0";
        buffer.read(filename, nsize);
        std::cout << filename << std::endl;

        int unk03, unk04;
        qint64 size, offset, zsize;
        buffer.read((char*)&unk03, 4);
        buffer.read((char*)&unk04, 4);

        buffer.read((char*)&size, 8);
        buffer.read((char*)&offset, 8);
        buffer.read((char*)&zsize, 8);

        std::cout << "position = " << buffer.pos() << std::endl;
        tablePosition = buffer.pos();

        std::cout << "offset = " << offset << std::endl;
        buffer.seek(offset);
        int offsetAdd;
        buffer.read((char*)&offsetAdd, 4);
        int realOffset = buffer.pos() - 4 + offsetAdd;
        zsize -= offsetAdd;

        buffer.seek(realOffset);
        std::cout << "real offset = " << realOffset << std::endl;


        /*
        goto tableoff
        get nsize short
        getdstring name nsize
        get unk03 long
        get unk04 long
        get size longlong
        get offset longlong
        get zsize longlong
        savepos tableoff
        goto offset
        get offadd long
        math offset + offadd
        math zsize - offadd
        clog name offset zsize size
        */
    }
}

void TW2_DZIP_Extractor::quitThread()
{
    _stopped = true;
}
