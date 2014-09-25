/* Copyright (c) 2014 Quanta Research Cambridge, Inc
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
#include "llvm/ADT/STLExtras.h"
#include "llvm/Assembly/Parser.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Constants.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm;

#include "declarations.h"

int trace_translate;// = 1;
int trace_full;// = 1;
static int dump_interpret;// = 1;
static int output_stdout;// = 1;

SLOTARRAY_TYPE slotarray[MAX_SLOTARRAY];
ExecutionEngine *EE;
const char *globalName;
OPERAND_ITEM_TYPE operand_list[MAX_OPERAND_LIST];
int operand_list_index;

static int slotarray_index = 1;
static Module *Mod;
static std::map<const Value *, int> slotmap;
static FILE *outputFile;
static int already_printed_header;
std::list<VTABLE_WORK> vtablework;
static NamedMDNode *CU_Nodes;

/*
 * Remove alloca and calls to 'llvm.dbg.declare()' that were added
 * when compiling with '-g'
 */
static bool opt_runOnBasicBlock(BasicBlock &BB)
{
    bool changed = false;
    BasicBlock::iterator Start = BB.getFirstInsertionPt();
    BasicBlock::iterator E = BB.end();
    if (Start == E) return false;
    BasicBlock::iterator I = Start++;
    while(1) {
        BasicBlock::iterator PI = llvm::next(BasicBlock::iterator(I));
        int opcode = I->getOpcode();
        Value *retv = (Value *)I;
        //printf("[%s:%d] OP %d %p;", __FUNCTION__, __LINE__, opcode, retv);
        //for (unsigned i = 0;  i < I->getNumOperands(); ++i) {
            //printf(" %p", I->getOperand(i));
        //}
        //printf("\n");
        switch (opcode) {
        case Instruction::Load:
            {
            const char *cp = I->getOperand(0)->getName().str().c_str();
            if (!strcmp(cp, "this")) {
                retv->replaceAllUsesWith(I->getOperand(0));
                I->eraseFromParent(); // delete "Load 'this'" instruction
                changed = true;
            }
            }
            break;
        case Instruction::Alloca:
            if (I->hasName()) {
                Value *newt = NULL;
                BasicBlock::iterator PN = PI;
                while (PN != E) {
                    BasicBlock::iterator PNN = llvm::next(BasicBlock::iterator(PN));
                    if (PN->getOpcode() == Instruction::Store && retv == PN->getOperand(1))
                        newt = PN->getOperand(0);
                    for (User::op_iterator OI = PN->op_begin(), OE = PN->op_end(); OI != OE; ++OI) {
                        if (*OI == retv && newt)
                            *OI = newt;
                    }
                    if (PN->getOpcode() == Instruction::Store && PN->getOperand(0) == PN->getOperand(1)) {
                        if (PI == PN)
                            PI = PNN;
                        PN->eraseFromParent(); // delete Store instruction
                    }
                    PN = PNN;
                }
                I->eraseFromParent(); // delete Alloca instruction
                changed = true;
            }
            break;
        case Instruction::Call:
            if (const CallInst *CI = dyn_cast<CallInst>(I)) {
              const Value *Operand = CI->getCalledValue();
                if (Operand->hasName() && isa<Constant>(Operand)) {
                  const char *cp = Operand->getName().str().c_str();
                  if (!strcmp(cp, "llvm.dbg.declare") /*|| !strcmp(cp, "printf")*/) {
                      I->eraseFromParent(); // delete this instruction
                      changed = true;
                  }
                }
            }
            break;
        };
        if (PI == E)
            break;
        I = PI;
    }
    return changed;
}

/*
 * Common utilities for processing Instruction lists
 */
int getLocalSlot(const Value *V)
{
    std::map<const Value *, int>::iterator FI = slotmap.find(V);
    if (FI == slotmap.end()) {
       slotmap.insert(std::pair<const Value *, int>(V, slotarray_index++));
       return getLocalSlot(V);
    }
    return (int)FI->second;
}
void prepareOperand(const Value *Operand)
{
    if (!Operand)
        return;
    int slotindex = getLocalSlot(Operand);
    operand_list[operand_list_index].value = slotindex;
    if (Operand->hasName()) {
        if (isa<Constant>(Operand)) {
            operand_list[operand_list_index].type = OpTypeExternalFunction;
            slotarray[slotindex].name = Operand->getName().str().c_str();
            goto retlab;
        }
    }
    else {
        const Constant *CV = dyn_cast<Constant>(Operand);
        if (CV && !isa<GlobalValue>(CV)) {
            if (const ConstantInt *CI = dyn_cast<ConstantInt>(CV)) {
                operand_list[operand_list_index].type = OpTypeInt;
                slotarray[slotindex].offset = CI->getZExtValue();
                goto retlab;
            }
            printf("[%s:%d] non integer constant\n", __FUNCTION__, __LINE__);
        }
        if (dyn_cast<MDNode>(Operand) || dyn_cast<MDString>(Operand)
         || Operand->getValueID() == Value::PseudoSourceValueVal ||
            Operand->getValueID() == Value::FixedStackPseudoSourceValueVal) {
            printf("[%s:%d] MDNode/MDString/Pseudo\n", __FUNCTION__, __LINE__);
        }
    }
    operand_list[operand_list_index].type = OpTypeLocalRef;
retlab:
    operand_list_index++;
}

