#include "Utils_Loaders_Qt.h"

namespace UtilsLoadersQt
{
    bool relativeSeek(QFile& file, qint64 value)
    {
        return file.seek(file.pos() + value);
    }

    QString readString(QFile& file, quint32 stringSize)
    {
        char str[stringSize + 1];
        file.read(str, stringSize);
        str[stringSize] = '\0';
        return str;
    }
}
