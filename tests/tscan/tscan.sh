#!/bin/bash

cd "$(dirname "$0")"

EXE=TscanCodeV2.14.24.linux/TscanCodeV2.14.2395.linux/tscancode

if [ ! -e $EXE ] ; then
	if [ ! -e TscanCodeV2.14.24.linux.zip ] ; then
		wget https://github.com/Tencent/TscanCode/raw/master/release/linux/TscanCodeV2.14.24.linux.zip
	fi
	unzip TscanCodeV2.14.24.linux.zip
	chmod 755 $EXE
fi

$EXE -UDMALLOC -I../../lib/include/ -I../../lib/include/intern/ ../../lib/src/*.c
