// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#ifndef METADATATYPES_H
#define METADATATYPES_H

#include <QString>
#include <QHash>

namespace v8unpack {

// Метаданные 1С
constexpr auto md_Config                      = "Конфигурация";
constexpr auto md_Common                      = "Общие";
constexpr auto md_AccumulationRegisters       = "Регистры накопления";
constexpr auto md_AccountingRegisters         = "Регистры бухгалтерии";
constexpr auto md_CalculationRegisters        = "Регистры расчета";
constexpr auto md_BusinessProcesses           = "Бизнес-процессы";
constexpr auto md_Catalogs                    = "Справочники";
constexpr auto md_ChartsOfCharacteristicTypes = "Планы видов характеристик";
constexpr auto md_CommandGroups               = "Группы команд";
constexpr auto md_CommonAttributes            = "Общие реквизиты";
constexpr auto md_CommonCommands              = "Общие команды";
constexpr auto md_CommonForms                 = "Общие формы";
constexpr auto md_CommonModules               = "Общие модули";
constexpr auto md_CommonPictures              = "Общие картинки";
constexpr auto md_CommonTemplates             = "Общие макеты";
constexpr auto md_Constants                   = "Константы";
constexpr auto md_DataProcessors              = "Обработки";
constexpr auto md_DefinedTypes                = "Определяемые типы";
constexpr auto md_DocumentJournals            = "Журналы документов";
constexpr auto md_DocumentNumerators          = "Нумераторы";
constexpr auto md_Documents                   = "Документы";
constexpr auto md_Enums                       = "Перечисления";
constexpr auto md_EventSubscriptions          = "Подписки на события";
constexpr auto md_ExchangePlans               = "Планы обмена";
constexpr auto md_ChartOfAccounts             = "Планы счетов";
constexpr auto md_ChartOfCalculationTypes     = "Планы видов расчета";
constexpr auto md_ExternalDataSources         = "Внешние источники данных";
constexpr auto md_FilterCriteria              = "Критерии отбора";
constexpr auto md_FunctionalOptions           = "Функциональные опции";
constexpr auto md_FunctionalOptionsParameters = "Параметры функциональных опций";
constexpr auto md_HTTPServices                = "HTTP-сервисы";
constexpr auto md_InformationRegisters        = "Регистры сведений";
constexpr auto md_Interfaces                  = "Интерфейсы";
constexpr auto md_Languages                   = "Языки";
constexpr auto md_Reports                     = "Отчеты";
constexpr auto md_Roles                       = "Роли";
constexpr auto md_Bots                        = "Боты";
constexpr auto md_ScheduledJobs               = "Регламентные задания";
constexpr auto md_SessionParameters           = "Параметры сеанса";
constexpr auto md_SettingsStorages            = "Хранилища настроек";
constexpr auto md_StyleItems                  = "Элементы стиля";
constexpr auto md_Styles                      = "Стили";
constexpr auto md_Subsystems                  = "Подсистемы";
constexpr auto md_Tasks                       = "Задачи";
constexpr auto md_WebServices                 = "Web-сервисы";
constexpr auto md_WSReferences                = "WS-ссылки";
constexpr auto md_WebSocketClients            = "WebSocket-клиенты";
constexpr auto md_XDTOPackages                = "XDTO-пакеты";
constexpr auto md_IntegrationServices         = "Сервисы интеграции";
constexpr auto md_Sequences                   = "Последовательности";
constexpr auto md_Numerators                  = "Нумераторы";

constexpr auto  GUID_Subsystems                  = "37f2fa9a-b276-11d4-9435-004095e12fc7";
constexpr auto  GUID_CommonModules               = "0fe48980-252d-11d6-a3c7-0050bae0a776";
constexpr auto  GUID_SessionParameters           = "24c43748-c938-45d0-8d14-01424a72b11e";
constexpr auto  GUID_Roles                       = "09736b02-9cac-4e3f-b4f7-d3e9576ab948";
constexpr auto  GUID_CommonAttributes            = "15794563-ccec-41f6-a83c-ec5f7b9a5bc1";
constexpr auto  GUID_ExchangePlans               = "857c4a91-e5f4-4fac-86ec-787626f1c108";
constexpr auto  GUID_FilterCriteria              = "3e7bfcc0-067d-11d6-a3c7-0050bae0a776";
constexpr auto  GUID_EventSubscriptions          = "4e828da6-0f44-4b5b-b1c0-a2b3cfe7bdcc";
constexpr auto  GUID_ScheduledJobs               = "11bdaf85-d5ad-4d91-bb24-aa0eee139052";
constexpr auto  GUID_FunctionalOptions           = "af547940-3268-434f-a3e7-e47d6d2638c3";
constexpr auto  GUID_FunctionalOptionsParameters = "30d554db-541e-4f62-8970-a1c6dcfeb2bc";
constexpr auto  GUID_DefinedTypes                = "c045099e-13b9-4fb6-9d50-fca00202971e";
constexpr auto  GUID_SettingsStorages            = "46b4cd97-fd13-4eaa-aba2-3bddd7699218";
constexpr auto  GUID_CommonForms                 = "07ee8426-87f1-11d5-b99c-0050bae0a95d";
constexpr auto  GUID_CommonCommands              = "2f1a5187-fb0e-4b05-9489-dc5dd6412348";
constexpr auto  GUID_CommandGroups               = "1c57eabe-7349-44b3-b1de-ebfeab67b47d";
constexpr auto  GUID_Interfaces                  = "39bddf6a-0c3c-452b-921c-d99cfa1c2f1b";
constexpr auto  GUID_CommonTemplates             = "0c89c792-16c3-11d5-b96b-0050bae0a95d";
constexpr auto  GUID_CommonPictures              = "7dcd43d9-aca5-4926-b549-1842e6a4e8cf";
constexpr auto  GUID_XDTOPackages                = "cc9df798-7c94-4616-97d2-7aa0b7bc515e";
constexpr auto  GUID_WebServices                 = "8657032e-7740-4e1d-a3ba-5dd6e8afb78f";
constexpr auto  GUID_HTTPServices                = "0fffc09c-8f4c-47cc-b41c-8d5c5a221d79";
constexpr auto  GUID_WSReferences                = "d26096fb-7a5d-4df9-af63-47d04771fa9b";
constexpr auto  GUID_WebSocketClients            = "a7641777-7813-45c6-96ef-9d51587a6ac6";
constexpr auto  GUID_IntegrationServices         = "bf3420b0-f6f9-41a0-b83a-fe9d4ab0b65d";
constexpr auto  GUID_StyleItems                  = "58848766-36ea-4076-8800-e91eb49590d7";
constexpr auto  GUID_Styles                      = "3e5404af-6ef8-4c73-ad11-91bd2dfac4c8";
constexpr auto  GUID_Languages                   = "9cd510ce-abfc-11d4-9434-004095e12fc7";
constexpr auto  GUID_Catalogs                    = "cf4abea6-37b2-11d4-940f-008048da11f9";
constexpr auto  GUID_Constants                   = "0195e80c-b157-11d4-9435-004095e12fc7";
constexpr auto  GUID_Documents                   = "061d872a-5787-460e-95ac-ed74ea3a3e84";
constexpr auto  GUID_Numerators                  = "36a8e346-9aaa-4af9-bdbd-83be3c177977";
constexpr auto  GUID_Sequences                   = "bc587f20-35d9-11d6-a3c7-0050bae0a776";
constexpr auto  GUID_JournDocuments              = "4612bd75-71b7-4a5c-8cc5-2b0b65f9fa0d";
constexpr auto  GUID_Enums                       = "f6a80749-5ad7-400b-8519-39dc5dff2542";
constexpr auto  GUID_Reports                     = "631b75a0-29e2-11d6-a3c7-0050bae0a776";
constexpr auto  GUID_DataProcessors              = "bf845118-327b-4682-b5c6-285d2a0eb296";
constexpr auto  GUID_ChartOfCharacteristicTypes  = "82a1b659-b220-4d94-a9bd-14d757b95a48";
constexpr auto  GUID_ChartsOfAccounts            = "238e7e88-3c5f-48b2-8a3b-81ebbecb20ed";
constexpr auto  GUID_ChartsOfCalculationTypes    = "30b100d6-b29f-47ac-aec7-cb8ca8a54767";
constexpr auto  GUID_InformationRegisters        = "13134201-f60b-11d5-a3c7-0050bae0a776";
constexpr auto  GUID_AccumulationRegisters       = "b64d9a40-1642-11d6-a3c7-0050bae0a776";
constexpr auto  GUID_AccountingRegisters         = "2deed9b8-0056-4ffe-a473-c20a6c32a0bc";
constexpr auto  GUID_CalculationRegisters        = "f2de87a8-64e5-45eb-a22d-b3aedab050e7";
constexpr auto  GUID_BusinessProcesses           = "fcd3404e-1523-48ce-9bc0-ecdb822684a1";
constexpr auto  GUID_Tasks                       = "3e63355c-1378-4953-be9b-1deb5fb6bec5";
constexpr auto  GUID_ExternalDataSources         = "5274d9fc-9c3a-4a71-8f5e-a0db8ab23de5";
constexpr auto  GUID_Bots                        = "6e6dc072-b7ac-41e7-8f88-278d25b6da2a";


/// Справочник GUID типов метаданных 1С (взят из v8_reader/guids.h).
/// Ключ — GUID в нижнем регистре без дефисов, значение — человекочитаемое имя.
const QHash<QString, QString>& metadataTypes();

/// Нормализовать GUID: убрать дефисы, привести к нижнему регистру.
QString normalizeGuid(const QString &guid);

} // namespace v8

#endif // METADATATYPES_H