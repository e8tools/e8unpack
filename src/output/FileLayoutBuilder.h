#ifndef FILELAYOUTBUILDER_H
#define FILELAYOUTBUILDER_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/metadata/MetadataMapper.h"
#include <QString>

namespace v8unpack {

/**
 * Строит файловую структуру на диске согласно метаданным.
 */
class FileLayoutBuilder {
public:
    explicit FileLayoutBuilder(const QString &outputRoot);

    /// Записать один MappedEntry в файловую систему.
    /// Автоматически создаёт подкаталоги.
    bool writeEntry(const MetadataMapper::MappedEntry &entry);

    /// Записать все записи.
    bool writeAll(const QVector<MetadataMapper::MappedEntry> &entries);

private:
    QString m_root;
};

} // namespace v8

#endif // FILELAYOUTBUILDER_H
