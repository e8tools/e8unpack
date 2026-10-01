// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "MetadataMapper.h"
#include <QRegularExpression>

namespace v8unpack {

MetadataMapper::MetadataMapper(const MetadataMap &map) : m_map(map) {}

QVector<MetadataMapper::MappedEntry> MetadataMapper::map(
    const QVector<V8Container::Entry> &entries) const
{
    QVector<MappedEntry> result;
    result.reserve(entries.size());

    for (const auto &entry : entries) {
        MappedEntry me;
        me.objectGuid = entry.header.name;

        // Пытаемся найти GUID в карте по всем известным типам.
        // В реальном приложении тип можно определить по префиксу GUID
        // или по структуре вложенности контейнера. Здесь — перебор.
        bool found = false;
        for (const QString &typeName : m_map.typeNames()) {
            const QString name = m_map.objectName(typeName, me.objectGuid);
            if (!name.isEmpty()) {
                me.typeName   = typeName;
                me.objectName = name;
                found = true;
                break;
            }
        }

        if (!found) {
            me.typeName   = "Unknown";
            me.objectName = me.objectGuid; // fallback
        }

        me.relativePath = QString("%1/%2").arg(me.typeName, me.objectName);
        result.append(me);
    }

    return result;
}

} // namespace v8