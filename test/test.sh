#!/bin/bash
set -x
clang++ -g -fno-limit-debug-info -femit-all-decls -emit-llvm -S -o test.ll test.cpp

