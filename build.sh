#!/bin/bash

### Цвета консоли.

ECHO_ESCAPE=" -e "

TEXT_FG_BLACK="[30m"
TEXT_FG_RED="[31m"
TEXT_FG_GREEN="[32m"
TEXT_FG_ORANGE="[33m"
TEXT_FG_BLUE="[34m"
TEXT_FG_MAGENTA="[35m"
TEXT_FG_CYAN="[36m"
TEXT_FG_GRAY="[37m"
TEXT_FG_DARK_GRAY="[30;1m"
TEXT_FG_LIGHT_RED="[31;1m"
TEXT_FG_LIGHT_GREEN="[32;1m"
TEXT_FG_YELLOW="[33;1m"
TEXT_FG_VIOLET="[34;1m"
TEXT_FG_LIGHT_MAGENTA="[35;1m"
TEXT_FG_LIGHT_CYAN="[36;1m"
TEXT_FG_WHITE="[37;1m"

TEXT_BG_BLACK="[40m"
TEXT_BG_RED="[41m"
TEXT_BG_GREEN="[42m"
TEXT_BG_YELLOW="[43m"
TEXT_BG_BLUE="[44m"
TEXT_BG_MAGENTA="[45m"
TEXT_BG_CYAN="[46m"
TEXT_BG_GRAY="[47m"

TEXT_NORM="[0m" # Возврат к обычному тексту

TT_MSG_PREFIX="$TEXT_BG_GREEN$TEXT_FG_YELLOW esp> $TEXT_NORM"

_banner()
{
    echo
    echo $ECHO_ESCAPE$TEXT_BG_GREEN$TEXT_FG_WHITE"┌──────────"$TEXT_FG_WHITE"┐"$TEXT_NORM
    echo $ECHO_ESCAPE$TEXT_BG_GREEN$TEXT_FG_WHITE"│ "$TEXT_FG_LIGHT_RED"E"$TEXT_FG_YELLOW"spresso "$TEXT_FG_WHITE"│"$TEXT_NORM
    echo $ECHO_ESCAPE$TEXT_BG_GREEN$TEXT_FG_WHITE"└"$TEXT_FG_WHITE"──────────┘"$TEXT_NORM
    echo
}

_failed()
{
    echo
    echo $ECHO_ESCAPE$TT_MSG_PREFIX$TEXT_BG_RED$TEXT_FG_YELLOW" Failed :( "$TEXT_NORM
    echo
}

_completed()
{
    echo
    echo $ECHO_ESCAPE$TT_MSG_PREFIX$TEXT_BG_CYAN$TEXT_FG_WHITE" Completed successfully :) "$TEXT_NORM
    echo
}

# Выполняет команду; при ошибке сообщает о неудаче и завершает сценарий.
_ex()
{
    echo $@
    if ! $@
    then
        _failed
        exit 1
    fi
}

_banner

export TT_OS=`uname -s | tr '[:upper:]' '[:lower:]'`

# Режимы из аргументов: release, verbose, single. Они выводятся в строке
# BUILD MODE, но idf.py их не получает: это остаток сборки через cmake и
# make, закомментированной ниже.
echo $@ | grep -- 'release' > /dev/null 2>&1
if [ "$?" -eq "0" ]; then
    CMAKE_BUILD_TYPE=Release
    BUILD_PARAMS=$BUILD_PARAMS' release'
else
    CMAKE_BUILD_TYPE=Debug
    BUILD_PARAMS=$BUILD_PARAMS' debug'
fi

echo $@ | grep -- 'verbose' > /dev/null 2>&1
if [ "$?" -eq "0" ]; then
    CMAKE_VERBOSE_MAKEFILE=TRUE
    BUILD_PARAMS=$BUILD_PARAMS' verbose'
else
    CMAKE_VERBOSE_MAKEFILE=FALSE
    BUILD_PARAMS=$BUILD_PARAMS' silent'
fi

echo $@ | grep -- 'single' > /dev/null 2>&1
if [ "$?" -eq "0" ]; then
    TX_BUILD_THREADS=1
    BUILD_PARAMS=$BUILD_PARAMS' single'
else if [ "$TT_OS" == "darwin" ]; then
    TX_BUILD_THREADS=`/usr/sbin/system_profiler -detailLevel full SPHardwareDataType | awk '/Total Number of Cores/ {print $5};'`
else
    TX_BUILD_THREADS=`cat /proc/cpuinfo | grep ^processor | wc -l`
fi
    BUILD_PARAMS=$BUILD_PARAMS' multi-make'
fi

echo $ECHO_ESCAPE$TT_MSG_PREFIX$TEXT_BG_BLUE$TEXT_FG_WHITE" BUILD MODE:$BUILD_PARAMS "$TEXT_NORM
echo

# cd build
# _ex cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCMAKE_VERBOSE_MAKEFILE=$CMAKE_VERBOSE_MAKEFILE ..
# _ex make -j $TX_BUILD_THREADS

# _ex idf.py set-target esp32
# _ex idf.py menuconfig
# Окружение ESP-IDF должно быть загружено заранее: . <путь к ESP-IDF>/export.sh
_ex idf.py build
# _ex idf.py -p /dev/ttyUSB0 flash

_completed
