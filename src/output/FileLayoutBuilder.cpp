// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "FileLayoutBuilder.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace v8unpack {

FileLayoutBuilder::FileLayoutBuilder(const QString &outputRoot)
    : m_root(outputRoot) {}

bool FileLayoutBuilder::writeEntry(const MetadataMapper::MappedEntry &entry) {
    const QString dirPath = QDir(m_root).filePath(entry.relativePath);
    QDir().mkpath(dirPath);

    // Имя файла внутри каталога объекта.
    // В реальном приложении здесь нужно анализировать тип данных:
    // модуль → module.bsl, форма → form.json, макет → layout.bin и т.д.
    // Скелет использует универсальное имя с расширением .bin.
    const QString fileName = QDir(dirPath).filePath("data.bin");

    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    return f.write(entry.data) == entry.data.size();
}

bool FileLayoutBuilder::writeAll(const QVector<MetadataMapper::MappedEntry> &entries) {
    bool ok = true;
    for (const auto &e : entries) {
        if (!writeEntry(e))
            ok = false;
    }
    return ok;
}

} // namespace v8