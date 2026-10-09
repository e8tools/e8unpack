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
    add(GUID_Section_Forms_ExchangePlans,    sec_Forms);
    add(GUID_Section_Forms_SettingsStorages, sec_Forms);
    add(GUID_Section_Forms_FilterCriteria,   sec_Forms);
    add(GUID_Section_Forms_Documents,        sec_Forms);
    add(GUID_Section_Forms_Reports, sec_Forms);
    add(GUID_Section_Forms_DataProcessors, sec_Forms);
    add(GUID_Section_Forms_Enums, sec_Forms);
    add(GUID_Section_Forms_DocumentJournals, sec_Forms);
    add(GUID_Section_Forms_ChartsOfAccounts, sec_Forms);
    add(GUID_Section_Forms_ChartsOfCharacteristicTypes, sec_Forms);
    add(GUID_Section_Forms_ChartsOfCalculationTypes, sec_Forms);
    add(GUID_Section_Forms_InformationRegisters, sec_Forms);
    add(GUID_Section_Forms_AccumulationRegisters, sec_Forms);
    add(GUID_Section_Forms_AccountingRegisters, sec_Forms);
    add(GUID_Section_Forms_CalculationRegisters, sec_Forms);
    add(GUID_Section_Forms_BusinessProcesses, sec_Forms);
    add(GUID_Section_Forms_Tasks, sec_Forms);

    return m;
}

const QHash<QString, QString>& sectionTypes()
{
    static const QHash<QString, QString> types = buildSections();
    return types;
}

} // namespace v8