#!/bin/bash

make clang_fuzz

mkdir CORPUS
cp input/* CORPUS

./main.fuzz CORPUS/ -jobs=20
