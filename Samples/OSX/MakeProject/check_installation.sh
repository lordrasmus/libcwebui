#!/usr/bin/bash

which brew > /dev/null

if [ ! 0 -eq $? ] ; then
	ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi

echo "#include <event2/event.h>" > /tmp/test.c
clang -c -o /tmp/test.out /tmp/test.c > /dev/null

if [ ! 0 -eq $? ] ; then
	brew install libevent
fi
