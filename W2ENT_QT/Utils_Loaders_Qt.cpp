#include "Utils_Loaders_Qt.h"



QString readStringNoCheck(QFile& file, int nbLetters)
{
    char str[nbLetters + 1];
    file.read(str, nbLetters);
    str[nbLetters] = '\0';
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
    qint64 back = file.pos();
    QString returnedString;
    char c;
    for (int i = 0; i < count; ++i)
    {
       file.read(&c, 1);
       if (c == 0x00)
           break;
       returnedString.append(c);
    }

    file.seek(back + count);

    return returnedString;
}
