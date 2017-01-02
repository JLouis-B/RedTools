#include "extfiles.h"
#include "ui_extfiles.h"

#include "qirrlichtwidget.h"

#include "LoadersUtils.h"

ExtFiles::ExtFiles(QIrrlichtWidget* irrlicht, QWidget *parent) :
    QDialog(parent), _irrlicht(irrlicht),
    _ui(new Ui::ExtFiles)
{
    _ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(_ui->button_close, SIGNAL(clicked(bool)), this, SLOT(close()));
    QObject::connect(_ui->button_selectFile, SIGNAL(clicked(bool)), this, SLOT(selectFile()));

    QObject::connect(_ui->button_back, SIGNAL(clicked(bool)), this, SLOT(back()));
    QObject::connect(_ui->button_checkW2MI, SIGNAL(clicked(bool)), this, SLOT(checkW2MI()));
    QObject::connect(_ui->listWidget, SIGNAL(currentTextChanged(QString)), this, SLOT(changeSelection(QString)));
}

ExtFiles::~ExtFiles()
{
    delete _ui;
}

void ExtFiles::read(QString filename)
{
    core::array<core::stringc> files;

    _ui->lineEdit->setText(filename);
    _ui->listWidget->clear();

    const io::path filenamePath = QSTRING_TO_PATH(filename);
    io::IReadFile* file = _irrlicht->getFileSystem()->createAndOpenFile(filenamePath);

    WitcherFileType fileType = checkTWFileFormatVersion(file);
    if (!checkTWFileExtension(filenamePath))
        fileType = WFT_NOT_WITCHER;


    switch (fileType)
    {
        case WFT_NOT_WITCHER:
            _ui->label_fileType->setText("File type : Not a witcher file");
            return;

        case WFT_WITCHER_2:
            files = readTW2File(file);
            _ui->label_fileType->setText("File type : The Witcher 2 file");
            break;

        case WFT_WITCHER_3:
            files = readTW3File(file);
            _ui->label_fileType->setText("File type : The Witcher 3 file");
            break;
    }

    for (u32 i = 0; i < files.size(); ++i)
    {
        _ui->listWidget->addItem(QString(files[i].c_str()));
    }

    file->drop();
}


void ExtFiles::selectFile()
{
    QString file = QFileDialog::getOpenFileName(this, "Select the file to analyze", _ui->lineEdit->text(), "");
    if (file != "")
    {
        read(file);
    }
}

// Witcher 2 --------------------------------------
core::array<core::stringc> ExtFiles::readTW2File(io::IReadFile* file)
{
    core::array<core::stringc> files;

    file->seek(4);

    core::array<s32> header = readDataArray<s32>(file, 10);

    core::array<core::stringc> Strings;

    file->seek(header[2]);
    for (int i = 0; i < header[3]; ++i)
    {
        Strings.push_back(readString(file, readU8(file) -128));
    }


    for (u32 typeIndex = 0; typeIndex < Strings.size(); typeIndex++)
    {
        //Externals files
        file->seek(header[6]);
        for (int i = 0; i < header[7]; i++)
        {
            unsigned char format_name, size;
            file->read(&size, 1);
            file->read(&format_name, 1);

            file->seek(-1, true);

            if (format_name == 1)
                file->seek(1, true);

            core::stringc filename, file_type;
            filename = readString(file, size);

            int file_typeIndex;
            file->read(&file_typeIndex, 4);
            file_typeIndex -= 1;
            // Type of the file (Cmesh, CmaterielInstance...)
            // file_type = Types[file_typeIndex];

            if ((u32)file_typeIndex == typeIndex)
            {
                //cout << Types[index] << " : " << filename << endl;
                core::stringc file = Strings[file_typeIndex] + " : " + filename;
                files.push_back(file);
            }
            //cout << filename << endl;
        }
    }

    return files;
}

// Witcher 3 -------------------------
core::array<core::stringc> ExtFiles::readTW3File(io::IReadFile* file)
{
    core::array<core::stringc> strings;
    core::array<core::stringc> files;

    file->seek(12);

    core::array<s32> headerData = readDataArray<s32>(file, 38);

    s32 stringChunkStart = headerData[7];
    s32 stringChunkSize = headerData[8];
    file->seek(stringChunkStart);
    while (file->getPos() - stringChunkStart < stringChunkSize)
    {
        core::stringc str = readStringUntilNull(file);
        strings.push_back(str);
    }

    s32 nbFiles = headerData[14];
    for (s32 i = 0; i < nbFiles; ++i)
    {
        files.push_back(strings[strings.size() - nbFiles + i]);
    }

    return files;
}

// W2MI stuff ----------------
void ExtFiles::checkW2MI()
{
    _back = _ui->lineEdit->text();
    read(Settings::_pack0 + "/" + _ui->listWidget->currentItem()->text());
    _ui->button_back->setEnabled(true);
}

void ExtFiles::changeSelection(QString newSelectedText)
{
    _ui->button_checkW2MI->setEnabled(newSelectedText.contains(".w2mi"));
}

void ExtFiles::back()
{
    read(_back);
    _ui->button_back->setEnabled(false);
}
