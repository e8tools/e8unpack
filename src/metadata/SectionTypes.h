#ifndef SECTIONTYPES_H
#define SECTIONTYPES_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <QString>
#include <QHash>

namespace v8unpack {

// Человекочитаемые имена секций внутри объектов
constexpr auto sec_Forms            = "Формы";
constexpr auto sec_Attributes       = "Реквизиты";
constexpr auto sec_TabularSections  = "ТабличныеЧасти";
constexpr auto sec_Commands         = "Команды";
constexpr auto sec_Templates        = "Макеты";

// GUID-ы секций внутри объектов метаданных
constexpr auto GUID_Section_Forms           = "fdf816d2-1ead-11d5-b975-0050bae0a95d";
constexpr auto GUID_Section_Templates       = "3daea016-69b7-4ed4-9453-127911372fe6";  // Макеты
constexpr auto GUID_Section_Commands        = "4fe87c89-9ad4-43f6-9fdb-9dc83b3879c6";  // Команды
constexpr auto GUID_Section_TabularSections = "932159f9-95b2-4e76-a8dd-8849fe5c5ded";  // Табличные
// Секции «Формы» у разных типов объектов имеют собственные GUID-ы
// (установлено по реальным данным БП, проверено на 1Cv8_common.cf).
constexpr auto GUID_Section_Forms_ExchangePlans    = "87c509ab-3d38-4d67-b379-aca796298578";  // Формы планов обмена
constexpr auto GUID_Section_Forms_SettingsStorages = "b8533c0c-2342-4db3-91a2-c2b08cbf6b23";  // Формы хранилищ настроек
constexpr auto GUID_Section_Forms_FilterCriteria   = "00867c40-06b1-11d6-a3c7-0050bae0a776";  // Формы критериев отбора
constexpr auto GUID_Section_Forms_Documents        = "fb880e93-47d7-4127-9357-a20e69c17545";  // Формы документов
constexpr auto GUID_Section_Forms_Reports                       = "a3b368c0-29e2-11d6-a3c7-0050bae0a776";  // Формы отчетов
constexpr auto GUID_Section_Forms_DataProcessors                = "d5b0e5ed-256d-401c-9c36-f630cafd8a62";  // Формы обработок
constexpr auto GUID_Section_Forms_Enums                         = "33f2e54b-37ce-4a7a-a569-b648d7aa4634";  // Формы перечислений
constexpr auto GUID_Section_Forms_DocumentJournals              = "ec81ad10-ca07-11d5-b9a5-0050bae0a95d";  // Формы журналов документов
constexpr auto GUID_Section_Forms_ChartsOfAccounts              = "5372e285-03db-4f8c-8565-fe56f1aea40e";  // Формы планов счетов
constexpr auto GUID_Section_Forms_ChartsOfCharacteristicTypes   = "eb2b78a8-40a6-4b7e-b1b3-6ca9966cbc94";  // Формы планов видов характеристик
constexpr auto GUID_Section_Forms_ChartsOfCalculationTypes      = "a7f8f92a-7a4b-484b-937e-42d242e64144";  // Формы планов видов расчета
constexpr auto GUID_Section_Forms_InformationRegisters          = "13134204-f60b-11d5-a3c7-0050bae0a776";  // Формы регистров сведений
constexpr auto GUID_Section_Forms_AccumulationRegisters         = "b64d9a44-1642-11d6-a3c7-0050bae0a776";  // Формы регистров накопления
constexpr auto GUID_Section_Forms_AccountingRegisters           = "d3b5d6eb-4ea2-4610-a3e2-624d4e815934";  // Формы регистров бухгалтерии
constexpr auto GUID_Section_Forms_CalculationRegisters          = "a2cb086c-db98-43e4-a1a9-0760ab048f8d";  // Формы регистров расчета
constexpr auto GUID_Section_Forms_BusinessProcesses             = "3f7a8120-b71a-4265-98bf-4d9bc09b7719";  // Формы бизнес-процессов
constexpr auto GUID_Section_Forms_Tasks                         = "3f58cbfb-4172-4e54-be49-561a579bb38b";  // Формы задач

//constexpr auto GUID_Section_Attributes      = "3daea016-69b7-4ed4-9453-127911372fe6";
//constexpr auto GUID_Section_TabularSections = "932159f9-95b2-4e76-a8dd-8849fe5c5ded";
// (дополняйте по мере обнаружения других секций)

/// Справочник GUID-ов секций → человекочитаемое имя.
const QHash<QString, QString>& sectionTypes();

} // namespace v8

#endif // SECTIONTYPES_H