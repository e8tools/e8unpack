// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "MetadataMap.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace v8unpack {

bool MetadataMap::loadFromJson(const QString &filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    QJsonObject root = doc.object();
    for (auto typeIt = root.begin(); typeIt != root.end(); ++typeIt) {
        const QString typeName = typeIt.key();
        QJsonObject typeObj = typeIt.value().toObject();
        QJsonObject objects = typeObj.value("objects").toObject();

        QHash<QString, QString> guidToName;
        for (auto objIt = objects.begin(); objIt != objects.end(); ++objIt) {
            const QString guid = objIt.key();
            const QString name = objIt.value().toObject().value("name").toString();
            if (!name.isEmpty())
                guidToName.insert(guid, name);
        }
        m_index.insert(typeName, guidToName);
    }

    return true;
}

bool MetadataMap::isEmpty() const {
    return m_index.isEmpty();
}

QString MetadataMap::objectName(const QString &typeName, const QString &objectGuid) const {
    auto typeIt = m_index.find(typeName);
    if (typeIt == m_index.end())
        return {};

    auto objIt = typeIt->find(objectGuid);
    return (objIt != typeIt->end()) ? *objIt : QString();
}

QStringList MetadataMap::typeNames() const {
    return m_index.keys();
}

std::optional<MetadataMap::ObjectInfo> MetadataMap::lookup(const QString &objectGuid) const
{
    for (auto it = m_index.begin(); it != m_index.end(); ++it) {
            const QString &typeName = it.key();
            const QHash<QString, QString> &objects = it.value();

            auto objIt = objects.find(objectGuid);
            if (objIt != objects.end() && !objIt.value().isEmpty()) {
                return ObjectInfo{ typeName, objIt.value() };
            }
        }
        return std::nullopt;
}

} // namespace v8