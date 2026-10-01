// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "SectionTypes.h"
#include "src/core/MetadataTypes.h"

namespace v8unpack {

static QHash<QString, QString> buildSections()
{
    QHash<QString, QString> m;
    auto add = [&m](const char* guid, const char* name) {
        m.insert(normalizeGuid(QString::fromLatin1(guid)),
                 QString::fromUtf8(name));
    };

    add(GUID_Section_Forms,           sec_Forms);
    add(GUID_Section_Templates,       sec_Templates);
    add(GUID_Section_Commands,        sec_Commands);
    add(GUID_Section_TabularSections, sec_TabularSections);

    return m;
}

const QHash<QString, QString>& sectionTypes()
{
    static const QHash<QString, QString> types = buildSections();
    return types;
}

} // namespace v8