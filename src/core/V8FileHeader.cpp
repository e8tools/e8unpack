// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "V8FileHeader.h"
#include <QIODevice>
#include <QtEndian>

namespace v8unpack {

bool V8FileHeader::readFrom(QIODevice &device) {
    if (device.bytesAvailable() < kHeaderSize)
        return false;

    QByteArray raw = device.read(kHeaderSize);
    const uchar *p = reinterpret_cast<const uchar*>(raw.constData());

    // Поля читаются в little-endian (формат 1С)
    creationDate     = qFromLittleEndian<quint32>(p + 0x00);
    modificationDate = qFromLittleEndian<quint32>(p + 0x04);
    dataSize         = qFromLittleEndian<quint64>(p + 0x08);
    blockSize        = qFromLittleEndian<quint64>(p + 0x10);
    isPacked         = (p[0x18] != 0);

    // Имя (GUID) хранится как ASCII-строка в последних 8 байтах? — в реальном формате
    // имя может отсутствовать в заголовке и читаться отдельно. Здесь оставлено
    // упрощённо для скелета; в рабочей версии нужно смотреть parse-логику v8unpack.
    name = QString::fromLatin1(raw.mid(0x1F - 8, 8));

    return true;
}

bool V8FileHeader::writeTo(QIODevice &device) const {
    QByteArray raw(kHeaderSize, '\0');
    uchar *p = reinterpret_cast<uchar*>(raw.data());

    qToLittleEndian<quint32>(creationDate,     p + 0x00);
    qToLittleEndian<quint32>(modificationDate, p + 0x04);
    qToLittleEndian<quint64>(dataSize,         p + 0x08);
    qToLittleEndian<quint64>(blockSize,        p + 0x10);
    p[0x18] = isPacked ? 1 : 0;

    QByteArray nameBytes = name.toLatin1().left(8);
    nameBytes.append(8 - nameBytes.size(), '\0');
    std::memcpy(p + 0x1F - 8, nameBytes.constData(), 8);

    return device.write(raw) == raw.size();
}

} // namespace v8