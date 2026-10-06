// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "V8Container.h"
#include <QFile>
#include <zlib.h>
#include <stdexcept>

namespace v8unpack {

V8Container::V8Container(QIODevice *device) : m_device(device) {}
V8Container::~V8Container() = default;

bool V8Container::open(const QString &filePath) {
    auto file = std::make_unique<QFile>(filePath);
    if (!file->open(QIODevice::ReadOnly))
        return false;

    m_device = std::move(file);
    m_open = true;
    return readTableOfContents();
}

bool V8Container::isOpen() const { return m_open; }

bool V8Container::readTableOfContents() {
    m_entries.clear();
    if (!m_device) return false;

    // В реальном v8unpack здесь читается заголовок контейнера,
    // затем в цикле — заголовки элементов и их данные.
    // Скелет: читаем первый заголовок и пытаемся интерпретировать
    // как единичный элемент. Полная реализация должна повторять
    // алгоритм из parse-команды оригинального v8unpack.
    m_device->seek(0);

    V8FileHeader hdr;
    if (!hdr.readFrom(*m_device))
        return false;

    Entry e;
    e.header = hdr;
    e.dataOffset = m_device->pos();
    e.rawData = m_device->read(hdr.blockSize);
    m_entries.append(e);

    return true;
}

QByteArray V8Container::extractEntryData(const Entry &entry, bool decompress) const {
    if (!decompress || !entry.header.isPacked)
        return entry.rawData;

    // Распаковка deflate (zlib). В оригинале используется inflate-логика.
    QByteArray out;
    out.resize(entry.header.dataSize);

    z_stream zs{};
    zs.next_in  = reinterpret_cast<Bytef*>(const_cast<char*>(entry.rawData.constData()));
    zs.avail_in = entry.rawData.size();
    zs.next_out = reinterpret_cast<Bytef*>(out.data());
    zs.avail_out= out.size();

    if (inflateInit(&zs) != Z_OK)
        return {};

    int ret = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);

    if (ret != Z_STREAM_END)
        return entry.rawData; // fallback: вернуть как есть

    out.resize(zs.total_out);
    return out;
}

} // namespace v8