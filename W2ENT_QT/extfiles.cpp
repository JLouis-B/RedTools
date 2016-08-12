#include "extfiles.h"
#include "ui_extfiles.h"

#include "qirrlichtwidget.h"

#include "LoadersUtils.h"

ExtFiles::ExtFiles(QIrrlichtWidget* irrlicht, QWidget *parent) :
    QDialog(parent), _irrlicht(irrlicht),
    ui(new Ui::ExtFiles)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(ui->button_close, SIGNAL(clicked(bool)), this, SLOT(close()));
    QObject::connect(ui->button_selectFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));

    QObject::connect(ui->button_back, SIGNAL(clicked(bool)), this, SLOT(back()));
    QObject::connect(ui->button_checkW2MI, SIGNAL(clicked(bool)), this, SLOT(checkW2MI()));
    QObject::connect(ui->listWidget, SIGNAL(currentTextChanged(QString)), this, SLOT(changeSelection(QString)));
}

ExtFiles::~ExtFiles()
{
    delete ui;
}

void ExtFiles::read(QString filename)
{
    ui->lineEdit->setText(filename);
    ui->listWidget->clear();

    const io::path filenamePath = QSTRING_TO_PATH(filename);
    const WitcherFileType fileType = getFileType(filenamePath);

    switch (fileType)
    {
        case WFT_NOT_WITCHER:
            ui->label_fileType->setText("File type : Not a witcher file");
            return;
            break;

        case WFT_WITCHER_2:
            ui->label_fileType->setText("File type : The Witcher 2 file");
            break;

        case WFT_WITCHER_3:
            ui->label_fileType->setText("File type : The Witcher 3 file");
            break;
    }


    core::array<core::stringc> files = read(filenamePath);
    for (u32 i = 0; i < files.size(); ++i)
    {
        ui->listWidget->addItem(QString(files[i].c_str()));
    }
}


void ExtFiles::selectFile()
{
    QString file = QFileDialog::getOpenFileName(this, "Select the file to analyze", ui->lineEdit->text(), "");
    if (file != "")
    {
        read(file);
    }
}


WitcherFileType ExtFiles::getFileType(io::path filename)
{
    irr::io::IReadFile* file = _irrlicht->getFileSystem()->createAndOpenFile(filename);

    if (!file)
        return WFT_NOT_WITCHER;

    int versionNumber;

    file->seek(4, true);
    file->read(&versionNumber, 4);

    file->drop();

    if (versionNumber == 115)
        return WFT_WITCHER_2;
    else if (versionNumber == 162)
        return WFT_WITCHER_3;
    else
        return WFT_NOT_WITCHER;
}


core::array<core::stringc> ExtFiles::read(io::path filename)
{
    core::array<core::stringc> files;

    const WitcherFileType fileType = getFileType(filename);

    switch (fileType)
    {
        case WFT_NOT_WITCHER:
            break;

        case WFT_WITCHER_2:
            files = ReadTW2File(filename);
            break;

        case WFT_WITCHER_3:
            files = ReadTW3File(filename);
            break;
    }
    return files;
}

// Wicther 2 --------------------------------------
core::stringc readWord(irr::io::IReadFile* file, int nbLetters)
{
    core::stringc str;

    char buf;
    for (int i = 0; i < nbLetters; ++i)
    {
        file->read(&buf, 1);
        if (buf != 0)
        {
            str += buf;
            if (str.size() > 1000)
               break;
        }
    }
    return str;
}

core::array<core::stringc> ExtFiles::ReadTW2File(io::path filename)
{
    irr::io::IReadFile* file = _irrlicht->getFileSystem()->createAndOpenFile(filename);

    core::array<core::stringc> files;
    if (!file)
    {
        return files;
    }

    file->seek(4);

    int data[10];
    file->read(&data, 40);

    core::array<core::stringc> Types;

    file->seek(data[2]);
    for (int i = 0; i < data[3]; i++)
    {
        unsigned char wordSize;
        file->read(&wordSize, 1);
        wordSize -= 128;

        Types.push_back(readWord(file, wordSize));
    }


    for (u32 typeIndex = 0; typeIndex < Types.size(); typeIndex++)
    {
        //Externals files
        file->seek(data[6]);
        for (int i = 0; i < data[7]; i++)
        {
            unsigned char format_name, size;
            file->read(&size, 1);
            file->read(&format_name, 1);

            file->seek(-1, true);

            if (format_name == 1)
                file->seek(1, true);

            core::stringc filename, file_type;
            filename = readWord(file, size);

            int file_typeIndex;
            file->read(&file_typeIndex, 4);
            file_typeIndex -= 1;
            // Type of the file (Cmesh, CmaterielInstance...)
            // file_type = Types[file_typeIndex];

            if ((u32)file_typeIndex == typeIndex)
            {
                //cout << Types[index] << " : " << filename << endl;
                core::stringc file = Types[file_typeIndex] + " : " + filename;
                files.push_back(file);
            }
            //cout << filename << endl;
        }
    }

    file->drop();
    return files;
}

// Witcher 3 -------------------------

bool isAFile(core::stringc string)
{
    return (string.findFirst('.') != -1);
}

core::array<core::stringc> ExtFiles::ReadTW3File(io::path filename)
{
    core::array<core::stringc> files;

    irr::io::IReadFile* file = _irrlicht->getFileSystem()->createAndOpenFile(filename);
    if (!file)
    {
        return files;
    }

    file->seek(12);

    int headerData[38];
    file->read(&headerData, 38 * 4);

    int stringChunkStart = headerData[7];
    int stringChunkSize = headerData[8];
    file->seek(stringChunkStart);
    while (file->getPos() - stringChunkStart < stringChunkSize)
    {
        core::stringc str = readStringUntilNull(file);
        if (isAFile(str))
            files.push_back(str);
        //std::cout << str.c_str() << std::endl;
    }

    file->drop();
    return files;
}

// W2MI stuff ----------------
void ExtFiles::checkW2MI()
{
    _back = ui->lineEdit->text();
    read(Settings::_pack0 + "/" + ui->listWidget->currentItem()->text());
    ui->button_back->setEnabled(true);
}

void ExtFiles::changeSelection(QString newSelectedText)
{
    ui->button_checkW2MI->setEnabled(newSelectedText.contains(".w2mi"));
}

void ExtFiles::back()
{
    read(_back);
    ui->button_back->setEnabled(false);
}
