#include "GUI_MaterialsExplorer.h"
#include "ui_GUI_MaterialsExplorer.h"

#include <QPainter>
#include <QTextDocument>
#include <QDesktopServices>
#include <QFileDialog>

#include <iostream>

#include "Utils_Loaders_Irr.h"
#include "Utils_Qt_Irr.h"
#include "Settings.h"

RichTextDelegate::RichTextDelegate(QObject *parent):QItemDelegate(parent)
{
}

void RichTextDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

    if( option.state & QStyle::State_Selected )
        painter->fillRect( option.rect, option.palette.highlight() );


    painter->save();

    QTextDocument document;
    document.setTextWidth(option.rect.width());
    QVariant value = index.data(Qt::DisplayRole);
    if (value.isValid() && !value.isNull())
    {
                document.setHtml(value.toString());
                painter->translate(option.rect.topLeft());
                document.drawContents(painter);

    }

    painter->restore();
}

QTableWidgetItemWithData::QTableWidgetItemWithData(QString richData, QString rawData) : QTableWidgetItem(richData), _rawData(rawData)
{
    setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
}

GUI_MaterialsExplorer::GUI_MaterialsExplorer(QWidget *parent, io::IFileSystem *fs, QString filename) :
    QDialog(parent), _Fs(fs),
    _ui(new Ui::GUI_MaterialsExplorer)
{
    _ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(_ui->button_selectFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
    QObject::connect(_ui->listWidget_materials, SIGNAL(currentRowChanged(int)), this, SLOT(selectMaterial(int)));

    read(filename);

    _ui->tableWidget_properties->setItemDelegateForColumn(2, new RichTextDelegate(_ui->tableWidget_properties));
    QObject::connect(_ui->tableWidget_properties, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(openData(QTableWidgetItem*)));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

GUI_MaterialsExplorer::~GUI_MaterialsExplorer()
{
    delete _ui;
}

void GUI_MaterialsExplorer::selectMaterial(int row)
{
    _ui->tableWidget_properties->setRowCount(0);

    if ((size_t)row < _materials.size())
    {
        std::vector<Property> properties = _materials[row];
        for (auto p : properties)
        {
            _ui->tableWidget_properties->insertRow(_ui->tableWidget_properties->rowCount());
            _ui->tableWidget_properties->setItem(_ui->tableWidget_properties->rowCount() - 1, 0, new QTableWidgetItem(p.name));
            _ui->tableWidget_properties->setItem(_ui->tableWidget_properties->rowCount() - 1, 1, new QTableWidgetItem(p.type));


            QString dataRichText = p.data;
            if (p.type == "handle:ITexture")
                dataRichText = QString("<a href=\"target\">") + p.data + "</a>";

            _ui->tableWidget_properties->setItem(_ui->tableWidget_properties->rowCount() - 1, 2, new QTableWidgetItemWithData(dataRichText, p.data));
        }
    }
}


QString parseTW3Data(core::array<core::stringc>& strings, core::array<core::stringc>& files, QString type, io::IReadFile* file, int size)
{

    if (type == "Float")
        return QString::number(readF32(file));
    else if (type == "handle:ITexture")
    {
        u8 texId = 255 - readU8(file);
        if (texId < files.size())
            return irrStringToQString(files[texId]);
        else
            return "Invalid file";
    }
    else if (type == "Color")
    {
        file->seek(9, true);
        QString r = QString::number(readU8(file));
        file->seek(8, true);
        QString g = QString::number(readU8(file));
        file->seek(8, true);
        QString b = QString::number(readU8(file));
        file->seek(8, true);
        QString a = QString::number(readU8(file));

        return "RGBA = " + r + ", " + g + ", " + b + ", " + a;
    }
    else if (type == "Vector")
    {
        //std::cout << "ADRESS = " << file->getPos() << std::endl;
        file->seek(9, true);
        QString r = QString::number(readF32(file));
        file->seek(8, true);
        QString g = QString::number(readF32(file));
        file->seek(8, true);
        QString b = QString::number(readF32(file));
        file->seek(8, true);
        QString a = QString::number(readF32(file));
        return "XYZW = " + r + ", " + g + ", " + b + ", " + a;
    }
    else
    {
        return QString("Type not implemented. Adress : ") + QString::number(file->getPos());
    }
}

void GUI_MaterialsExplorer::openData(QTableWidgetItem* data)
{
    if (data->column() != 2)
        return;
    if (data->text().indexOf("</a>") == -1)
        return;

    QString rawData = reinterpret_cast<QTableWidgetItemWithData*>(data)->_rawData.replace("\\", "/");
    QFileInfo info(rawData);
    QString base = info.path() + "/" + info.baseName();
    std::cout << base.toStdString().c_str() << std::endl;

    QStringList extensions;
    extensions.push_back(".jpg");
    extensions.push_back(".png");
    extensions.push_back(".tga");
    extensions.push_back(".dds");

    foreach(QString extension, extensions)
    {
        QString fullPath = Settings::_baseDir + "/" + base + extension;
        if (QFile::exists(fullPath))
        {
            QDesktopServices::openUrl(QUrl(fullPath));
            break;
        }
    }
}


void GUI_MaterialsExplorer::ReadIMaterialProperty(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files)
{
    s32 nbProperty = readS32(file);
    //std::cout << "nb property = " << nbProperty << std::endl;
    //std::cout << "adress = " << file->getPos() << std::endl;

    // Read the properties of the material
    std::vector<Property> materialProps;
    for (s32 i = 0; i < nbProperty; ++i)
    {
        const s32 back = file->getPos();

        s32 propSize = readS32(file);

        u16 propId, propTypeId;
        file->read(&propId, 2);
        file->read(&propTypeId, 2);

        if (propId >= strings.size())
            break;

        Property p;
        p.name = strings[propId].c_str();
        p.type = strings[propTypeId].c_str();
        p.data = parseTW3Data(strings, files, p.type, file, propSize);


        materialProps.push_back(p);

        //std::cout << "The property is " << Strings[propId].c_str() << " of the type " << Strings[propTypeId].c_str() << std::endl;
        file->seek(back + propSize);
    }

    _materials.push_back(materialProps);
}

bool ReadPropertyHeader(io::IReadFile* file, SPropertyHeader& propHeader, core::array<core::stringc>& strings)
{
    u16 propName = readU16(file);
    u16 propType = readU16(file);

    if (propName == 0 || propType == 0 || propName >= strings.size() || propType >= strings.size())
        return false;

    propHeader.propName = strings[propName];
    propHeader.propType = strings[propType];

    const long back = file->getPos();
    propHeader.propSize = readS32(file);

    propHeader.endPos = back + propHeader.propSize;

    return true;
}

void GUI_MaterialsExplorer::W3_CMaterialInstance(io::IReadFile* file, W3_DataInfos infos, core::array<core::stringc>& strings, core::array<core::stringc>& files)
{
    file->seek(infos.adress + 1);

    const s32 endOfChunk = infos.adress + infos.size;
    while (file->getPos() < endOfChunk)
    {
        SPropertyHeader propHeader;
        if (!ReadPropertyHeader(file, propHeader, strings))
        {
            file->seek(-2, true);
            ReadIMaterialProperty(file, strings, files);
            return;
        }

        //std::cout << "@" << file->getPos() <<", property = " << property.c_str() << ", type = " << propertyType.c_str() << std::endl;

        /*
        if (propHeader.propName == "baseMaterial")
        {
            u8 fileId = readData<u8>(file);
            fileId = 255 - fileId;
            file->seek(3, true);


            //std::cout << "MATERIAL FILE IS " << Files[fileId].c_str() << std::endl;

            if (core::hasFileExtension(Files[fileId], "w2mi"))
            {
                mat = ReadW2MIFile(ConfigGamePath + Files[fileId]);
                return mat;
            }
        }
        */

        file->seek(propHeader.endPos);
    }
}


void GUI_MaterialsExplorer::loadTW3Materials(io::IReadFile* file)
{
    file->seek(0);
    RedEngineFileHeader header;
    loadTW3FileHeader(file, header);

    file->seek(12);
    core::array<s32> headerData = readDataArray<s32>(file, 38);
    s32 contentChunkStart = headerData[19];
    s32 contentChunkSize = headerData[20];

    file->seek(contentChunkStart);
    for (s32 i = 0; i < contentChunkSize; ++i)
    {
        W3_DataInfos infos;
        u16 dataType = readU16(file);
        core::stringc dataTypeName = header.Strings[dataType];

        file->seek(6, true);

        file->read(&infos.size, 4);
        file->read(&infos.adress, 4);
        //std::cout << "begin at " << infos.adress << " and end at " << infos.adress + infos.size << std::endl;

        file->seek(8, true);

        s32 back = file->getPos();
        if (dataTypeName == "CMaterialInstance")
        {
            _ui->listWidget_materials->addItem("Material " + QString::number(i));
            W3_CMaterialInstance(file, infos, header.Strings, header.Files);
        }
        file->seek(back);
    }
}


// TW2 -------------------------------

bool W2_ReadPropertyHeader(io::IReadFile* file, SPropertyHeader& propHeader, core::array<core::stringc>& strings)
{
    u16 propName = readU16(file);
    u16 propType = readU16(file);

    if (propName == 0 || propType == 0 || propName > strings.size() || propType > strings.size())
        return false;

    propHeader.propName = strings[propName - 1];
    propHeader.propType = strings[propType - 1];

    // The difference with TW3
    file->seek(2, true);

    const long back = file->getPos();
    propHeader.propSize = readS32(file);
    propHeader.endPos = back + propHeader.propSize;

    return true;
}

QString parseTW2Data(core::array<core::stringc>& strings, core::array<core::stringc>& files, QString type, io::IReadFile* file, int size)
{

    if (type == "Float")
        return QString::number(readF32(file));
    else if (type == "*ITexture")
    {
        u8 texId = 255 - readU8(file);
        if (texId < files.size())
            return files[texId].c_str();
        else
            return "Invalid file";
    }
    else if (type == "Color")
    {
        file->seek(9, true);
        QString r = QString::number(readU8(file));
        file->seek(8, true);
        QString g = QString::number(readU8(file));
        file->seek(8, true);
        QString b = QString::number(readU8(file));
        file->seek(8, true);
        QString a = QString::number(readU8(file));

        return "RGBA = " + r + ", " + g + ", " + b + ", " + a;
    }
    else
    {
        return QString("Type not implemented. Adress : ") + QString::number(file->getPos());
    }
}


void GUI_MaterialsExplorer::W2_CMaterialInstance(io::IReadFile* file, ChunkDescriptor infos, core::array<core::stringc>& strings, core::array<core::stringc>& files)
{
    file->seek(infos.adress);

    std::vector<Property> materialProps;

    video::SMaterial material;
    material.MaterialType = video::EMT_SOLID;

    while (1)
    {
        SPropertyHeader propHeader;
        if (!W2_ReadPropertyHeader(file, propHeader, strings))
            break;

        //std::cout << propHeader.propName.c_str() << std::endl;
        //std::cout << propHeader.propType.c_str() << std::endl;
        if (propHeader.propType == "*IMaterial")
        {
            file->seek(1, true); //FilesTable[255-readU8(file)];
            file->seek(6, true); //readUnsignedChars(file, 6);

            s32 nMatElement = readS32(file);

            for (int n = 0; n < nMatElement; n++)
            {
                long propertyStart = file->getPos();
                int propertySize = readS32(file);

                u16 propertyIndex = readU16(file) - 1;
                u16 propertyTypeIndex = readU16(file) - 1;

                core::stringc propertyName = strings[propertyIndex];
                core::stringc propertyType = strings[propertyTypeIndex];


                Property p;
                p.name = strings[propertyIndex].c_str();
                p.type = strings[propertyTypeIndex].c_str();
                p.data = parseTW2Data(strings, files, p.type, file, propertySize);


                materialProps.push_back(p);

                file->seek(propertyStart + propertySize);
            }
        }
        file->seek(propHeader.endPos);
    }
    _materials.push_back(materialProps);
    //std::cout << "Texture : " << FilesTable[255-readUnsignedChars(1)[0]] << std::endl;
}



void GUI_MaterialsExplorer::loadTW2Materials(io::IReadFile* file)
{
    file->seek(4);
    RedEngineFileHeader header;
    loadTW2FileHeader(file, header, true);

    core::array<s32> headerData = readDataArray<s32>(file, 10);


    file->seek(headerData[4]);
    for(int i = 0; i < headerData[5]; ++i) // Now, the list of all the components of the files
    {
        // Read data
        const u16 dataTypeId = readU16(file) - 1;
        core::stringc dataTypeName = header.Strings[dataTypeId];

        core::array<s32> data2 = readDataArray<s32>(file, 5);             // Data info (adress...)

        ChunkDescriptor chunkInfos;
        chunkInfos.size = data2[1];
        chunkInfos.adress = data2[2];

        core::stringc meshSource;

        if (data2[0] == 0)
        {
            const u8 size = readU8(file) - 128;
            const u8 offset = readU8(file);
            if (offset != 1)
                file->seek(-1, true);

            meshSource = readString(file, size);
        }
        else
        {
            readU8(file);
        }


        // now all stuff readed
        const int back2 = file->getPos();

        if (dataTypeName == "CMaterialInstance")
        {
            _ui->listWidget_materials->addItem("Material " + QString::number(i));
            W2_CMaterialInstance(file, chunkInfos, header.Strings, header.Files);
        }

        file->seek(back2);
    }
}

void GUI_MaterialsExplorer::read(QString path)
{
    _ui->lineEdit_selectedFile->setText(path);
    _ui->listWidget_materials->clear();
    _materials.clear();

    _ui->tableWidget_properties->setRowCount(0);

    const io::path filePath = qStringToIrrPath(path);
    io::IReadFile* file = _Fs->createAndOpenFile(filePath);

    RedEngineVersion fileType = getRedEngineFileType(file);
    switch (fileType)
    {
        case REV_UNKNOWN:
            _ui->label_fileType->setText("File type : Not a witcher file");
            break;

        case REV_WITCHER_2:
            loadTW2Materials(file);
            _ui->label_fileType->setText("File type : The Witcher 2 file");
            break;

        case REV_WITCHER_3:
            loadTW3Materials(file);
            _ui->label_fileType->setText("File type : The Witcher 3 file");
            break;
    }

    if (file)
        file->drop();
}


void GUI_MaterialsExplorer::selectFile()
{
    QString defaultDir = _ui->lineEdit_selectedFile->text();
    if (_ui->lineEdit_selectedFile->text().isEmpty())
        defaultDir = Settings::_baseDir;

    QString filePath = QFileDialog::getOpenFileName(this, "Select the file to analyze", defaultDir, "");
    if (!filePath.isEmpty())
    {
        read(filePath);
    }
}
