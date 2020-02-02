#!/bin/bash

# https://github.com/google/fuzzing/blob/master/tutorial/libFuzzerTutorial.md

make clang_fuzz

mkdir -p CORPUS
cp input/* CORPUS

./main.fuzz CORPUS/ -jobs=$(nproc) -workers=$(nproc)
