#include "Utils_Loaders_Qt.h"



QString readStringNoCheck(QFile& file, int nbLetters)
{
    char str[nbLetters];
    file.read(str, nbLetters);
    return str;
}

QString readString(QFile& file, int nbLetters)
{
    QString str;

    char buf;
    for (int i = 0; i < nbLetters; ++i)
    {
        file.read(&buf, 1);
        if (buf != 0)
            str.append(buf);
    }

    return str;
}

QString readStringFixedSize(QFile &file, int count)
{
    QString returnedString;
    char c;
    while (1)
    {
       file.read(&c, 1);
       if (c == 0x00)
           break;
       returnedString.append(c);
    }

    qint64 back = file.pos();
    file.seek(back + count - (returnedString.size() + 1));

    return returnedString;
}
