#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Сбор несистемных разделяемых библиотек рядом с бинарником (Linux).

Замыкание обходится рекурсивно по `ldd`: начиная с самого файла, каждая
несистемная библиотека копируется в <dest> и опрашивается снова. Библиотеки
glibc и динамический линкер не копируются — они есть в любой системе.

Если нужная библиотека не найдена, скрипт завершается с ошибкой: неполный архив
должен ломать сборку, а не доезжать до пользователя.

Режимы:
    --exe <файл> --dest <каталог>   собрать библиотеки в каталог
    --check --dir <каталог>         проверить, что в каталоге зависимость полная
                                    (каталог с бинарником и подкаталогом lib/)
"""
import argparse
import os
import shutil
import subprocess
import sys

# Библиотеки, которые присутствуют в любой системе: glibc и линкер.
SYSTEM = {
    "linux-vdso.so.1",
    "linux-gate.so.1",
    "ld-linux.so.2",
    "ld-linux-x86-64.so.2",
    "libc.so.6",
    "libm.so.6",
    "libpthread.so.0",
    "libdl.so.2",
    "librt.so.1",
    "libresolv.so.2",
    "libutil.so.1",
    "libnsl.so.1",
    "libanl.so.1",
}


def is_elf(path):
    try:
        with open(path, "rb") as f:
            return f.read(4) == b"\x7fELF"
    except OSError:
        return False


def ldd_deps(path):
    """Список (имя, путь|None) — прямые зависимости файла по данным ldd."""
    try:
        proc = subprocess.run(["ldd", path], capture_output=True, text=True)
    except OSError:
        return []
    if proc.returncode != 0:
        return []

    deps = []
    for raw in proc.stdout.splitlines():
        line = raw.strip()
        if not line:
            continue
        # «statically linked», «not a dynamic executable» — зависимостей нет.
        if "=>" not in line and "/" not in line:
            continue
        if "=>" in line:
            name, rest = line.split("=>", 1)
            name, rest = name.strip(), rest.strip()
            if rest.startswith("not found"):
                deps.append((name, None))
            else:
                deps.append((name, rest.split(" (")[0].strip()))
        else:
            # Строка вида "/lib64/ld-linux-x86-64.so.2 (0x...)" — линкер.
            name = line.split(" (")[0].strip()
            deps.append((os.path.basename(name), name))
    return deps


def is_system(name):
    return os.path.basename(name) in SYSTEM


def collect(exe, dest):
    """Рекурсивно скопировать несистемные зависимости в dest."""
    if not is_elf(exe):
        print("bundle-linux-libs: %s не ELF-файл" % exe, file=sys.stderr)
        return [], []

    os.makedirs(dest, exist_ok=True)
    queue = [os.path.abspath(exe)]
    seen = set()
    copied = []
    missing = []

    while queue:
        current = queue.pop()
        key = os.path.basename(current)
        if key in seen:
            continue
        seen.add(key)

        for name, path in ldd_deps(current):
            if is_system(name):
                continue
            if path is None:
                missing.append((os.path.basename(current), name))
                continue
            target = os.path.join(dest, os.path.basename(path))
            if not os.path.exists(target):
                shutil.copy2(os.path.realpath(path), target)
                copied.append(os.path.basename(target))
            queue.append(target)

    return sorted(copied), missing


def check(root):
    """Проверить, что все несистемные зависимости разрешаются внутри каталога."""
    libs = os.path.join(root, "lib")
    if not os.path.isdir(libs):
        print("bundle-linux-libs: нет каталога %s" % libs, file=sys.stderr)
        return False

    env = dict(os.environ)
    env["LD_LIBRARY_PATH"] = libs
    bad = []
    files = []
    for dirpath, _dirnames, filenames in os.walk(root):
        for name in filenames:
            path = os.path.join(dirpath, name)
            if is_elf(path):
                files.append(path)

    for path in files:
        try:
            proc = subprocess.run(["ldd", path], capture_output=True, text=True, env=env)
        except OSError:
            continue
        for raw in proc.stdout.splitlines():
            line = raw.strip()
            if "=>" not in line:
                continue
            name, rest = line.split("=>", 1)
            name, rest = name.strip(), rest.strip()
            if is_system(name):
                continue
            if rest.startswith("not found"):
                bad.append((path, name, "не найдена"))
                continue
            resolved = os.path.realpath(rest.split(" (")[0].strip())
            inside = os.path.realpath(root)
            if not (resolved == inside or resolved.startswith(inside + os.sep) or os.path.dirname(resolved) == os.path.realpath(libs)):
                bad.append((path, name, resolved))

    print("bundle-linux-libs: проверено ELF-файлов %d, внутри каталога %d библиотек"
          % (len(files), len(os.listdir(libs))))
    if bad:
        for path, name, where in bad:
            print("  %s: %s -> %s" % (os.path.basename(path), name, where), file=sys.stderr)
        return False
    return True


def main():
    ap = argparse.ArgumentParser(add_help=True)
    ap.add_argument("--exe")
    ap.add_argument("--dest")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--dir")
    args = ap.parse_args()

    if args.check:
        if not args.dir:
            ap.error("--check требует --dir")
        ok = check(args.dir)
        print("bundle-linux-libs: %s" % ("зависимость полная" if ok else "НЕПОЛНАЯ ЗАВИСИМОСТЬ"))
        return 0 if ok else 1

    if not args.exe or not args.dest:
        ap.error("нужны --exe и --dest")

    copied, missing = collect(args.exe, args.dest)
    print("bundle-linux-libs: скопировано %d библиотек:" % len(copied))
    for name in copied:
        print("  %s" % name)

    if missing:
        for who, name in missing:
            print("bundle-linux-libs: не найдена зависимость %s (нужна %s)" % (name, who),
                  file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
