#include "Extractor_TW1_BIF.h"
#include "Log/LoggerManager.h"
#include "Utils_Loaders_Qt.h"

#include <QDir>

bool operator< (const ResourceId& a, const ResourceId& b)
{
    if (a._bifId != b._bifId)
        return a._bifId < b._bifId;

    return a._resourceId < b._resourceId;
}

Extractor_TW1_BIF::Extractor_TW1_BIF(QString file, QString folder)
    : _file(file),
      _folder(folder),
      _nbProgress(0),
      _lastProgression(0)
{
    createResourceTypeMap();
    _nbProgress = 0;
    _lastProgression = 0;
    _stopped = false;
}

void Extractor_TW1_BIF::run()
{
    extractKeyBIF(_folder, _file);
}

QString Extractor_TW1_BIF::getExtensionFromResourceType(unsigned short resourceType)
{
    QMap<unsigned short, QString>::iterator it = _fileTypes.find(resourceType);
    if (it == _fileTypes.end())
        return "";
    else
        return "." + it.value();
}

void Extractor_TW1_BIF::extractKeyBIF(QString exportFolder, QString keyFilename)
{
    LoggerManager::Instance()->addLineAndFlush(formatString("BIF: Decompress file %s", keyFilename.toStdString().c_str()));
    QFile keyFile(keyFilename);

    QFileInfo fileInf(keyFile);
    const QString baseFilename = fileInf.absoluteDir().absolutePath();
    const QString bifFolder = baseFilename + "/";

    if (!keyFile.open(QIODevice::ReadOnly))
    {
        LoggerManager::Instance()->addLineAndFlush(formatString("BIF: Fail to open %s", keyFilename.toStdString().c_str()));
        emit error();
        return;
    }

    // magic word "BIFFV1.1"
    keyFile.seek(8);

    quint32 nbFiles = readUInt32(keyFile);
    quint32 nbKeys = readUInt32(keyFile);

    LoggerManager::Instance()->addLineAndFlush(formatString("nbFiles = %d", nbFiles));

    relativeSeek(keyFile, 4);

    quint32 fileOffset = readUInt32(keyFile);
    quint32 keysOffset = readUInt32(keyFile);

    // List of all the files packed
    keyFile.seek(keysOffset);
    for (quint32 i = 0; i < nbKeys; ++i)
    {
        QString name = readString(keyFile, 16);
        //std::cout << name.toStdString().c_str() << std::endl;

        quint16 resourceType = readUInt16(keyFile);
        quint32 resourceId = readUInt32(keyFile);
        quint32 flags = readUInt32(keyFile);

        quint32 bifId = ((flags & 0xFFF00000) >> 20);

        ResourceId rId(bifId, resourceId);

        //std::cout << "resourceId=" << resourceId << ", flags=" << flags << std::endl;
        _resources.insert(rId, name);
    }


    // List of the BIF files where the files are packed
    keyFile.seek(fileOffset);
    for (quint32 i = 0; i < nbFiles; ++i)
    {
        quint32 fileSize = readUInt32(keyFile);
        quint32 filenameOffset = readUInt32(keyFile);
        quint32 filenameSize = readUInt32(keyFile);

        qint64 back = keyFile.pos();
        keyFile.seek(filenameOffset);
        QString filename = readString(keyFile, filenameSize);
        //std::cout << qname.toStdString().c_str() << std::endl;

        if (QFile::exists(bifFolder + filename))
            extractBIF(exportFolder, bifFolder + filename, i);

        keyFile.seek(back);

        if (_stopped)
            break;
    }
    _resources.clear();

    LoggerManager::Instance()->addLineAndFlush("BIF: Decompression finished");
    emit finished();
}

