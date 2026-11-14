#ifndef UTILS_QT_LOADERS_H
#define UTILS_QT_LOADERS_H

#include <QFile>

namespace UtilsLoadersQt
{
    template <class T>
    T readData(QFile& f)
    {
        T buf{};
        f.read(reinterpret_cast<char*>(&buf), sizeof(T));
        return buf;
    }

    inline qint8   readInt8(QFile& f)   { return readData<qint8>(f); }
    inline qint16  readInt16(QFile& f)  { return readData<qint16>(f); }
    inline qint32  readInt32(QFile& f)  { return readData<qint32>(f); }
    inline qint64  readInt64(QFile& f)  { return readData<qint64>(f); }

    inline quint8  readUInt8(QFile& f)  { return readData<quint8>(f); }
    inline quint16 readUInt16(QFile& f) { return readData<quint16>(f); }
    inline quint32 readUInt32(QFile& f) { return readData<quint32>(f); }
    inline quint64 readUInt64(QFile& f) { return readData<quint64>(f); }

    bool relativeSeek(QFile& file, qint64 value);
    QString readString(QFile& file, quint32 stringSize);
}

#endif // UTILS_QT_LOADERS_H