const char *getparam(int arg)
{
   char temp[MAX_CHAR_BUFFER];
   temp[0] = 0;
   if (operand_list[arg].type == OpTypeLocalRef)
       return slotarray[operand_list[arg].value].name;
   else if (operand_list[arg].type == OpTypeExternalFunction)
       return slotarray[operand_list[arg].value].name;
   else if (operand_list[arg].type == OpTypeInt)
       sprintf(temp, "%lld", (long long)slotarray[operand_list[arg].value].offset);
   else if (operand_list[arg].type == OpTypeString)
       return (const char *)operand_list[arg].value;
   return strdup(temp);
}

/*
 * GEP and Load instructions interpreter functions
 * (just execute using the memory areas allocated by the constructors)
 */
static uint64_t getOperandValue(const Value *Operand)
{
    const Constant *CV = dyn_cast<Constant>(Operand);
    const ConstantInt *CI;

    if (CV && !isa<GlobalValue>(CV) && (CI = dyn_cast<ConstantInt>(CV)))
        return CI->getZExtValue();
    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    exit(1);
}
static uint64_t executeGEPOperation(gep_type_iterator I, gep_type_iterator E)
{
    const DataLayout *TD = EE->getDataLayout();
    uint64_t Total = 0;
    for (; I != E; ++I) {
        if (StructType *STy = dyn_cast<StructType>(*I)) {
            const StructLayout *SLO = TD->getStructLayout(STy);
            const ConstantInt *CPU = cast<ConstantInt>(I.getOperand());
            Total += SLO->getElementOffset(CPU->getZExtValue());
        } else {
            SequentialType *ST = cast<SequentialType>(*I);
            // Get the index number for the array... which must be long type...
            Total += TD->getTypeAllocSize(ST->getElementType())
                 * getOperandValue(I.getOperand());
        }
    }
    return Total;
}
static void LoadIntFromMemory(uint64_t *Dst, uint8_t *Src, unsigned LoadBytes)
{
    if (LoadBytes > sizeof(*Dst)) {
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        exit(1);
    }
    memcpy(Dst, Src, LoadBytes);
}
static uint64_t LoadValueFromMemory(PointerTy Ptr, Type *Ty)
{
    const DataLayout *TD = EE->getDataLayout();
    const unsigned LoadBytes = TD->getTypeStoreSize(Ty);
    uint64_t rv = 0;

    if (trace_full)
        printf("[%s:%d] bytes %d type %x\n", __FUNCTION__, __LINE__, LoadBytes, Ty->getTypeID());
    switch (Ty->getTypeID()) {
    case Type::IntegerTyID:
        LoadIntFromMemory(&rv, (uint8_t*)Ptr, LoadBytes);
        break;
    case Type::PointerTyID:
        if (!Ptr) {
            printf("[%s:%d] %p\n", __FUNCTION__, __LINE__, Ptr);
            exit(1);
        }
        else
            rv = (uint64_t) *((PointerTy*)Ptr);
        break;
    default:
        errs() << "Cannot load value of type " << *Ty << "!";
        exit(1);
    }
    if (trace_full)
        printf("[%s:%d] rv %llx\n", __FUNCTION__, __LINE__, (long long)rv);
    return rv;
}

/*
 * Walk all BasicBlocks for a Function, calling requested processing function
 */
