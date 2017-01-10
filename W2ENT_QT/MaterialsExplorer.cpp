#include "MaterialsExplorer.h"
#include "ui_MaterialsExplorer.h"
#include "extfiles.h"

MaterialsExplorer::MaterialsExplorer(QWidget *parent, QIrrlichtWidget* irrlicht, QString filename) :
    QDialog(parent), _irrlicht(irrlicht),
    _ui(new Ui::MaterialsExplorer)
{
    _ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(_ui->button_selectFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));
    QObject::connect(_ui->listWidgetMaterials, SIGNAL(currentRowChanged(int)), this, SLOT(selectMaterial(int)));

    read(filename);
}

MaterialsExplorer::~MaterialsExplorer()
{
    delete _ui;
}

void MaterialsExplorer::selectMaterial(int row)
{
    _ui->tableWidget_properties->setRowCount(0);

    if (row < _materials.size())
    {
        std::vector<Property> properties = _materials[row];
        for (auto p : properties)
        {
            _ui->tableWidget_properties->insertRow(_ui->tableWidget_properties->rowCount());
            _ui->tableWidget_properties->setItem(_ui->tableWidget_properties->rowCount() - 1, 0, new QTableWidgetItem(p.name));
            _ui->tableWidget_properties->setItem(_ui->tableWidget_properties->rowCount() - 1, 1, new QTableWidgetItem(p.type));
            _ui->tableWidget_properties->setItem(_ui->tableWidget_properties->rowCount() - 1, 2, new QTableWidgetItem(p.data));
        }
    }
}

QString parseData(core::array<core::stringc>& strings, core::array<core::stringc>& files, QString type, io::IReadFile* file, int size)
{

    if (type == "Float")
        return QString::number(readF32(file));
    else if (type == "handle:ITexture")
    {
        u8 texId = 255 - readU8(file);
        if (texId < files.size())
            return files[texId].c_str();
        else
            return "Invalid file";
    }
    else if (type == "Color")
    {
        /*
        std::cout << "ADRESS = " << file->getPos() << std::endl;
        file->seek(1, true);
        std::cout << "prop = " << strings[readU16(file)].c_str() << ", type = " << strings[readU16(file)].c_str() << std::endl;
        file->seek(5, true);
        std::cout << "prop = " << strings[readU16(file)].c_str() << ", type = " << strings[readU16(file)].c_str() << std::endl;
        file->seek(5, true);
        std::cout << "prop = " << strings[readU16(file)].c_str() << ", type = " << strings[readU16(file)].c_str() << std::endl;
        file->seek(5, true);
        std::cout << "prop = " << strings[readU16(file)].c_str() << ", type = " << strings[readU16(file)].c_str() << std::endl;
        file->seek(5, true);
        */

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
        return "TODO";
}

void MaterialsExplorer::ReadIMaterialProperty(io::IReadFile* file, core::array<core::stringc>& strings, core::array<core::stringc>& files)
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
        p.data = parseData(strings, files, p.type, file, propSize);


        materialProps.push_back(p);

        //std::cout << "The property is " << Strings[propId].c_str() << " of the type " << Strings[propTypeId].c_str() << std::endl;
        /*
        const s32 textureLayer = getTextureLayerFromTextureType(Strings[propId]);
        if (textureLayer != -1)
        {
            u8 texId = readData<u8>(file);
            texId = 255 - texId;

            if (texId < Files.size())
            {
                video::ITexture* texture = 0;
                texture = getTexture(Files[texId]);

                if (texture)
                {
                    log->addAndPush(core::stringc(" ") + Strings[propId].c_str() + " ");
                    mat.setTexture(textureLayer, texture);

                    if (textureLayer == 1)  // normal map
                        mat.MaterialType = video::EMT_NORMAL_MAP_SOLID;
                }
                else
                {
                    Feedback += "Some textures havn't been found, have you correctly set your textures directory ?\n";
                    log->addAndPush(core::stringc("Error : the file ") + Files[texId] + core::stringc(" can't be opened.\n"));
                }
            }
        }
        */
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
    //file->seek(-4, true);

    propHeader.endPos = back + propHeader.propSize;

    return true;
}

void MaterialsExplorer::W3_CMaterialInstance(io::IReadFile* file, W3_DataInfos infos, core::array<core::stringc>& strings, core::array<core::stringc>& files)
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


void MaterialsExplorer::loadTW3Materials(io::IReadFile* file)
{
    file->seek(0);
    core::array<core::stringc> strings, files;
    loadTWStringsAndFiles(file, WFT_WITCHER_3, strings, files, false);

    file->seek(12);
    core::array<s32> headerData = readDataArray<s32>(file, 38);
    s32 contentChunkStart = headerData[19];
    s32 contentChunkSize = headerData[20];

    file->seek(contentChunkStart);
    for (s32 i = 0; i < contentChunkSize; ++i)
    {
        W3_DataInfos infos;
        u16 dataType = readU16(file);
        core::stringc dataTypeName = strings[dataType];

        file->seek(6, true);

        file->read(&infos.size, 4);
        file->read(&infos.adress, 4);
        //std::cout << "begin at " << infos.adress << " and end at " << infos.adress + infos.size << std::endl;

        file->seek(8, true);

        s32 back = file->getPos();
        if (dataTypeName == "CMaterialInstance")
        {
            _ui->listWidgetMaterials->addItem("Material " + QString::number(i));
            W3_CMaterialInstance(file, infos, strings, files);
        }
        else
        {
            //W3_CUnknown(file, infos);
        }
        file->seek(back);
    }
}


void MaterialsExplorer::read(QString filename)
{
    _ui->lineEdit->setText(filename);
    _ui->listWidgetMaterials->clear();
    _materials.clear();

    _ui->tableWidget_properties->setRowCount(0);

    const io::path filenamePath = QSTRING_TO_PATH(filename);
    io::IReadFile* file = _irrlicht->getFileSystem()->createAndOpenFile(filenamePath);

    const WitcherFileType fileType = WFT_WITCHER_3;//checkTWFileFormatVersion(file);

    switch (fileType)
    {
        case WFT_NOT_WITCHER:
            _ui->label_fileType->setText("File type : Not a witcher file");
            return;

        case WFT_WITCHER_2:
            _ui->label_fileType->setText("File type : Not a witcher file");
            return;
            break;

        case WFT_WITCHER_3:
            loadTW3Materials(file);
            _ui->label_fileType->setText("File type : The Witcher 3 file");
            break;
    }

    /*
    int row = _ui->listWidgetMaterials->selectedIndexes().at(0).row();
    if (row < _materials.size())
    {
        std::vector<Property> properties = _materials[row];
        for (auto p : properties)
        {
            _ui->tableWidget_properties->insertRow(_ui->tableWidget_properties->rowCount());
            _ui->tableWidget_properties->setItem(_ui->tableWidget_properties->rowCount() - 1, 0, new QTableWidgetItem(p.name));
            _ui->tableWidget_properties->setItem(_ui->tableWidget_properties->rowCount() - 1, 1, new QTableWidgetItem(p.type));
            _ui->tableWidget_properties->setItem(_ui->tableWidget_properties->rowCount() - 1, 2, new QTableWidgetItem(p.data));
        }
    }*/
}


void MaterialsExplorer::selectFile()
{
    QString file = QFileDialog::getOpenFileName(this, "Select the file to analyze", _ui->lineEdit->text(), "");
    if (file != "")
    {
        read(file);
    }
}
