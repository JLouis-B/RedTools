#include "Utils_Loaders_Qt.h"

void relativeSeek(QFile& file, int value)
{
    file.seek(file.pos() + value);
}

QString readString(QFile& file, int stringSize)
{
    char str[stringSize + 1];
    file.read(str, stringSize);
    str[stringSize] = '\0';
    return str;
}