static void processFunction(VTABLE_WORK *work, const char *guardName, const char *(*proc)(Function ***thisp, Instruction &I))
{
    Function *F = work->thisp[0][work->f];
    slotmap.clear();
    slotarray_index = 1;
    memset(slotarray, 0, sizeof(slotarray));
    /* Do generic optimization of instruction list (remove debug calls, remove automatic variables */
    for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I)
        opt_runOnBasicBlock(*I);
    if (trace_translate) {
        printf("FULL_AFTER_OPT: %s\n", F->getName().str().c_str());
        F->dump();
        printf("TRANSLATE:\n");
    }
    /* connect up argument formal param names with actual values */
    for (Function::const_arg_iterator AI = F->arg_begin(), AE = F->arg_end(); AI != AE; ++AI) {
        int slotindex = getLocalSlot(AI);
        if (AI->hasByValAttr()) {
            printf("[%s] hasByVal param not supported\n", __FUNCTION__);
            exit(1);
        }
        slotarray[slotindex].name = strdup(AI->getName().str().c_str());
        if (trace_full)
            printf("%s: [%d] '%s'\n", __FUNCTION__, slotindex, slotarray[slotindex].name);
        if (!strcmp(slotarray[slotindex].name, "this"))
            slotarray[slotindex].svalue = (uint8_t *)work->thisp;
        else if (!strcmp(slotarray[slotindex].name, "v")) {
            slotarray[slotindex] = work->arg;
        }
        else
            printf("%s: unknown parameter!! [%d] '%s'\n", __FUNCTION__, slotindex, slotarray[slotindex].name);
    }
    /* If this is an 'update' method, generate 'if guard' around instruction stream */
    already_printed_header = 0;
    /* Generate Verilog for all instructions.  Record function calls for post processing */
    for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I) {
        if (trace_translate && I->hasName())         // Print out the label if it exists...
            printf("LLLLL: %s\n", I->getName().str().c_str());
        for (BasicBlock::iterator ins = I->begin(), ins_end = I->end(); ins != ins_end;) {
            char instruction_label[MAX_CHAR_BUFFER];

            BasicBlock::iterator next_ins = llvm::next(BasicBlock::iterator(ins));
            operand_list_index = 0;
            memset(operand_list, 0, sizeof(operand_list));
            if (ins->hasName() || !ins->getType()->isVoidTy()) {
              int t = getLocalSlot(ins);
              operand_list[operand_list_index].type = OpTypeLocalRef;
              operand_list[operand_list_index++].value = t;
              if (ins->hasName())
                  slotarray[t].name = strdup(ins->getName().str().c_str());
              else {
                  char temp[MAX_CHAR_BUFFER];
                  sprintf(temp, "%%%d", t);
                  slotarray[t].name = strdup(temp);
              }
              sprintf(instruction_label, "%10s/%d: ", slotarray[t].name, t);
            }
            else {
              operand_list_index++;
              sprintf(instruction_label, "            : ");
            }
            if (trace_translate)
            printf("%s    XLAT:%14s", instruction_label, ins->getOpcodeName());
            for (unsigned i = 0, E = ins->getNumOperands(); i != E; ++i)
                prepareOperand(ins->getOperand(i));
            switch (ins->getOpcode()) {
            case Instruction::GetElementPtr:
                {
                uint64_t Total = executeGEPOperation(gep_type_begin(ins), gep_type_end(ins));
                if (!slotarray[operand_list[1].value].svalue) {
                    printf("[%s:%d] GEP pointer not valid\n", __FUNCTION__, __LINE__);
                    break;
                    exit(1);
                }
                uint8_t *ptr = slotarray[operand_list[1].value].svalue + Total;
                slotarray[operand_list[0].value].name = strdup(map_address(ptr, ""));
                slotarray[operand_list[0].value].svalue = ptr;
                slotarray[operand_list[0].value].offset = Total;
                }
                break;
            case Instruction::Load:
                {
                slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
                PointerTy Ptr = (PointerTy)slotarray[operand_list[1].value].svalue;
                if(!Ptr) {
                    printf("[%s:%d] arg not LocalRef;", __FUNCTION__, __LINE__);
                    if (!slotarray[operand_list[0].value].svalue)
                        operand_list[0].type = OpTypeInt;
                    break;
                }
                slotarray[operand_list[0].value].svalue = (uint8_t *)LoadValueFromMemory(Ptr, ins->getType());
                slotarray[operand_list[0].value].name = strdup(map_address(Ptr, ""));
                }
                break;
            default:
                {
                const char *vout = proc(work->thisp, *ins);
                if (vout) {
                    if (!already_printed_header) {
                        fprintf(outputFile, "\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n; %s\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n", globalName);
                        if (guardName)
                            fprintf(outputFile, "if (%s5guardEv && %s6enableEv) then begin\n", guardName, guardName);
                    }
                    already_printed_header = 1;
                    fprintf(outputFile, "        %s\n", vout);
                }
                }
                break;
            case Instruction::Alloca: // ignore
                memset(&slotarray[operand_list[0].value], 0, sizeof(slotarray[0]));
                break;
            }
            if (trace_translate)
                printf("\n");
            ins = next_ins;
        }
    }
}

static bool endswith(const char *str, const char *suffix)
{
    int skipl = strlen(str) - strlen(suffix);
    return skipl >= 0 && !strcmp(str + skipl, suffix);
}

/*
 * Symbolically run through all rules, running either preprocessing or
 * generating verilog.
 */
