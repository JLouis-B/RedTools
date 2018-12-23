#include "Utils_TheCouncil.h"

#include <QDir>

#include <iostream>

QFileInfo findFile(QString base, bool checkJSon, bool checkCEF)
{
    QFileInfo fInfo(base);
    QString absolutePath = fInfo.absolutePath();
    QString filename = fInfo.baseName().toLower();
    QString extension = fInfo.suffix().toLower();

    // the case where the extensions change
    if (extension == "fbx")
        extension = "cef";

    std::cout << "absolutePath = " << absolutePath.toStdString().c_str() << std::endl;
    std::cout << "filename = " << filename.toStdString().c_str() << std::endl;

    // search the files
    QDir dirFiles(absolutePath);
    dirFiles.setFilter(QDir::NoDotAndDotDot | QDir::Files);

    QFileInfo bestFile;
    qint64 bestVersion = -1;
    foreach(QFileInfo fileInfo, dirFiles.entryInfoList())
    {
        bool isTheGoodFile = true;

        QString fileInfoFilename = fileInfo.baseName().toLower();
        QString fileInfoExtension = fileInfo.suffix().toLower();

        int pos = fileInfoFilename.lastIndexOf(QChar('_'));
        QString fileInfoVersion = fileInfoFilename.right(pos + 1);
        qint64 fileInfoVersionInt = fileInfoVersion.toInt();
        //std::cout << "version = " << fileInfoVersionInt << std::endl;

        fileInfoFilename = fileInfoFilename.left(pos);

        for (int i = 0; i < fileInfoFilename.size(); ++i)
        {
            if (filename[i] != fileInfoFilename[i])
                isTheGoodFile = false;
        }
        for (int i = 0; i < extension.size(); ++i)
        {
            if (extension[i] != fileInfoExtension[i])
                isTheGoodFile = false;
        }


        //if (fileInfoVersionInt < bestVersion)
        //    isTheGoodFile = false;

        if (checkJSon)
        {
            QFile file;
            file.setFileName(fileInfo.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QByteArray firstBytes = file.read(2);
                if (firstBytes != "{\"")
                    isTheGoodFile = false;
            }
            file.close();
        }

        if (checkCEF)
        {
            QFile file;
            file.setFileName(fileInfo.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QByteArray firstBytes = file.read(3);
                if (firstBytes != "CEF")
                    isTheGoodFile = false;
            }
            file.close();
        }

        if (isTheGoodFile)
        {
            std::cout << "Find a valid file : " << fileInfo.absoluteFilePath().toStdString().c_str() << std::endl;
            bestFile = fileInfo;
            bestVersion = fileInfoVersionInt;
        }
    }
    return bestFile;
}
