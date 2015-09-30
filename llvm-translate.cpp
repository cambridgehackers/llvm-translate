/* Copyright (c) 2015 The Connectal Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
// Portions of this program were derived from source with the license:
//     This file is distributed under the University of Illinois Open Source
//     License. See LICENSE.TXT for details.
#include <stdio.h>
#include "llvm/Linker.h"
#include "llvm/PassManager.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Assembly/Parser.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Constants.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

#include "declarations.h"

cl::list<std::string> InputFile(cl::Positional, cl::OneOrMore, cl::desc("<input bitcode>"));
cl::opt<std::string> MArch("march", cl::desc("Architecture to generate assembly for (see --version)"));
cl::list<std::string> MAttrs("mattr", cl::CommaSeparated, cl::desc("Target specific attributes (-mattr=help for details)"), cl::value_desc("a1,+a2,-a3,..."));

int trace_translate;// = 1;
int trace_full;// = 1;
static int dump_interpret;// = 1;
static int output_stdout;// = 1;
FILE *outputFile;
ExecutionEngine *EE;

bool endswith(const char *str, const char *suffix)
{
    int skipl = strlen(str) - strlen(suffix);
    return skipl >= 0 && !strcmp(str + skipl, suffix);
}

/*
 * Read/load llvm input files
 */
static Module *llvm_ParseIRFile(const std::string &Filename, LLVMContext &Context)
{
    SMDiagnostic Err;
    OwningPtr<MemoryBuffer> File;
    Module *M = new Module(Filename, Context);
    M->addModuleFlag(llvm::Module::Error, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);
    if (MemoryBuffer::getFileOrSTDIN(Filename, File)
     || !ParseAssembly(File.take(), M, Err, Context))
        return 0;
    return M;
}

int main(int argc, char **argv, char * const *envp)
{
    std::string ErrorMsg;
    unsigned i = 0;

printf("[%s:%d] start\n", __FUNCTION__, __LINE__);
    //DebugFlag = dump_interpret != 0;
    outputFile = fopen("output.tmp", "w");
    if (output_stdout)
        outputFile = stdout;

    cl::ParseCommandLineOptions(argc, argv, "llvm interpreter & dynamic compiler\n");

    LLVMContext &Context = getGlobalContext();

    // Load/link the input bitcode
    Module *Mod = llvm_ParseIRFile(InputFile[0], Context);
    if (!Mod) {
        printf("llvm-translate: load/link error in %s\n", InputFile[i].c_str());
        return 1;
    }
    Linker L(Mod);
    for (i = 1; i < InputFile.size(); ++i) {
        Module *M = llvm_ParseIRFile(InputFile[i], Context);
        if (!M || L.linkInModule(M, &ErrorMsg)) {
            printf("llvm-translate: load/link error in %s\n", InputFile[i].c_str());
            return 1;
        }
    }

    FILE *Outc_fd = fopen("foo.tmp.xc", "w");
    FILE *Outch_fd = fopen("foo.tmp.h", "w");

    PassManager Passes;
    Passes.add(new RemoveAllocaPass());
    Passes.add(new CallProcessPass());
    Passes.add(new GeneratePass(Outc_fd, Outch_fd));
    Passes.run(*Mod);
    //ModulePass *DebugIRPass = createDebugIRPass();
    //DebugIRPass->runOnModule(*Mod);

printf("[%s:%d] now run main program\n", __FUNCTION__, __LINE__);
    DebugFlag = dump_interpret != 0;
    // Run main
    std::vector<std::string> InputArgv;
    InputArgv.push_back("param1");
    InputArgv.push_back("param2");
    //char *envp[] = {NULL};
    //int Result = EE->runFunctionAsMain(EntryFn, InputArgv, envp);
//printf("[%s:%d] %d\n", __FUNCTION__, __LINE__, Result);

    //dump_class_data();

    // write copy of optimized bitcode
    //raw_fd_ostream OutBit("foo.tmp.bc", ErrorMsg, sys::fs::F_Binary);
    //WriteBitcodeToFile(Mod, OutBit);
printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
    return 0;
}
