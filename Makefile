
LEVEL := .
TOOLNAME := llijca

#LINK_COMPONENTS := mcjit jit instrumentation interpreter nativecodegen bitreader asmparser irreader selectiondag native
LINK_COMPONENTS := jit instrumentation interpreter bitreader asmparser irreader selectiondag native

PROJECT_NAME := jca
PROJ_VERSION := 0.9

LLVM_SRC_ROOT = /home/jca/s/git/llvm
LLVM_OBJ_ROOT = /home/jca/s/git/llvm/build
PROJ_SRC_ROOT := /home/jca/s/git/llvm/projects/jca
PROJ_OBJ_ROOT := /home/jca/s/git/llvm/build/projects/jca
PROJ_INSTALL_ROOT := /usr/local

include $(LLVM_OBJ_ROOT)/Makefile.config
include $(LLVM_SRC_ROOT)/Makefile.rules

