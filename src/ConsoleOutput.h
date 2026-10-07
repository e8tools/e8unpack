#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

// ConsoleOutput.h — корректный вывод Unicode в консоль Windows
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.


#include <QString>
#include <QTextStream>
#include <QStringConverter>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace v8unpack {

namespace detail {

#ifdef Q_OS_WIN
// true — хендл действительно консоль (не файл/pipe)
inline bool isConsoleHandle(HANDLE h)
{
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return false;
    DWORD mode = 0;
    return GetConsoleMode(h, &mode) != 0;
}
#endif

inline void writeToFile(FILE* f, const QString& text)
{
    QTextStream s(f);
    s.setEncoding(QStringConverter::Utf8);
    s << text;
    s.flush();          // гарантированно сбрасываем буфер QTextStream
    std::fflush(f);     // и буфер C-runtime
}

} // namespace detail

inline void writeStdout(const QString& text)
{
#ifdef Q_OS_WIN
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (detail::isConsoleHandle(h)) {
        DWORD written = 0;
        const std::wstring w = text.toStdWString();
        if (WriteConsoleW(h, w.c_str(), static_cast<DWORD>(w.size()), &written, nullptr))
            return;
        // если по какой-то причине не удалось — падаем в UTF-8
    }
    detail::writeToFile(stdout, text);
#else
    detail::writeToFile(stdout, text);
#endif
}

inline void writeStderr(const QString& text)
{
#ifdef Q_OS_WIN
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if (detail::isConsoleHandle(h)) {
        DWORD written = 0;
        const std::wstring w = text.toStdWString();
        if (WriteConsoleW(h, w.c_str(), static_cast<DWORD>(w.size()), &written, nullptr))
            return;
    }
    detail::writeToFile(stderr, text);
#else
    detail::writeToFile(stderr, text);
#endif
}

} // namespace v8


#endif // CONSOLEOUTPUT_H
