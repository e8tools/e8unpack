#!/usr/bin/env bash
# Проверка работы с кириллицей в именах каталогов и файлов.
# Запуск из корня репозитория:  bash test/run-cyrillic.sh <путь-к-бинарнику>

UNPACK="${1:-build/v8unpack}"

BASE="Проверка кириллицы"
IN="$BASE/Исходные данные"
TMP="$BASE/файл контейнера.tmp"
OUT="$BASE/Распаковано"
TMP2="$BASE/собранный файл.tmp"
OUT2="$BASE/Распаковано повторно"

# контрольная площадка с именами ASCII: отделяет проблемы кириллицы от общих
# проблем окружения (недоступные утилиты, права, отсутствующий каталог)
CTRL="cyr-control"
CTRL_IN="$CTRL/in"
CTRL_TMP="$CTRL/container.tmp"
CTRL_OUT="$CTRL/out"

LOG="cyr-step-detail.log"
rm -rf "$BASE" "$CTRL" cyr-diff.log "$LOG"
mkdir -p "$IN/Вложенный каталог" "$CTRL_IN"

dd if=/dev/urandom of="$IN/файл данных.bin" bs=1 count=4096 >/dev/null 2>&1
dd if=/dev/urandom of="$IN/Вложенный каталог/ещё один файл.bin" bs=1 count=2048 >/dev/null 2>&1
cp "$IN/файл данных.bin" "$CTRL_IN/file.bin"

show_failure() {
	echo "Failed: $1"
	echo "--- вывод инструмента ---"
	cat "$LOG" 2>/dev/null
	echo "--- каталоги глазами операционной системы ---"
	ls -b "$BASE" 2>&1
	echo "--- кодировка вывода bash ---"
	printf 'LC_ALL=%s LANG=%s\n' "${LC_ALL:-нет}" "${LANG:-нет}"
	exit 1
}

echo 'контроль: то же самое с именами ASCII...'
if ! "$UNPACK" -build "$CTRL_IN" "$CTRL_TMP" >"$LOG" 2>&1; then
	show_failure "не удалось собрать контейнер из $CTRL_IN (имена ASCII)"
fi
mkdir -p "$CTRL_OUT"
if ! "$UNPACK" -parse "$CTRL_TMP" "$CTRL_OUT" >"$LOG" 2>&1; then
	show_failure "не удалось разобрать $CTRL_TMP (имена ASCII)"
fi
if ! diff -r "$CTRL_IN" "$CTRL_OUT" >cyr-diff.log 2>&1; then
	show_failure "содержимое каталогов с именами ASCII различается"
fi
echo Passed

echo 'сборка из каталога с кириллицей...'
if ! "$UNPACK" -build "$IN" "$TMP" >"$LOG" 2>&1; then
	show_failure "не удалось собрать контейнер из $IN"
fi

mkdir -p "$OUT"
echo 'разбор контейнера в каталог с кириллицей...'
if ! "$UNPACK" -parse "$TMP" "$OUT" >"$LOG" 2>&1; then
	show_failure "не удалось разобрать $TMP в $OUT"
fi

if ! diff -r "$IN" "$OUT" >cyr-diff.log 2>&1; then
	echo "Failed: содержимое каталогов различается"
	cat cyr-diff.log
	exit 1
fi
echo Passed

echo 'двойной проход: из кириллического каталога и обратно...'
if ! "$UNPACK" -build "$OUT" "$TMP2" >"$LOG" 2>&1; then
	show_failure "не удалось собрать контейнер из $OUT"
fi
mkdir -p "$OUT2"
if ! "$UNPACK" -parse "$TMP2" "$OUT2" >"$LOG" 2>&1; then
	show_failure "не удалось разобрать $TMP2 в $OUT2"
fi
if ! diff -r "$IN" "$OUT2" >cyr-diff.log 2>&1; then
	echo "Failed: после повторного прохода содержимое различается"
	cat cyr-diff.log
	exit 1
fi
echo Passed

rm -rf "$BASE" "$CTRL" cyr-diff.log "$LOG"
