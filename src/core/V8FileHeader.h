#ifndef V8FILEHEADER_H
#define V8FILEHEADER_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <QString>
#include <cstdint>

class QIODevice;   // forward-declaration глобального Qt-класса — ВНЕ namespace

namespace v8unpack {

/**
 * Заголовок элемента контейнера 1С.
 * В оригинальном v8unpack это структура с полями:
 *   - name (GUID объекта)
 *   - creationDate / modificationDate (в формате 1С)
 *   - dataSize / blockSize
 *   - isPacked (флаг сжатия)
 */
struct V8FileHeader {
    QString  name;              // GUID или служебное имя
    quint32  creationDate = 0;
    quint32  modificationDate = 0;
    quint64  dataSize = 0;
    quint64  blockSize = 0;
    bool     isPacked = false;

    static constexpr quint32 kHeaderSize = 0x1F; // размер заголовка в байтах (по формату 1С)

    bool readFrom(class QIODevice &device);
    bool writeTo(class QIODevice &device) const;
};

} // namespace v8

#endif // V8FILEHEADER_H
