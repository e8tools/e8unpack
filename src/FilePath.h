/*----------------------------------------------------------
This Source Code Form is subject to the terms of the
Mozilla Public License, v.2.0. If a copy of the MPL
was not distributed with this file, You can obtain one
at http://mozilla.org/MPL/2.0/.
----------------------------------------------------------*/

#ifndef V8UNPACK_FILE_PATH_H
#define V8UNPACK_FILE_PATH_H

#include <string>
#include <vector>
#include <boost/filesystem.hpp>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <shellapi.h>
#endif

// Все пути внутри программы хранятся строками в UTF-8. На Windows ни argv,
// ни boost::filesystem::path не работают с UTF-8 напрямую: узкая строка
// преобразуется по ANSI-кодировке процесса, поэтому кириллица в путях
// превращалась в мусор («файл не найден» при существующем каталоге).
// Здесь это преобразование выполняется явно: UTF-8 <-> UTF-16 <-> path.
// На остальных платформах строка и есть путь, поведение не меняется.

namespace v8unpack {

#ifdef _WIN32

/// UTF-8 -> UTF-16.
inline std::wstring to_wide(const std::string &utf8)
{
	if (utf8.empty()) {
		return std::wstring();
	}
	const int len = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
	if (len <= 0) {
		return std::wstring();
	}
	std::wstring wide(static_cast<size_t>(len), L'\0');
	::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), &wide[0], len);
	return wide;
}

/// UTF-16 -> UTF-8.
inline std::string from_wide(const std::wstring &wide)
{
	if (wide.empty()) {
		return std::string();
	}
	const int len = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
	if (len <= 0) {
		return std::string();
	}
	std::string utf8(static_cast<size_t>(len), '\0');
	::WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), &utf8[0], len, nullptr, nullptr);
	return utf8;
}

#endif // _WIN32

/// Строка UTF-8 -> путь файловой системы.
inline boost::filesystem::path to_path(const std::string &utf8)
{
#ifdef _WIN32
	return boost::filesystem::path(to_wide(utf8));
#else
	return boost::filesystem::path(utf8);
#endif
}

/// То же для строки в стиле C.
inline boost::filesystem::path to_path(const char *utf8)
{
	return to_path(utf8 != nullptr ? std::string(utf8) : std::string());
}

/// Путь файловой системы -> строка UTF-8.
inline std::string to_utf8(const boost::filesystem::path &path)
{
#ifdef _WIN32
	return from_wide(path.native());
#else
	return path.native();
#endif
}

/// Аргументы командной строки в UTF-8.
///
/// argv на Windows приходит в ANSI-кодировке процесса, поэтому кириллица в
/// пути теряется ещё до входа в main(). Берём настоящую командную строку в
/// UTF-16 и переводим её в UTF-8; при неудаче откатываемся на argv.
inline std::vector<std::string> command_line_args(int argc, char *argv[])
{
	std::vector<std::string> result;

#ifdef _WIN32
	int wide_argc = 0;
	LPWSTR *wide_argv = ::CommandLineToArgvW(::GetCommandLineW(), &wide_argc);
	if (wide_argv != nullptr) {
		for (int i = 0; i < wide_argc; i++) {
			result.push_back(from_wide(wide_argv[i]));
		}
		::LocalFree(wide_argv);
	}
	if (!result.empty()) {
		return result;
	}
#endif

	result.reserve(static_cast<size_t>(argc));
	for (int i = 0; i < argc; i++) {
		result.push_back(argv[i] != nullptr ? std::string(argv[i]) : std::string());
	}
	return result;
}

} // namespace v8unpack

#endif // V8UNPACK_FILE_PATH_H