void Extractor_TW1_BIF::extractBIF(QString exportFolder, QString bifFilename, unsigned int bifId)
{
    LoggerManager::Instance()->addLineAndFlush("Read BIF");
    QFile bifFile(bifFilename);

    QFileInfo fileInf(bifFile);
    const QString baseFilename = fileInf.baseName();
    const QString newFileFolder = exportFolder + "/" + baseFilename + "/";

    if (!bifFile.open(QIODevice::ReadOnly))
    {
        LoggerManager::Instance()->addLineAndFlush(formatString("BIF: Fail to open BIF file %s", bifFilename.toStdString().c_str()));
        return;
    }

    QDir dir;
    if (!dir.mkdir(newFileFolder))
    {
        LoggerManager::Instance()->addLineAndFlush(formatString("BIF: Fail to mkdir %s", newFileFolder.toStdString().c_str()));
        return;
    }

    // magic word "BIFFV1.1"
    bifFile.seek(8);

    quint32 nbFiles = readUInt32(bifFile);
    //std::cout << "nbFiles = " << nbFiles << std::endl;

    // unknown
    relativeSeek(bifFile, 4);
    quint32 offset = readUInt32(bifFile);

    bifFile.seek(offset);
    for (unsigned int i = 0; i < nbFiles; ++i)
    {
        quint32 resourceId = readUInt32(bifFile);
        quint32 flags = readUInt32(bifFile);

        //unsigned int bifId = ((flags & 0xFFF00000) >> 20);
        //std::cout << "flags=" << flags << ", resourceId=" << resourceId << std::endl;


        quint32 adress = readUInt32(bifFile);
        quint32 size = readUInt32(bifFile);
        quint16 resourceType = readUInt16(bifFile);

        //std::cout << "adress = " << adress << std::endl;
        //std::cout << "size = " << size << std::endl;
        //std::cout << "pos = " << buffer.pos() << std::endl;

        relativeSeek(bifFile, 2);

        if (resourceId > i)
        {
            // Curiously the last resource entry of djinni.bif seem to be missing
            continue;
        }

        qint64 back = bifFile.pos();
        bifFile.seek(adress);

        ResourceId rId(bifId, resourceId);
        //std::cout << "id="<<  id <<", size=" << size << ", "<< back - 8 << ", i=" << i << std::endl;
        QByteArray fileData = bifFile.read(size);

        QString filename;
        QMap<ResourceId, QString>::iterator it = _resources.find(rId);
        if (it != _resources.end())
            filename = it.value();
        else
            filename = QString::number(i);

        const QString extension = getExtensionFromResourceType(resourceType);
        const QString newFileFilename = newFileFolder + filename + extension;
        QFile newFile(newFileFilename);
        if (!newFile.open(QIODevice::WriteOnly))
        {
            LoggerManager::Instance()->addLineAndFlush(formatString("BIF: Fail to open decompressed file %s", newFileFilename.toStdString().c_str()));
            return;
        }
        newFile.write(fileData);
        newFile.close();

        bifFile.seek(back);

        _nbProgress++;
        int progression = static_cast<int>(static_cast<float>(_nbProgress) / _resources.size() * 100.f);
        if (progression > _lastProgression)
        {
            _lastProgression = progression;
            emit onProgress(progression);
        }

        if (_stopped)
            break;
    }
    bifFile.close();
    LoggerManager::Instance()->addLineAndFlush("Read BIF OK");
}

void Extractor_TW1_BIF::quitThread()
{
    _stopped = true;
}


