#!/usr/bin/bash

which brew

if [ ! 0 -eq $? ] ; then
	ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi
