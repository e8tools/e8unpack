# v8Unpack, GCC edition
Добавлен проект CodeBlocks 13.12.
Для сборки требуется Boost.

## Fork of v8Unpack project by Denis Demidov (disa_da2@mail.ru)

[Original project HOME](https://www.assembla.com/spaces/V8Unpack/team)

[Original project svn repo](http://svn2.assembla.com/svn/V8Unpack/)

## Note

V8Unpack - a small console program  for rebuild/build configuration files [1C](http://1c.ru) such as *.cf *.epf *.erf
 
## Plaform 

Windows, POSIX

## Environment

Project for [codelite IDE](http://www.codelite.org/)  
Project for [Codeblocks IDE](http://codeblocks.org/)  
CMake

## Build

[![Build status](https://ci.appveyor.com/api/projects/status/48ac3trblfjjkts7/branch/master?svg=true)](https://ci.appveyor.com/project/dmpas/v8unpack/branch/master)

[Прямая ссылка на последнюю успешную сборку win32](https://ci.appveyor.com/api/projects/dmpas/v8unpack/artifacts/Release%2Fv8unpack.exe?branch=master)

### Ubuntu/Debian

```
sudo apt-add-repository ppa:dmpas/e8
sudo apt-get update
sudo apt-get install v8unpack
```

### Fedora\Centos
```
cd /etc/yum.repos.d/
sudo wget http://download.opensuse.org/repositories/home:/pumbaEO/Fedora_22/home:pumbaEO.repo
sudo dnf install v8unpack
```

### Готовые сборки (GitHub Releases)

Сборки лежат в [releases](https://github.com/e8tools/e8unpack/releases).

Linux-архив (`v8unpack-<версия>-linux-x86_64.tar.gz`) самодостаточен: рядом с
бинарником в `lib/` лежат Qt6 и её зависимости (ICU, glib, pcre2, zstd, zlib,
double-conversion, libb2, libstdc++, libgcc). Устанавливать Qt6 не нужно —
достаточно распаковать архив и запустить `./v8unpack`. Сборка сделана в Debian 12
(glibc 2.36) — на более старых системах с glibc < 2.36 понадобится своя сборка.

В `.deb` те же библиотеки ставятся в `/usr/lib/v8unpack`, бинарник находит их
через `RPATH`.

### Chocolatey
```
choco install v8unpack -source https://www.myget.org/F/onescript -y
```

## Version 3.0

- Оптимизирована сборка .cf файла ключ -B[UILD]. В версии 2.0 сборка корневого контейнера происходила в оперативной памяти.
При сборке больших конфигураций это могло приводить к ошибке "segmentation fault". В версии 3.0 сборка корневого контейнера происходит 
динамически с сохранением элементов контейнера непосредственно в файл по мере их создания.


## Использование

```
  -U[NPACK]            in_filename.cf     out_dirname
  -U[NPACK]  -L[IST]   listfile
  -PA[CK]              in_dirname         out_filename.cf
  -PA[CK]    -L[IST]   listfile
  -I[NFLATE]           in_filename.data   out_filename
  -I[NFLATE] -L[IST]   listfile
  -D[EFLATE]           in_filename        filename.data
  -D[EFLATE] -L[IST]   listfile
  -P[ARSE]             in_filename        out_dirname
  -P[ARSE]   -L[IST]   listfile
  -B[UILD] [-N[OPACK]] in_dirname         out_filename
  -B[UILD] [-N[OPACK]] -L[IST] listfile
```