void Extractor_TW1_BIF::createResourceTypeMap()
{
    _fileTypes.insert(0x0000, "res");
    _fileTypes.insert(0x0001, "bmp");
    _fileTypes.insert(0x0002, "mve");
    _fileTypes.insert(0x0003, "tga");
    _fileTypes.insert(0x0004, "wav");

    _fileTypes.insert(0x0006, "plt");
    _fileTypes.insert(0x0007, "ini");
    _fileTypes.insert(0x0008, "mp3");
    _fileTypes.insert(0x0009, "mpg");
    _fileTypes.insert(0x000A, "txt");
    _fileTypes.insert(0x000B, "xml");

    _fileTypes.insert(0x07D0, "plh");
    _fileTypes.insert(0x07D1, "tex");
    _fileTypes.insert(0x07D2, "mdl");
    _fileTypes.insert(0x07D3, "thg");

    _fileTypes.insert(0x07D5, "fnt");

    _fileTypes.insert(0x07D7, "lua");
    _fileTypes.insert(0x07D8, "slt");
    _fileTypes.insert(0x07D9, "nss");
    _fileTypes.insert(0x07DA, "ncs");
    _fileTypes.insert(0x07DB, "mod");
    _fileTypes.insert(0x07DC, "are");
    _fileTypes.insert(0x07DD, "set");
    _fileTypes.insert(0x07DE, "ifo");
    _fileTypes.insert(0x07DF, "bic");
    _fileTypes.insert(0x07E0, "wok");
    _fileTypes.insert(0x07E1, "2da");
    _fileTypes.insert(0x07E2, "tlk");

    _fileTypes.insert(0x07E6, "txi");
    _fileTypes.insert(0x07E7, "git");
    _fileTypes.insert(0x07E8, "bti");
    _fileTypes.insert(0x07E9, "uti");
    _fileTypes.insert(0x07EA, "btc");
    _fileTypes.insert(0x07EB, "utc");


    _fileTypes.insert(0x07ED, "dlg");
    _fileTypes.insert(0x07EE, "itp");
    _fileTypes.insert(0x07EF, "btt");
    _fileTypes.insert(0x07F0, "utt");
    _fileTypes.insert(0x07F1, "dds");
    _fileTypes.insert(0x07F2, "bts");
    _fileTypes.insert(0x07F3, "uts");
    _fileTypes.insert(0x07F4, "ltr");
    _fileTypes.insert(0x07F5, "gff");
    _fileTypes.insert(0x07F6, "fac");
    _fileTypes.insert(0x07F7, "bte");
    _fileTypes.insert(0x07F8, "ute");
    _fileTypes.insert(0x07F9, "btd");
    _fileTypes.insert(0x07FA, "utd");
    _fileTypes.insert(0x07FB, "btp");
    _fileTypes.insert(0x07FC, "utp");
    _fileTypes.insert(0x07FD, "dft");
    _fileTypes.insert(0x07FE, "gic");
    _fileTypes.insert(0x07FF, "gui");
    _fileTypes.insert(0x0800, "css");
    _fileTypes.insert(0x0801, "ccs");
    _fileTypes.insert(0x0802, "btm");
    _fileTypes.insert(0x0803, "utm");
    _fileTypes.insert(0x0804, "dwk");
    _fileTypes.insert(0x0805, "pwk");
    _fileTypes.insert(0x0806, "btg");

    _fileTypes.insert(0x0808, "jrl");
    _fileTypes.insert(0x0809, "sav");
    _fileTypes.insert(0x080A, "utw");
    _fileTypes.insert(0x080B, "4pc");
    _fileTypes.insert(0x080C, "ssf");

    _fileTypes.insert(0x080F, "bik");
    _fileTypes.insert(0x0810, "ndb");
    _fileTypes.insert(0x0811, "ptm");
    _fileTypes.insert(0x0812, "ptt");
    _fileTypes.insert(0x0813, "ncm");
    _fileTypes.insert(0x0814, "mfx");
    _fileTypes.insert(0x0815, "mat");
    _fileTypes.insert(0x0816, "mdb");
    _fileTypes.insert(0x0817, "say");
    _fileTypes.insert(0x0818, "ttf");
    _fileTypes.insert(0x0819, "ttc");
    _fileTypes.insert(0x081A, "cut");
    _fileTypes.insert(0x081B, "ka");
    _fileTypes.insert(0x081C, "jpg");
    _fileTypes.insert(0x081D, "ico");
    _fileTypes.insert(0x081E, "ogg");
    _fileTypes.insert(0x081F, "spt");
    _fileTypes.insert(0x0820, "spw");
    _fileTypes.insert(0x0821, "wfx");
    _fileTypes.insert(0x0822, "ugm");
    _fileTypes.insert(0x0823, "qdb");
    _fileTypes.insert(0x0824, "qst");
    _fileTypes.insert(0x0825, "npc");
    _fileTypes.insert(0x0826, "spn");
    _fileTypes.insert(0x0827, "utx");
    _fileTypes.insert(0x0828, "mmd");
    _fileTypes.insert(0x0829, "smm");
    _fileTypes.insert(0x082A, "uta");
    _fileTypes.insert(0x082B, "mde");
    _fileTypes.insert(0x082C, "mdv");
    _fileTypes.insert(0x082D, "mda");
    _fileTypes.insert(0x082E, "mba");
    _fileTypes.insert(0x082F, "oct");
    _fileTypes.insert(0x0830, "bfx");
    _fileTypes.insert(0x0831, "pdb");
    _fileTypes.insert(0x0832, "TheWitcherSave");
    _fileTypes.insert(0x0833, "pvs");
    _fileTypes.insert(0x0834, "cfx");
    _fileTypes.insert(0x0835, "luc");

    _fileTypes.insert(0x0837, "prb");
    _fileTypes.insert(0x0838, "cam");
    _fileTypes.insert(0x0839, "vds");
    _fileTypes.insert(0x083A, "bin");
    _fileTypes.insert(0x083B, "wob");
    _fileTypes.insert(0x083C, "api");
    _fileTypes.insert(0x083D, "properties");
    _fileTypes.insert(0x083E, "png");

    _fileTypes.insert(0x270B, "big");

    _fileTypes.insert(0x270D, "erf");
    _fileTypes.insert(0x270E, "bif");
    _fileTypes.insert(0x270F, "key");
}
