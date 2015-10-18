#include "cleantexturespath.h"
#include "ui_cleantexturespath.h"

#include <iostream>

CleanTexturesPath::CleanTexturesPath(QString texPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CleanTexturesPath), _texPath(texPath)
{
    ui->setupUi(this);
}

CleanTexturesPath::~CleanTexturesPath()
{
    delete ui;
}

void CleanTexturesPath::clean()
{
    QDir dirFiles(_texPath);
    dirFiles.setFilter(QDir::NoDotAndDotDot | QDir::Files);
    dirFiles.setNameFilters(QStringList() << "*.dds");


    int nbTexs = dirFiles.entryInfoList().size();
    ui->progressBar->setMaximum(nbTexs);
    int i = 0;
    foreach(QFileInfo fileInfo, dirFiles.entryInfoList())
    {
        // change filename
        QString base = fileInfo.baseName();
        QString prefix = base.left(7);
        if (prefix == "unpack-")
        {
            base = base.right(base.size() - 7);
            //std::cout << base.toStdString().c_str() << std::endl;
        }

        QString extension = fileInfo.suffix();
        QString newPath = fileInfo.absoluteDir().absolutePath() + "/" + base + "." + extension;


        if (!QFile::rename(fileInfo.absoluteFilePath() , newPath))
        {
            //std::cout << "fail to load " << fileInfo.absoluteFilePath().toStdString().c_str() << std::endl;
        }

        ++i;
        // update UI
        if (i%500 == 0)
        {
            ui->progressBar->setValue(i);
            ui->label_progression->setText("Texture " + QString::number(i) + " of " + QString::number(nbTexs));
            ui->label_tex->setText(fileInfo.baseName());
            //std::cout << newPath.toStdString().c_str() << std::endl;
            QCoreApplication::processEvents();
        }

    }
}
