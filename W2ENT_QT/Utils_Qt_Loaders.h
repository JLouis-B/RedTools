#ifndef UTILS_QT_LOADERS_H
#define UTILS_QT_LOADERS_H

#include <QFile>

template <class T>
T readData(QFile& f)
{
    T buf;
    f.read((char*)&buf, sizeof(T));
    return buf;
}

#define readInt8 readData<qint8>
#define readInt16 readData<qint16>
#define readInt32 readData<qint32>
#define readInt64 readData<qint64>



QString readStringFixedSize(QFile& file, int count);

#endif // UTILS_QT_LOADERS_H
