// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "MetadataTypes.h"

namespace v8unpack {

static QHash<QString, QString> buildTypes()
{
    QHash<QString, QString> m;
    /// Ветка Общие
    m.insert(GUID_Subsystems,          QString("Общие/")+QString(md_Subsystems));
    m.insert(GUID_CommonModules,       QString("Общие/")+QString(md_CommonModules));
    m.insert(GUID_SessionParameters,   QString("Общие/")+QString(md_SessionParameters));
    m.insert(GUID_Roles,               QString("Общие/")+QString(md_Roles));
    m.insert(GUID_CommonAttributes,    QString("Общие/")+QString(md_CommonAttributes));
    m.insert(GUID_ExchangePlans,       QString("Общие/")+QString(md_ExchangePlans));
    m.insert(GUID_FilterCriteria,      QString("Общие/")+QString(md_FilterCriteria));
    m.insert(GUID_EventSubscriptions,  QString("Общие/")+QString(md_EventSubscriptions));
    m.insert(GUID_ScheduledJobs,       QString("Общие/")+QString(md_ScheduledJobs));
    m.insert(GUID_Bots,                QString("Общие/")+QString(md_Bots));
    m.insert(GUID_FunctionalOptions,   QString("Общие/")+QString(md_FunctionalOptions));

    m.insert(GUID_FunctionalOptionsParameters, QString("Общие/")+QString(md_FunctionalOptionsParameters));

    m.insert(GUID_DefinedTypes,        QString("Общие/")+QString(md_DefinedTypes));
    m.insert(GUID_SettingsStorages,    QString("Общие/")+QString(md_SettingsStorages));
    m.insert(GUID_CommonCommands,      QString("Общие/")+QString(md_CommonCommands));
    m.insert(GUID_CommandGroups,       QString("Общие/")+QString(md_CommandGroups));
    m.insert(GUID_CommonForms,         QString("Общие/")+QString(md_CommonForms));
    m.insert(GUID_CommonTemplates,     QString("Общие/")+QString(md_CommonTemplates));
    m.insert(GUID_CommonPictures,      QString("Общие/")+QString(md_CommonPictures));
    m.insert(GUID_XDTOPackages,        QString("Общие/")+QString(md_XDTOPackages));
    m.insert(GUID_WebServices,         QString("Общие/")+QString(md_WebServices));
    m.insert(GUID_HTTPServices,        QString("Общие/")+QString(md_HTTPServices));
    m.insert(GUID_WSReferences,        QString("Общие/")+QString(md_WSReferences));
    m.insert(GUID_WebSocketClients,    QString("Общие/")+QString(md_WebSocketClients));
    m.insert(GUID_IntegrationServices, QString("Общие/")+QString(md_IntegrationServices));
    m.insert(GUID_StyleItems,          QString("Общие/")+QString(md_StyleItems));
    m.insert(GUID_Styles,              QString("Общие/")+QString(md_Styles));
    m.insert(GUID_Languages,           QString("Общие/")+QString(md_Languages));
    m.insert(GUID_Interfaces,          QString("Общие/")+QString(md_Interfaces));
    m.insert(GUID_Numerators,          QString("Общие/")+QString(md_Numerators));
    m.insert(GUID_Sequences,           QString("Общие/")+QString(md_Sequences));

    /// Основные метаданные
    ///
    m.insert(GUID_Constants, md_Constants);
    m.insert(GUID_Catalogs, md_Catalogs);
    m.insert(GUID_Documents, md_Documents);
    m.insert(GUID_JournDocuments, md_DocumentJournals);
    m.insert(GUID_Enums, md_Enums);
    m.insert(GUID_Reports, md_Reports);
    m.insert(GUID_DataProcessors, md_DataProcessors);
    m.insert(GUID_ChartOfCharacteristicTypes, md_ChartsOfCharacteristicTypes);
    m.insert(GUID_ChartsOfAccounts, md_ChartOfAccounts);
    m.insert(GUID_ChartsOfCalculationTypes, md_ChartOfCalculationTypes);
    m.insert(GUID_InformationRegisters, md_InformationRegisters);
    m.insert(GUID_AccumulationRegisters, md_AccumulationRegisters);
    m.insert(GUID_AccountingRegisters, md_AccountingRegisters);
    m.insert(GUID_CalculationRegisters, md_CalculationRegisters);
    m.insert(GUID_BusinessProcesses, md_BusinessProcesses);
    m.insert(GUID_Tasks, md_Tasks);
    m.insert(GUID_ExternalDataSources, md_ExternalDataSources);
    return m;
}

const QHash<QString, QString>& metadataTypes()
{
    static const QHash<QString, QString> types = buildTypes();
    return types;
}

QString normalizeGuid(const QString &guid)
{
    // Оставляем GUID как есть, только приводим к нижнему регистру —
    // потому что в файлах 1С GUID иногда встречается в верхнем регистре.
    return guid.toLower();
}

} // namespace v8