static void processConstructorAndRules(Module *Mod, Function ****modfirst,
       const char *(*proc)(Function ***thisp, Instruction &I))
{
    int generate = proc == generateVerilog;
    *modfirst = NULL;       // init the Module list before calling constructors
    // run Constructors
    EE->runStaticConstructorsDestructors(false);
    // Construct the address -> symbolic name map using dwarf debug info
    constructAddressMap(CU_Nodes);
    int ModuleRfirst= lookup_field("class.Module", "rfirst")/sizeof(uint64_t);
    int ModuleNext  = lookup_field("class.Module", "next")/sizeof(uint64_t);
    int RuleNext    = lookup_field("class.Rule", "next")/sizeof(uint64_t);
    Function ***modp = *modfirst;

    // Walk the rule lists for all modules, generating work items
    while (modp) {                   // loop through all modules
        printf("Module %p: rfirst %p next %p\n", modp, modp[ModuleRfirst], modp[ModuleNext]);
        Function ***rulep = (Function ***)modp[ModuleRfirst];        // Module.rfirst
        while (rulep) {                      // loop through all rules for module
            printf("Rule %p: next %p\n", rulep, rulep[RuleNext]);
            static std::string method[] = { "body", "guard", "update", ""};
            std::string *p = method;
            do {
                vtablework.push_back(VTABLE_WORK(lookup_method("class.Rule", *p),
                    rulep, SLOTARRAY_TYPE()));
            } while (*++p != "" && generate); // only preprocess 'body'
            rulep = (Function ***)rulep[RuleNext];           // Rule.next
        }
        modp = (Function ***)modp[ModuleNext]; // Module.next
    }

    // Walk list of work items, generating code
    while (vtablework.begin() != vtablework.end()) {
        Function *f = vtablework.begin()->thisp[0][vtablework.begin()->f];
        globalName = strdup(f->getName().str().c_str());
        VTABLE_WORK work = *vtablework.begin();
        const char *guardName = NULL;
        if (generate && endswith(globalName, "updateEv")) {
            char temp[MAX_CHAR_BUFFER];
            strcpy(temp, globalName);
            temp[strlen(globalName) - 9] = 0;  // truncate "updateEv"
            guardName = strdup(temp);
        }
        processFunction(&work, guardName, proc);
        if (guardName && already_printed_header)
            fprintf(outputFile, "end;\n");
        vtablework.pop_front();
    }
}

/*
 * Read/load llvm input files
 */
static Module *llvm_ParseIRFile(const std::string &Filename, SMDiagnostic &Err, LLVMContext &Context)
{
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

int main(int argc, char **argv, char * const *envp)
{
    std::string ErrorMsg;
    SMDiagnostic Err;

printf("[%s:%d] start\n", __FUNCTION__, __LINE__);
    DebugFlag = dump_interpret != 0;
    outputFile = fopen("output.tmp", "w");
    if (output_stdout)
        outputFile = stdout;

    cl::ParseCommandLineOptions(argc, argv, "llvm interpreter & dynamic compiler\n");

    LLVMContext &Context = getGlobalContext();

    // Load/link the input bitcode
    Mod = llvm_ParseIRFile(InputFile[0], Err, Context);
    if (!Mod) {
        errs() << argv[0] << ": error loading file '" << InputFile[0] << "'\n";
        return 1;
    }
    Linker L(Mod);
    for (unsigned i = 1; i < InputFile.size(); ++i) {
        Module *M = llvm_ParseIRFile(InputFile[i], Err, Context);
        if (!M || L.linkInModule(M, &ErrorMsg)) {
            errs() << argv[0] << ": link error in '" << InputFile[i] << "': " << ErrorMsg << "\n";
            return 1;
        }
    }

    //ModulePass *DebugIRPass = createDebugIRPass();
    //DebugIRPass->runOnModule(*Mod);

    // preprocessing before running anything
    CU_Nodes = Mod->getNamedMetadata("llvm.dbg.cu");
    if (CU_Nodes)
        process_metadata(CU_Nodes);
    adjustModuleSizes(Mod);

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
        printf("%s: unknown error creating EE!\n", argv[0]);
        exit(1);
    }

    Function **** modfirst = (Function ****)EE->getPointerToGlobal(Mod->getNamedValue("_ZN6Module5firstE"));
    Function *EntryFn = Mod->getFunction("main");
    if (!EntryFn || !modfirst) {
        printf("'main' function not found in module.\n");
        return 1;
    }

    // Preprocess the body rules, creating shadow variables and moving items to guard() and update()
    processConstructorAndRules(Mod, modfirst, calculateGuardUpdate);

    // Process the static constructors, generating code for all rules
    processConstructorAndRules(Mod, modfirst, generateVerilog);

printf("[%s:%d] now run main program\n", __FUNCTION__, __LINE__);
    // Run main
    std::vector<std::string> InputArgv;
    InputArgv.push_back("param1");
    InputArgv.push_back("param2");
    int Result = EE->runFunctionAsMain(EntryFn, InputArgv, envp);

    //dump_class_data();

printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
    return Result;
}
