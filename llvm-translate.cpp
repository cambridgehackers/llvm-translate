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
#include <list>
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

int trace_translate;// = 1;
int trace_full;// = 1;
static int dump_interpret;// = 1;
static int output_stdout;// = 1;
static FILE *outputFile;

ExecutionEngine *EE;
const char *globalName;
static Function *EntryFn;

char GeneratePass::ID = 0;
char RemoveAllocaPass::ID = 0;

static bool endswith(const char *str, const char *suffix)
{
    int skipl = strlen(str) - strlen(suffix);
    return skipl >= 0 && !strcmp(str + skipl, suffix);
}

/*
 * Remove alloca and calls to 'llvm.dbg.declare()' that were added
 * when compiling with '-g'
 */
bool RemoveAllocaPass::runOnFunction(Function &F)
{
    bool changed = false;
    for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
        Function::iterator BN = llvm::next(Function::iterator(BB));
    BasicBlock::iterator Start = BB->getFirstInsertionPt();
    BasicBlock::iterator E = BB->end();
    if (Start == E) return false;
    BasicBlock::iterator I = Start++;
    while(1) {
        BasicBlock::iterator PI = llvm::next(BasicBlock::iterator(I));
        int opcode = I->getOpcode();
        Value *retv = (Value *)I;
        switch (opcode) {
        case Instruction::Alloca:
            if (I->hasName() && endswith(I->getName().str().c_str(), ".addr")) {
                Value *newt = NULL;
                BasicBlock::iterator PN = PI;
                while (PN != E) {
                    BasicBlock::iterator PNN = llvm::next(BasicBlock::iterator(PN));
                    if (PN->getOpcode() == Instruction::Store && retv == PN->getOperand(1)) {
                        newt = PN->getOperand(0); // Remember value we were storing in temp
                        if (PI == PN)
                            PI = PNN;
                        PN->eraseFromParent(); // delete Store instruction
                    }
                    else if (PN->getOpcode() == Instruction::Load && retv == PN->getOperand(0)) {
                        PN->replaceAllUsesWith(newt); // replace with stored value
                        PN->eraseFromParent(); // delete Load instruction
                    }
                    PN = PNN;
                }
                I->eraseFromParent(); // delete Alloca instruction
                changed = true;
            }
            break;
        case Instruction::Call:
            if (CallInst *CI = dyn_cast<CallInst>(I)) {
                Value *Operand = CI->getCalledValue();
                if (Operand->hasName() && isa<Constant>(Operand)) {
                  const char *cp = Operand->getName().str().c_str();
                  if (!strcmp(cp, "llvm.dbg.declare") || !strcmp(cp, "atexit")) {
                      I->eraseFromParent(); // delete this instruction
                      break;
                      changed = true;
                  }
                }
                Instruction *nexti = PI;
                if (BB == F.begin() && BN == BE && I == BB->getFirstInsertionPt() && nexti == BB->getTerminator()) {
                    printf("[%s:%d] single!!!! %s\n", __FUNCTION__, __LINE__, F.getName().str().c_str());
                    InlineFunctionInfo IFI;
                    InlineFunction(CI, IFI, false);
                }
            }
            break;
        };
        if (PI == E)
            break;
        I = PI;
    }
    }
    return changed;
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

namespace {
    cl::list<std::string> InputFile(cl::Positional, cl::OneOrMore, cl::desc("<input bitcode>"));
    cl::opt<std::string> MArch("march", cl::desc("Architecture to generate assembly for (see --version)"));
    cl::list<std::string> MAttrs("mattr", cl::CommaSeparated, cl::desc("Target specific attributes (-mattr=help for details)"), cl::value_desc("a1,+a2,-a3,..."));
}

bool GeneratePass::runOnModule(Module &M)
{
    std::string ErrorMsg;
    Module *Mod = &M;

printf("[%s:%d] start\n", __FUNCTION__, __LINE__);

    // preprocessing before running anything
    NamedMDNode *CU_Nodes = Mod->getNamedMetadata("llvm.dbg.cu");
    if (CU_Nodes)
        process_metadata(CU_Nodes);

    EngineBuilder builder(Mod);
    builder.setMArch(MArch);
    builder.setMCPU("");
    builder.setMAttrs(MAttrs);
    builder.setErrorStr(&ErrorMsg);
    builder.setEngineKind(EngineKind::Interpreter);
    builder.setOptLevel(CodeGenOpt::None);

    // Create the execution environment and allocate memory for static items
    EE = builder.create();
    if (!EE) {
        printf("llvm-translate: unknown error creating EE!\n");
        exit(1);
    }

    Function **** modfirst = (Function ****)EE->getPointerToGlobal(Mod->getNamedValue("_ZN6Module5firstE"));
    EntryFn = Mod->getFunction("main");
    if (!EntryFn || !modfirst) {
        printf("'main' function not found in module.\n");
        exit(1);
    }

    // Preprocess the body rules, creating shadow variables and moving items to guard() and update()
    processConstructorAndRules(Mod, modfirst, CU_Nodes, calculateGuardUpdate, 0, outputFile);

    // Process the static constructors, generating code for all rules
    processConstructorAndRules(Mod, modfirst, CU_Nodes, generateVerilog, 1, outputFile);

printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
    return false;
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

    raw_fd_ostream Outc_fd("foo.tmp.xc", ErrorMsg);
    raw_fd_ostream Outch_fd("foo.tmp.h", ErrorMsg);

    PassManager Passes;
    Passes.add(new RemoveAllocaPass());
    Passes.add(new CallProcessPass());
    Passes.add(new GeneratePass());
    Passes.add(new CWriter(Outc_fd, Outch_fd));
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
    //raw_fd_ostream Out("foo.tmp.bc", ErrorMsg, sys::fs::F_Binary);
    //WriteBitcodeToFile(Mod, Out);
printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
    return 0;
}
