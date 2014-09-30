
LEVEL := .
TOOLNAME := llvm-translate

LINK_COMPONENTS := jit instrumentation interpreter bitreader asmparser irreader selectiondag native linker bitwriter

PROJECT_NAME := llvm-translate
PROJ_VERSION := 0.1

LLVM_SRC_ROOT = ../llvm
LLVM_OBJ_ROOT = ../llvm/build
PROJ_SRC_ROOT := $(PWD)
PROJ_OBJ_ROOT := $(PWD)
PROJ_INSTALL_ROOT := /usr/local

include $(LLVM_OBJ_ROOT)/Makefile.config
include $(LLVM_SRC_ROOT)/Makefile.rules

