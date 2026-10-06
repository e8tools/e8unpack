// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#ifndef V8CONTAINER_H
#define V8CONTAINER_H

#include "V8FileHeader.h"
#include <QString>
#include <QVector>
#include <QIODevice>
#include <memory>

namespace v8unpack {

/**
 * Абстракция контейнера 1С (файл .cf / .cfe / .epf).
 * В оригинальном v8unpack контейнер — это дерево элементов,
 * каждый из которых может быть либо файлом, либо вложенным контейнером.
 *
 * Здесь упрощённая модель: контейнер содержит список записей,
 * каждая запись ссылается на данные в родительском QIODevice.
 */
class V8Container {
public:
    struct Entry {
        V8FileHeader header;
        QByteArray   rawData;          // данные как есть (сжатые или нет)
        quint64      dataOffset = 0;   // смещение в родительском потоке
    };

    explicit V8Container(QIODevice *device = nullptr);
    ~V8Container();

    bool open(const QString &filePath);
    bool isOpen() const;

    /// Прочитать оглавление контейнера (список записей)
    bool readTableOfContents();

    /// Извлечь данные записи (при необходимости — распаковать)
    QByteArray extractEntryData(const Entry &entry, bool decompress = true) const;

    const QVector<Entry>& entries() const { return m_entries; }

private:
    std::unique_ptr<QIODevice> m_device;
    QVector<Entry>             m_entries;
    bool                       m_open = false;
};

} // namespace v8

#endif // V8CONTAINER_H
