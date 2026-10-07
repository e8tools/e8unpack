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

rm -rf "$BASE" cyr-diff.log
mkdir -p "$IN/Вложенный каталог"

# данные с кириллицей и в каталогах, и в именах файлов
dd if=/dev/urandom of="$IN/файл данных.bin" bs=1 count=4096 >/dev/null 2>&1
dd if=/dev/urandom of="$IN/Вложенный каталог/ещё один файл.bin" bs=1 count=2048 >/dev/null 2>&1

echo 'сборка из каталога с кириллицей...'
if ! "$UNPACK" -build "$IN" "$TMP" >/dev/null 2>&1; then
	echo "Failed: не удалось собрать контейнер из $IN"
	exit 1
fi

mkdir -p "$OUT"
echo 'разбор контейнера в каталог с кириллицей...'
if ! "$UNPACK" -parse "$TMP" "$OUT" >/dev/null 2>&1; then
	echo "Failed: не удалось разобрать $TMP в $OUT"
	exit 1
fi

if ! diff -r "$IN" "$OUT" >cyr-diff.log 2>&1; then
	echo "Failed: содержимое каталогов различается"
	cat cyr-diff.log
	exit 1
fi
echo Passed

echo 'повторная сборка и разбор (двойной проход)...'
if ! "$UNPACK" -build "$OUT" "$TMP2" >/dev/null 2>&1; then
	echo "Failed: не удалось собрать контейнер из $OUT"
	exit 1
fi
mkdir -p "$OUT2"
if ! "$UNPACK" -parse "$TMP2" "$OUT2" >/dev/null 2>&1; then
	echo "Failed: не удалось разобрать $TMP2 в $OUT2"
	exit 1
fi
if ! diff -r "$IN" "$OUT2" >cyr-diff.log 2>&1; then
	echo "Failed: после повторного прохода содержимое различается"
	cat cyr-diff.log
	exit 1
fi
echo Passed

rm -rf "$BASE" cyr-diff.log
