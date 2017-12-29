#include "Utils_Loaders_Qt.h"



QString readString(QFile& file, int nbChars)
{
    char str[nbChars + 1];
    file.read(str, nbChars);
    str[nbChars] = '\0';
    return str;
}

QString readStringFixedSize(QFile &file, int nbChars)
{
    qint64 back = file.pos();
    QString str = readString(file, nbChars);
    file.seek(back + nbChars);

    return str;
}
