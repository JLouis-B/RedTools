#include "Utils_TheCouncil.h"

#include <QDir>

#include <iostream>

QFileInfo findFile(QString base)
{
    QFileInfo fInfo(base);
    QString absolutePath = fInfo.absolutePath();
    QString filename = fInfo.baseName().toLower();

    std::cout << "absolutePath = " << absolutePath.toStdString().c_str() << std::endl;
    std::cout << "filename = " << filename.toStdString().c_str() << std::endl;

    // search the files
    QDir dirFiles(absolutePath);
    dirFiles.setFilter(QDir::NoDotAndDotDot | QDir::Files);

    foreach(QFileInfo fileInfo, dirFiles.entryInfoList())
    {
        bool isTheGoodFile = true;

        QString fileInfoFilename = fileInfo.baseName().toLower();
        for (int i = 0; i < filename.size(); ++i)
        {
            if (filename[i] != fileInfoFilename[i])
                isTheGoodFile = false;
        }

        if (isTheGoodFile)
        {
            std::cout << "Find the file : " << fileInfo.absoluteFilePath().toStdString().c_str() << std::endl;
            return fileInfo;
        }
    }
    return QFileInfo();
}
