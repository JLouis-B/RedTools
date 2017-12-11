#include "Utils_Qt_Loaders.h"

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
