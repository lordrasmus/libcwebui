#!/bin/bash

cd "$(dirname "$0")"


VERSION=0.15.0

if [ ! -e infer-linux64-v$VERSION.tar.xz ] ; then
	wget "https://github.com/facebook/infer/releases/download/v$VERSION/infer-linux64-v$VERSION.tar.xz"
fi

if [ ! -e infer-linux64-v$VERSION/bin/infer ] ; then
	tar -xavf infer-linux64-v$VERSION.tar.xz 
fi

./infer-linux64-v$VERSION/bin/infer run -- make -C ../../Samples/Linux/SingleMain/
