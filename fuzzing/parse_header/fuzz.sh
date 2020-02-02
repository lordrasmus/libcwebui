#!/bin/bash

make clang_fuzz

mkdir -p CORPUS
cp input/* CORPUS

./main.fuzz CORPUS/ -jobs=$(nproc) -workers=$(nproc)
