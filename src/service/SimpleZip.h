#pragma once

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QDateTime>
#include <vector>

// Minimal ZIP writer/reader using STORE (no compression)
class SimpleZipWriter {
public:
    bool open(const QString &path) {
        mFile.setFileName(path);
        return mFile.open(QIODevice::WriteOnly);
    }

    void addFile(const QString &name, const QByteArray &data) {
        // Local file header
        quint32 crc = crc32(data);
        QByteArray header;
        QDataStream hs(&header, QIODevice::WriteOnly);
        hs.setByteOrder(QDataStream::LittleEndian);

        hs << (quint32)0x04034b50;  // signature
        hs << (quint16)20;           // version needed
        hs << (quint16)0;            // flags
        hs << (quint16)0;            // compression: store
        hs << (quint16)timeDate();   // mod time
        hs << (quint16)todayDate();  // mod date
        hs << crc;
        hs << (quint32)data.size();  // compressed size
        hs << (quint32)data.size();  // uncompressed size
        hs << (quint16)name.toUtf8().size();
        hs << (quint16)0;            // extra field length

        mFile.write(header);
        mFile.write(name.toUtf8());
        mFile.write(data);

        // Record for central directory
        Entry e;
        e.name = name;
        e.offset = mOffset;
        e.size = data.size();
        e.crc = crc;
        mEntries.push_back(e);

        mOffset += header.size() + name.toUtf8().size() + data.size();
    }

    void close() {
        quint32 cdOffset = mOffset;
        quint32 cdSize = 0;

        // Central directory
        for (const auto &e : mEntries) {
            QByteArray cd;
            QDataStream cs(&cd, QIODevice::WriteOnly);
            cs.setByteOrder(QDataStream::LittleEndian);

            cs << (quint32)0x02014b50;
            cs << (quint16)20; cs << (quint16)20;
            cs << (quint16)0;           // flags
            cs << (quint16)0;           // compression
            cs << (quint16)timeDate();
            cs << (quint16)todayDate();
            cs << e.crc;
            cs << (quint32)e.size;      // compressed
            cs << (quint32)e.size;      // uncompressed
            cs << (quint16)e.name.toUtf8().size();
            cs << (quint16)0; cs << (quint16)0; cs << (quint16)0;
            cs << (quint32)0;           // external attrs
            cs << (quint32)e.offset;

            mFile.write(cd);
            mFile.write(e.name.toUtf8());
            cdSize += cd.size() + e.name.toUtf8().size();
        }

        // End of central directory
        QByteArray eocd;
        QDataStream es(&eocd, QIODevice::WriteOnly);
        es.setByteOrder(QDataStream::LittleEndian);

        es << (quint32)0x06054b50;
        es << (quint16)0; es << (quint16)0;
        es << (quint16)mEntries.size();
        es << (quint16)mEntries.size();
        es << cdSize;
        es << cdOffset;
        es << (quint16)0;

        mFile.write(eocd);
        mFile.close();
    }

private:
    struct Entry {
        QString name;
        quint32 offset = 0;
        quint32 size = 0;
        quint32 crc = 0;
    };

    quint16 timeDate() const {
        QDateTime now = QDateTime::currentDateTime();
        return (now.time().hour() << 11) | (now.time().minute() << 5) | (now.time().second() / 2);
    }

    quint16 todayDate() const {
        QDate d = QDate::currentDate();
        return ((d.year() - 1980) << 9) | (d.month() << 5) | d.day();
    }

    quint32 crc32(const QByteArray &data) const {
        static quint32 table[256];
        static bool init = false;
        if (!init) {
            for (quint32 i = 0; i < 256; i++) {
                quint32 crc = i;
                for (int j = 0; j < 8; j++)
                    crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
                table[i] = crc;
            }
            init = true;
        }

        quint32 crc = 0xFFFFFFFF;
        for (int i = 0; i < data.size(); i++) {
            crc = (crc >> 8) ^ table[(crc ^ (quint8)data[i]) & 0xFF];
        }
        return crc ^ 0xFFFFFFFF;
    }

    QFile mFile;
    std::vector<Entry> mEntries;
    quint32 mOffset = 0;
};
