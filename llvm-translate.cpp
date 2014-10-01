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
#include "llvm/PassManager.h"
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
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"

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
static std::map<const Value *, int> slotmap;
static FILE *outputFile;
static int already_printed_header;
std::list<VTABLE_WORK> vtablework;

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
                //I->eraseFromParent(); // delete Alloca instruction
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
       NamedMDNode *CU_Nodes,
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

static char ID;
class Foo : public ModulePass {
public:
  bool runOnModule(Module &M);
  Foo() : ModulePass(ID) {}
};

bool Foo::runOnModule(Module &M)
{
    std::string ErrorMsg;
Module *Mod = &M;

printf("[%s:%d] start\n", __FUNCTION__, __LINE__);

    // preprocessing before running anything
    NamedMDNode *CU_Nodes = Mod->getNamedMetadata("llvm.dbg.cu");
    if (CU_Nodes)
        process_metadata(CU_Nodes);

    /* iterate through all functions, adjusting 'Call' operands */
    for (Module::iterator FI = Mod->begin(), FE = Mod->end(); FI != FE; ++FI)
        for (Function::iterator BI = FI->begin(), BE = FI->end(); BI != BE; ++BI)
            for (BasicBlock::iterator II = BI->begin(), IE = BI->end(); II != IE; ) {
                BasicBlock::iterator PI = llvm::next(BasicBlock::iterator(II));
                if (II->getOpcode() == Instruction::Call)
                    callProcess_runOnInstruction(Mod, II);
                II = PI;
            }

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
    Function *EntryFn = Mod->getFunction("main");
    if (!EntryFn || !modfirst) {
        printf("'main' function not found in module.\n");
        exit(1);
    }

    // Preprocess the body rules, creating shadow variables and moving items to guard() and update()
    processConstructorAndRules(Mod, modfirst, CU_Nodes, calculateGuardUpdate);

    // Process the static constructors, generating code for all rules
    processConstructorAndRules(Mod, modfirst, CU_Nodes, generateVerilog);

printf("[%s:%d] now run main program\n", __FUNCTION__, __LINE__);
    DebugFlag = dump_interpret != 0;
    // Run main
    std::vector<std::string> InputArgv;
    InputArgv.push_back("param1");
    InputArgv.push_back("param2");
    char *envp[] = {NULL};
    int Result = EE->runFunctionAsMain(EntryFn, InputArgv, envp);
printf("[%s:%d] %d\n", __FUNCTION__, __LINE__, Result);

    //dump_class_data();

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
    PassManager Passes;

    Passes.add(new Foo());

    //ModulePass *DebugIRPass = createDebugIRPass();
    //DebugIRPass->runOnModule(*Mod);
    Passes.run(*Mod);

    // write copy of optimized bitcode
    raw_fd_ostream Out("foo.tmp.bc", ErrorMsg, sys::fs::F_Binary);
    WriteBitcodeToFile(Mod, Out);
printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
    return 0;
}
#if 1
#include <set>
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/InstVisitor.h"
#include "llvm/Support/InstIterator.h"
  class CWriter : public FunctionPass, public InstVisitor<CWriter> {
    formatted_raw_ostream &Out;
    std::map<const ConstantFP *, unsigned> FPConstantMap;
    std::set<Function*> intrinsicPrototypesAlreadyGenerated;
    std::set<const Argument*> ByValParams;
    unsigned FPCounter;
    unsigned OpaqueCounter;
    DenseMap<const Value*, unsigned> AnonValueNumbers;
    unsigned NextAnonValueNumber;
    DenseMap<StructType*, unsigned> UnnamedStructIDs;
  public:
    static char ID;
    explicit CWriter(formatted_raw_ostream &o)
      : FunctionPass(ID), Out(o), OpaqueCounter(0), NextAnonValueNumber(0) {
      FPCounter = 0;
    }
    virtual const char *getPassName() const { return "C backend"; }
    virtual bool doInitialization(Module &M);
    bool runOnFunction(Function &F) {
     if (F.hasAvailableExternallyLinkage())
       return false;
      lowerIntrinsics(F);
      printFunction(F);
      return false;
    }
    virtual bool doFinalization(Module &M) {
      //delete TAsm;
      FPConstantMap.clear();
      ByValParams.clear();
      intrinsicPrototypesAlreadyGenerated.clear();
      UnnamedStructIDs.clear();
      return false;
    }
    raw_ostream &printType(raw_ostream &Out, Type *Ty, bool isSigned = false, const std::string &VariableName = "", bool IgnoreName = false, const AttributeSet &PAL = AttributeSet());
    raw_ostream &printSimpleType(raw_ostream &Out, Type *Ty, bool isSigned, const std::string &NameSoFar = "");
    void printStructReturnPointerFunctionType(raw_ostream &Out, const AttributeSet &PAL, PointerType *Ty);
    std::string getStructName(StructType *ST);
    void writeOperandDeref(Value *Operand) {
      if (isAddressExposed(Operand)) {
        writeOperandInternal(Operand);
      } else {
        Out << "*(";
        writeOperand(Operand);
        Out << ")";
      }
    }
    void writeOperand(Value *Operand, bool Static = false);
    void writeInstComputationInline(Instruction &I);
    void writeOperandInternal(Value *Operand, bool Static = false);
    void writeOperandWithCast(Value* Operand, unsigned Opcode);
    void writeOperandWithCast(Value* Operand, const ICmpInst &I);
    bool writeInstructionCast(const Instruction &I);
    void writeMemoryAccess(Value *Operand, Type *OperandType, bool IsVolatile, unsigned Alignment);
  private :
    void lowerIntrinsics(Function &F);
    void printIntrinsicDefinition(const Function &F, raw_ostream &Out);
    void printModuleTypes();
    void printContainedStructs(Type *Ty, SmallPtrSet<Type *, 16> &);
    void printFunctionSignature(const Function *F, bool Prototype);
    void printFunction(Function &);
    void printBasicBlock(BasicBlock *BB);
    void printCast(unsigned opcode, Type *SrcTy, Type *DstTy);
    void printConstant(Constant *CPV, bool Static);
    void printConstantWithCast(Constant *CPV, unsigned Opcode);
    bool printConstExprCast(const ConstantExpr *CE, bool Static);
    void printConstantArray(ConstantArray *CPA, bool Static);
    void printConstantVector(ConstantVector *CV, bool Static);
    bool isAddressExposed(const Value *V) const {
      if (const Argument *A = dyn_cast<Argument>(V))
        return ByValParams.count(A);
      return isa<GlobalVariable>(V) || isDirectAlloca(V);
    }
    static bool isInlinableInst(const Instruction &I) {
      if (isa<CmpInst>(I))
        return true;
      if (I.getType() == Type::getVoidTy(I.getContext()) || !I.hasOneUse() ||
          isa<TerminatorInst>(I) || isa<CallInst>(I) || isa<PHINode>(I) ||
          isa<LoadInst>(I) || isa<VAArgInst>(I) || isa<InsertElementInst>(I) ||
          isa<InsertValueInst>(I))
        return false;
      if (I.hasOneUse()) {
        const Instruction &User = cast<Instruction>(*I.use_back());
        if (isInlineAsm(User) || isa<ExtractElementInst>(User) ||
            isa<ShuffleVectorInst>(User))
          return false;
      }
      return I.getParent() == cast<Instruction>(I.use_back())->getParent();
    }
    static const AllocaInst *isDirectAlloca(const Value *V) {
      const AllocaInst *AI = dyn_cast<AllocaInst>(V);
      if (!AI) return 0;
      if (AI->isArrayAllocation())
        return 0;   // FIXME: we can also inline fixed size array allocas!
      if (AI->getParent() != &AI->getParent()->getParent()->getEntryBlock())
        return 0;
      return AI;
    }
    static bool isInlineAsm(const Instruction& I) {
      if (const CallInst *CI = dyn_cast<CallInst>(&I))
        return isa<InlineAsm>(CI->getCalledValue());
      return false;
    }
    friend class InstVisitor<CWriter>;
    void visitReturnInst(ReturnInst &I);
    void visitBranchInst(BranchInst &I);
    void visitSwitchInst(SwitchInst &I);
    void visitIndirectBrInst(IndirectBrInst &I);
    void visitInvokeInst(InvokeInst &I) {
      llvm_unreachable("Lowerinvoke pass didn't work!");
    }
    void visitResumeInst(ResumeInst &I) {
      llvm_unreachable("DwarfEHPrepare pass didn't work!");
    }
    void visitUnreachableInst(UnreachableInst &I);
    void visitPHINode(PHINode &I);
    void visitBinaryOperator(Instruction &I);
    void visitICmpInst(ICmpInst &I);
    void visitFCmpInst(FCmpInst &I);
    void visitCastInst (CastInst &I);
    void visitSelectInst(SelectInst &I);
    void visitCallInst (CallInst &I);
    void visitInlineAsm(CallInst &I);
    bool visitBuiltinCall(CallInst &I, Intrinsic::ID ID, bool &WroteCallee);
    void visitAllocaInst(AllocaInst &I);
    void visitLoadInst  (LoadInst   &I);
    void visitStoreInst (StoreInst  &I);
    void visitGetElementPtrInst(GetElementPtrInst &I);
    void visitVAArgInst (VAArgInst &I);
    void visitInsertElementInst(InsertElementInst &I);
    void visitExtractElementInst(ExtractElementInst &I);
    void visitShuffleVectorInst(ShuffleVectorInst &SVI);
    void visitInsertValueInst(InsertValueInst &I);
    void visitExtractValueInst(ExtractValueInst &I);
    void visitInstruction(Instruction &I) {
      errs() << "C Writer does not know about " << I;
      llvm_unreachable(0);
    }
    void outputLValue(Instruction *I) {
      Out << "  " << GetValueName(I) << " = ";
    }
    bool isGotoCodeNecessary(BasicBlock *From, BasicBlock *To);
    void printPHICopiesForSuccessor(BasicBlock *CurBlock, BasicBlock *Successor, unsigned Indent);
    void printBranchToBlock(BasicBlock *CurBlock, BasicBlock *SuccBlock, unsigned Indent);
    void printGEPExpression(Value *Ptr, gep_type_iterator I, gep_type_iterator E, bool Static);
    std::string GetValueName(const Value *Operand);
  };
char CWriter::ID = 0;
static std::string CBEMangle(const std::string &S) {
  std::string Result;
  for (unsigned i = 0, e = S.size(); i != e; ++i)
    if (isalnum(S[i]) || S[i] == '_') {
      Result += S[i];
    } else {
      Result += '_';
      Result += 'A'+(S[i]&15);
      Result += 'A'+((S[i]>>4)&15);
      Result += '_';
    }
  return Result;
}
std::string CWriter::getStructName(StructType *ST) {
  if (!ST->isLiteral() && !ST->getName().empty())
    return CBEMangle("l_"+ST->getName().str());
  return "l_unnamed_" + utostr(UnnamedStructIDs[ST]);
}
void CWriter::printStructReturnPointerFunctionType(raw_ostream &Out, const AttributeSet &PAL, PointerType *TheTy) {
  FunctionType *FTy = cast<FunctionType>(TheTy->getElementType());
  std::string tstr;
  raw_string_ostream FunctionInnards(tstr);
  FunctionInnards << " (*) (";
  bool PrintedType = false;
  FunctionType::param_iterator I = FTy->param_begin(), E = FTy->param_end();
  Type *RetTy = cast<PointerType>(*I)->getElementType();
  unsigned Idx = 1;
  for (++I, ++Idx; I != E; ++I, ++Idx) {
    if (PrintedType)
      FunctionInnards << ", ";
    Type *ArgTy = *I;
    //if (PAL.paramHasAttr(Idx, Attribute::ByVal)) {
      //assert(ArgTy->isPointerTy());
      //ArgTy = cast<PointerType>(ArgTy)->getElementType();
    //}
    printType(FunctionInnards, ArgTy,
        /*isSigned=*/false/*PAL.paramHasAttr(Idx, Attribute::SExt)*/, "");
    PrintedType = true;
  }
  if (FTy->isVarArg()) {
    if (!PrintedType)
      FunctionInnards << " int"; //dummy argument for empty vararg functs
    FunctionInnards << ", ...";
  } else if (!PrintedType) {
    FunctionInnards << "void";
  }
  FunctionInnards << ')';
  printType(Out, RetTy,
      /*isSigned=*/false/*PAL.paramHasAttr(0, Attribute::SExt)*/, FunctionInnards.str());
}
raw_ostream &
CWriter::printSimpleType(raw_ostream &Out, Type *Ty, bool isSigned,
                         const std::string &NameSoFar) {
  assert((Ty->isPrimitiveType() || Ty->isIntegerTy() || Ty->isVectorTy()) &&
         "Invalid type for printSimpleType");
  switch (Ty->getTypeID()) {
  case Type::VoidTyID:   return Out << "void " << NameSoFar;
  case Type::IntegerTyID: {
    unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
    if (NumBits == 1)
      return Out << "bool " << NameSoFar;
    else if (NumBits <= 8)
      return Out << (isSigned?"signed":"unsigned") << " char " << NameSoFar;
    else if (NumBits <= 16)
      return Out << (isSigned?"signed":"unsigned") << " short " << NameSoFar;
    else if (NumBits <= 32)
      return Out << (isSigned?"signed":"unsigned") << " int " << NameSoFar;
    else if (NumBits <= 64)
      return Out << (isSigned?"signed":"unsigned") << " long long "<< NameSoFar;
    else {
      assert(NumBits <= 128 && "Bit widths > 128 not implemented yet");
      return Out << (isSigned?"llvmInt128":"llvmUInt128") << " " << NameSoFar;
    }
  }
  case Type::FloatTyID:  return Out << "float "   << NameSoFar;
  case Type::DoubleTyID: return Out << "double "  << NameSoFar;
  case Type::X86_FP80TyID:
  case Type::PPC_FP128TyID:
  case Type::FP128TyID:  return Out << "long double " << NameSoFar;
  case Type::X86_MMXTyID:
    return printSimpleType(Out, Type::getInt32Ty(Ty->getContext()), isSigned,
                     " __attribute__((vector_size(64))) " + NameSoFar);
  case Type::VectorTyID: {
    VectorType *VTy = cast<VectorType>(Ty);
    return printSimpleType(Out, VTy->getElementType(), isSigned,
                     " __attribute__((vector_size(" 
                     "utostr(TD->getTypeAllocSize(VTy))" " ))) " + NameSoFar);
  }
  default:
    errs() << "Unknown primitive type: " << *Ty << "\n";
    llvm_unreachable(0);
  }
}
raw_ostream &CWriter::printType(raw_ostream &Out, Type *Ty,
                                bool isSigned, const std::string &NameSoFar,
                                bool IgnoreName, const AttributeSet &PAL) {
  if (Ty->isPrimitiveType() || Ty->isIntegerTy() || Ty->isVectorTy()) {
    printSimpleType(Out, Ty, isSigned, NameSoFar);
    return Out;
  }
  switch (Ty->getTypeID()) {
  case Type::FunctionTyID: {
    FunctionType *FTy = cast<FunctionType>(Ty);
    std::string tstr;
    raw_string_ostream FunctionInnards(tstr);
    FunctionInnards << " (" << NameSoFar << ") (";
    unsigned Idx = 1;
    for (FunctionType::param_iterator I = FTy->param_begin(),
           E = FTy->param_end(); I != E; ++I) {
      Type *ArgTy = *I;
      //if (PAL.paramHasAttr(Idx, Attribute::ByVal)) {
        //assert(ArgTy->isPointerTy());
        //ArgTy = cast<PointerType>(ArgTy)->getElementType();
      //}
      if (I != FTy->param_begin())
        FunctionInnards << ", ";
      printType(FunctionInnards, ArgTy,
        /*isSigned=*/false/*PAL.paramHasAttr(Idx, Attribute::SExt)*/, "");
      ++Idx;
    }
    if (FTy->isVarArg()) {
      if (!FTy->getNumParams())
        FunctionInnards << " int"; //dummy argument for empty vaarg functs
      FunctionInnards << ", ...";
    } else if (!FTy->getNumParams()) {
      FunctionInnards << "void";
    }
    FunctionInnards << ')';
    printType(Out, FTy->getReturnType(),
      /*isSigned=*/false/*PAL.paramHasAttr(0, Attribute::SExt)*/, FunctionInnards.str());
    return Out;
  }
  case Type::StructTyID: {
    StructType *STy = cast<StructType>(Ty);
    if (!IgnoreName)
      return Out << getStructName(STy) << ' ' << NameSoFar;
    Out << NameSoFar + " {\n";
    unsigned Idx = 0;
    for (StructType::element_iterator I = STy->element_begin(),
           E = STy->element_end(); I != E; ++I) {
      Out << "  ";
      printType(Out, *I, false, "field" + utostr(Idx++));
      Out << ";\n";
    }
    Out << '}';
    if (STy->isPacked())
      Out << " __attribute__ ((packed))";
    return Out;
  }
  case Type::PointerTyID: {
    PointerType *PTy = cast<PointerType>(Ty);
    std::string ptrName = "*" + NameSoFar;
    if (PTy->getElementType()->isArrayTy() ||
        PTy->getElementType()->isVectorTy())
      ptrName = "(" + ptrName + ")";
    if (!PAL.isEmpty())
      return printType(Out, PTy->getElementType(), false, ptrName, true, PAL);
    return printType(Out, PTy->getElementType(), false, ptrName);
  }
  case Type::ArrayTyID: {
    ArrayType *ATy = cast<ArrayType>(Ty);
    unsigned NumElements = ATy->getNumElements();
    if (NumElements == 0) NumElements = 1;
    Out << NameSoFar << " { ";
    printType(Out, ATy->getElementType(), false,
              "array[" + utostr(NumElements) + "]");
    return Out << "; }";
  }
  default:
    llvm_unreachable("Unhandled case in getTypeProps!");
  }
  return Out;
}
void CWriter::printConstantArray(ConstantArray *CPA, bool Static) {
  Type *ETy = CPA->getType()->getElementType();
  bool isString = (ETy == Type::getInt8Ty(CPA->getContext()) ||
                   ETy == Type::getInt8Ty(CPA->getContext()));
  if (isString && (CPA->getNumOperands() == 0 ||
                   !cast<Constant>(*(CPA->op_end()-1))->isNullValue()))
    isString = false;
  if (isString) {
    Out << '\"';
    bool LastWasHex = false;
    for (unsigned i = 0, e = CPA->getNumOperands()-1; i != e; ++i) {
      unsigned char C = cast<ConstantInt>(CPA->getOperand(i))->getZExtValue();
      if (isprint(C) && (!LastWasHex || !isxdigit(C))) {
        LastWasHex = false;
        if (C == '"' || C == '\\')
          Out << "\\" << (char)C;
        else
          Out << (char)C;
      } else {
        LastWasHex = false;
        switch (C) {
        case '\n': Out << "\\n"; break;
        case '\t': Out << "\\t"; break;
        case '\r': Out << "\\r"; break;
        case '\v': Out << "\\v"; break;
        case '\a': Out << "\\a"; break;
        case '\"': Out << "\\\""; break;
        case '\'': Out << "\\\'"; break;
        default:
          Out << "\\x";
          Out << (char)(( C/16  < 10) ? ( C/16 +'0') : ( C/16 -10+'A'));
          Out << (char)(((C&15) < 10) ? ((C&15)+'0') : ((C&15)-10+'A'));
          LastWasHex = true;
          break;
        }
      }
    }
    Out << '\"';
  } else {
    Out << '{';
    if (CPA->getNumOperands()) {
      Out << ' ';
      printConstant(cast<Constant>(CPA->getOperand(0)), Static);
      for (unsigned i = 1, e = CPA->getNumOperands(); i != e; ++i) {
        Out << ", ";
        printConstant(cast<Constant>(CPA->getOperand(i)), Static);
      }
    }
    Out << " }";
  }
}
void CWriter::printConstantVector(ConstantVector *CP, bool Static) {
  Out << '{';
  if (CP->getNumOperands()) {
    Out << ' ';
    printConstant(cast<Constant>(CP->getOperand(0)), Static);
    for (unsigned i = 1, e = CP->getNumOperands(); i != e; ++i) {
      Out << ", ";
      printConstant(cast<Constant>(CP->getOperand(i)), Static);
    }
  }
  Out << " }";
}
void CWriter::printCast(unsigned opc, Type *SrcTy, Type *DstTy) {
  switch (opc) {
    case Instruction::UIToFP: case Instruction::SIToFP: case Instruction::IntToPtr:
    case Instruction::Trunc: case Instruction::BitCast: case Instruction::FPExt:
    case Instruction::FPTrunc: // For these the DstTy sign doesn't matter
      Out << '(';
      printType(Out, DstTy);
      Out << ')';
      break;
    case Instruction::ZExt: case Instruction::PtrToInt:
    case Instruction::FPToUI: // For these, make sure we get an unsigned dest
      Out << '(';
      printSimpleType(Out, DstTy, false);
      Out << ')';
      break;
    case Instruction::SExt:
    case Instruction::FPToSI: // For these, make sure we get a signed dest
      Out << '(';
      printSimpleType(Out, DstTy, true);
      Out << ')';
      break;
    default:
      llvm_unreachable("Invalid cast opcode");
  }
  switch (opc) {
    case Instruction::UIToFP: case Instruction::ZExt:
      Out << '(';
      printSimpleType(Out, SrcTy, false);
      Out << ')';
      break;
    case Instruction::SIToFP: case Instruction::SExt:
      Out << '(';
      printSimpleType(Out, SrcTy, true);
      Out << ')';
      break;
    case Instruction::IntToPtr: case Instruction::PtrToInt:
      Out << "(unsigned long)";
      break;
    case Instruction::Trunc: case Instruction::BitCast: case Instruction::FPExt:
    case Instruction::FPTrunc: case Instruction::FPToSI: case Instruction::FPToUI:
      break; // These don't need a source cast.
    default:
      llvm_unreachable("Invalid cast opcode");
      break;
  }
}
void CWriter::printConstant(Constant *CPV, bool Static) {
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(CPV)) {
    switch (CE->getOpcode()) {
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::SExt:
    case Instruction::FPTrunc: case Instruction::FPExt: case Instruction::UIToFP:
    case Instruction::SIToFP: case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::PtrToInt: case Instruction::IntToPtr: case Instruction::BitCast:
      Out << "(";
      printCast(CE->getOpcode(), CE->getOperand(0)->getType(), CE->getType());
      if (CE->getOpcode() == Instruction::SExt &&
          CE->getOperand(0)->getType() == Type::getInt1Ty(CPV->getContext())) {
        Out << "0-";
      }
      printConstant(CE->getOperand(0), Static);
      if (CE->getType() == Type::getInt1Ty(CPV->getContext()) &&
          (CE->getOpcode() == Instruction::Trunc ||
           CE->getOpcode() == Instruction::FPToUI ||
           CE->getOpcode() == Instruction::FPToSI ||
           CE->getOpcode() == Instruction::PtrToInt)) {
        Out << "&1u";
      }
      Out << ')';
      return;
    case Instruction::GetElementPtr:
      Out << "(";
      printGEPExpression(CE->getOperand(0), gep_type_begin(CPV),
                         gep_type_end(CPV), Static);
      Out << ")";
      return;
    case Instruction::Select:
      Out << '(';
      printConstant(CE->getOperand(0), Static);
      Out << '?';
      printConstant(CE->getOperand(1), Static);
      Out << ':';
      printConstant(CE->getOperand(2), Static);
      Out << ')';
      return;
    case Instruction::Add: case Instruction::FAdd: case Instruction::Sub:
    case Instruction::FSub: case Instruction::Mul: case Instruction::FMul:
    case Instruction::SDiv: case Instruction::UDiv: case Instruction::FDiv:
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
    case Instruction::And: case Instruction::Or: case Instruction::Xor:
    case Instruction::ICmp: case Instruction::Shl: case Instruction::LShr:
    case Instruction::AShr:
    {
      Out << '(';
      bool NeedsClosingParens = printConstExprCast(CE, Static);
      printConstantWithCast(CE->getOperand(0), CE->getOpcode());
      switch (CE->getOpcode()) {
      case Instruction::Add: case Instruction::FAdd: Out << " + "; break;
      case Instruction::Sub: case Instruction::FSub: Out << " - "; break;
      case Instruction::Mul: case Instruction::FMul: Out << " * "; break;
      case Instruction::URem: case Instruction::SRem: case Instruction::FRem: Out << " % "; break;
      case Instruction::UDiv: case Instruction::SDiv: case Instruction::FDiv: Out << " / "; break;
      case Instruction::And: Out << " & "; break;
      case Instruction::Or:  Out << " | "; break;
      case Instruction::Xor: Out << " ^ "; break;
      case Instruction::Shl: Out << " << "; break;
      case Instruction::LShr: case Instruction::AShr: Out << " >> "; break;
      case Instruction::ICmp:
        switch (CE->getPredicate()) {
          case ICmpInst::ICMP_EQ: Out << " == "; break;
          case ICmpInst::ICMP_NE: Out << " != "; break;
          case ICmpInst::ICMP_SLT: case ICmpInst::ICMP_ULT: Out << " < "; break;
          case ICmpInst::ICMP_SLE: case ICmpInst::ICMP_ULE: Out << " <= "; break;
          case ICmpInst::ICMP_SGT: case ICmpInst::ICMP_UGT: Out << " > "; break;
          case ICmpInst::ICMP_SGE: case ICmpInst::ICMP_UGE: Out << " >= "; break;
          default: llvm_unreachable("Illegal ICmp predicate");
        }
        break;
      default: llvm_unreachable("Illegal opcode here!");
      }
      printConstantWithCast(CE->getOperand(1), CE->getOpcode());
      if (NeedsClosingParens)
        Out << "))";
      Out << ')';
      return;
    }
    case Instruction::FCmp: {
      Out << '(';
      bool NeedsClosingParens = printConstExprCast(CE, Static);
      if (CE->getPredicate() == FCmpInst::FCMP_FALSE)
        Out << "0";
      else if (CE->getPredicate() == FCmpInst::FCMP_TRUE)
        Out << "1";
      else {
        const char* op = 0;
        switch (CE->getPredicate()) {
        default: llvm_unreachable("Illegal FCmp predicate");
        case FCmpInst::FCMP_ORD: op = "ord"; break;
        case FCmpInst::FCMP_UNO: op = "uno"; break;
        case FCmpInst::FCMP_UEQ: op = "ueq"; break;
        case FCmpInst::FCMP_UNE: op = "une"; break;
        case FCmpInst::FCMP_ULT: op = "ult"; break;
        case FCmpInst::FCMP_ULE: op = "ule"; break;
        case FCmpInst::FCMP_UGT: op = "ugt"; break;
        case FCmpInst::FCMP_UGE: op = "uge"; break;
        case FCmpInst::FCMP_OEQ: op = "oeq"; break;
        case FCmpInst::FCMP_ONE: op = "one"; break;
        case FCmpInst::FCMP_OLT: op = "olt"; break;
        case FCmpInst::FCMP_OLE: op = "ole"; break;
        case FCmpInst::FCMP_OGT: op = "ogt"; break;
        case FCmpInst::FCMP_OGE: op = "oge"; break;
        }
        Out << "llvm_fcmp_" << op << "(";
        printConstantWithCast(CE->getOperand(0), CE->getOpcode());
        Out << ", ";
        printConstantWithCast(CE->getOperand(1), CE->getOpcode());
        Out << ")";
      }
      if (NeedsClosingParens)
        Out << "))";
      Out << ')';
      return;
    }
    default:
      errs() << "CWriter Error: Unhandled constant expression: " << *CE << "\n";
      llvm_unreachable(0);
    }
  } else if (isa<UndefValue>(CPV) && CPV->getType()->isSingleValueType()) {
    Out << "((";
    printType(Out, CPV->getType()); // sign doesn't matter
    Out << ")/*UNDEF*/";
    if (!CPV->getType()->isVectorTy()) {
      Out << "0)";
    } else {
      Out << "{})";
    }
    return;
  }
  if (ConstantInt *CI = dyn_cast<ConstantInt>(CPV)) {
    Type* Ty = CI->getType();
    if (Ty == Type::getInt1Ty(CPV->getContext()))
      Out << (CI->getZExtValue() ? '1' : '0');
    else if (Ty == Type::getInt32Ty(CPV->getContext()))
      Out << CI->getZExtValue() << 'u';
    else if (Ty->getPrimitiveSizeInBits() > 32)
      Out << CI->getZExtValue() << "ull";
    else {
      Out << "((";
      printSimpleType(Out, Ty, false) << ')';
      if (CI->isMinValue(true))
        Out << CI->getZExtValue() << 'u';
      else
        Out << CI->getSExtValue();
      Out << ')';
    }
    return;
  }
  switch (CPV->getType()->getTypeID()) {
  case Type::FloatTyID: case Type::DoubleTyID: case Type::X86_FP80TyID:
  case Type::PPC_FP128TyID: case Type::FP128TyID: {
    ConstantFP *FPC = cast<ConstantFP>(CPV);
    std::map<const ConstantFP*, unsigned>::iterator I = FPConstantMap.find(FPC);
    if (I != FPConstantMap.end()) {
      Out << "(*(" << (FPC->getType() == Type::getFloatTy(CPV->getContext()) ?
                       "float" :
                       FPC->getType() == Type::getDoubleTy(CPV->getContext()) ?
                       "double" :
                       "long double")
          << "*)&FPConstant" << I->second << ')';
    } else {
      double V;
      if (FPC->getType() == Type::getFloatTy(CPV->getContext()))
        V = FPC->getValueAPF().convertToFloat();
      else if (FPC->getType() == Type::getDoubleTy(CPV->getContext()))
        V = FPC->getValueAPF().convertToDouble();
      else {
        APFloat Tmp = FPC->getValueAPF();
        bool LosesInfo;
        Tmp.convert(APFloat::IEEEdouble, APFloat::rmTowardZero, &LosesInfo);
        V = Tmp.convertToDouble();
      }
      if (IsNAN(V)) {
        const unsigned long QuietNaN = 0x7ff8UL;
        char Buffer[100];
        uint64_t ll = DoubleToBits(V);
        sprintf(Buffer, "0x%llx", static_cast<long long>(ll));
        std::string Num(&Buffer[0], &Buffer[6]);
        unsigned long Val = strtoul(Num.c_str(), 0, 16);
        if (FPC->getType() == Type::getFloatTy(FPC->getContext()))
          Out << "LLVM_NAN" << (Val == QuietNaN ? "" : "S") << "F(\""
              << Buffer << "\") /*nan*/ ";
        else
          Out << "LLVM_NAN" << (Val == QuietNaN ? "" : "S") << "(\""
              << Buffer << "\") /*nan*/ ";
      } else if (IsInf(V)) {
        if (V < 0) Out << '-';
        Out << "LLVM_INF" <<
            (FPC->getType() == Type::getFloatTy(FPC->getContext()) ? "F" : "")
            << " /*inf*/ ";
      } else {
        std::string Num;
Num = "BOZONUMBER";
       Out << Num;
      }
    }
    break;
  }
  case Type::ArrayTyID:
    if (!Static) {
      Out << "(";
      printType(Out, CPV->getType());
      Out << ")";
    }
    Out << "{ "; // Arrays are wrapped in struct types.
    if (ConstantArray *CA = dyn_cast<ConstantArray>(CPV)) {
      printConstantArray(CA, Static);
    } else {
      assert(isa<ConstantAggregateZero>(CPV) || isa<UndefValue>(CPV));
      ArrayType *AT = cast<ArrayType>(CPV->getType());
      Out << '{';
      if (AT->getNumElements()) {
        Out << ' ';
        Constant *CZ = Constant::getNullValue(AT->getElementType());
        printConstant(CZ, Static);
        for (unsigned i = 1, e = AT->getNumElements(); i != e; ++i) {
          Out << ", ";
          printConstant(CZ, Static);
        }
      }
      Out << " }";
    }
    Out << " }"; // Arrays are wrapped in struct types.
    break;
  case Type::VectorTyID:
    if (!Static) {
      Out << "(";
      printType(Out, CPV->getType());
      Out << ")";
    }
    if (ConstantVector *CV = dyn_cast<ConstantVector>(CPV)) {
      printConstantVector(CV, Static);
    } else {
      assert(isa<ConstantAggregateZero>(CPV) || isa<UndefValue>(CPV));
      VectorType *VT = cast<VectorType>(CPV->getType());
      Out << "{ ";
      Constant *CZ = Constant::getNullValue(VT->getElementType());
      printConstant(CZ, Static);
      for (unsigned i = 1, e = VT->getNumElements(); i != e; ++i) {
        Out << ", ";
        printConstant(CZ, Static);
      }
      Out << " }";
    }
    break;
  case Type::StructTyID:
    if (!Static) {
      Out << "(";
      printType(Out, CPV->getType());
      Out << ")";
    }
    if (isa<ConstantAggregateZero>(CPV) || isa<UndefValue>(CPV)) {
      StructType *ST = cast<StructType>(CPV->getType());
      Out << '{';
      if (ST->getNumElements()) {
        Out << ' ';
        printConstant(Constant::getNullValue(ST->getElementType(0)), Static);
        for (unsigned i = 1, e = ST->getNumElements(); i != e; ++i) {
          Out << ", ";
          printConstant(Constant::getNullValue(ST->getElementType(i)), Static);
        }
      }
      Out << " }";
    } else {
      Out << '{';
      if (CPV->getNumOperands()) {
        Out << ' ';
        printConstant(cast<Constant>(CPV->getOperand(0)), Static);
        for (unsigned i = 1, e = CPV->getNumOperands(); i != e; ++i) {
          Out << ", ";
          printConstant(cast<Constant>(CPV->getOperand(i)), Static);
        }
      }
      Out << " }";
    }
    break;
  case Type::PointerTyID:
    if (isa<ConstantPointerNull>(CPV)) {
      Out << "((";
      printType(Out, CPV->getType()); // sign doesn't matter
      Out << ")/*NULL*/0)";
      break;
    } else if (GlobalValue *GV = dyn_cast<GlobalValue>(CPV)) {
      writeOperand(GV, Static);
      break;
    }
  default:
    errs() << "Unknown constant type: " << *CPV << "\n";
    llvm_unreachable(0);
  }
}
bool CWriter::printConstExprCast(const ConstantExpr* CE, bool Static) {
  bool NeedsExplicitCast = false;
  Type *Ty = CE->getOperand(0)->getType();
  bool TypeIsSigned = false;
  switch (CE->getOpcode()) {
  case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
  case Instruction::LShr: case Instruction::URem:
  case Instruction::UDiv: NeedsExplicitCast = true; break;
  case Instruction::AShr: case Instruction::SRem:
  case Instruction::SDiv: NeedsExplicitCast = true; TypeIsSigned = true; break;
  case Instruction::SExt:
    Ty = CE->getType();
    NeedsExplicitCast = true;
    TypeIsSigned = true;
    break;
  case Instruction::ZExt: case Instruction::Trunc: case Instruction::FPTrunc:
  case Instruction::FPExt: case Instruction::UIToFP: case Instruction::SIToFP:
  case Instruction::FPToUI: case Instruction::FPToSI: case Instruction::PtrToInt:
  case Instruction::IntToPtr: case Instruction::BitCast:
    Ty = CE->getType();
    NeedsExplicitCast = true;
    break;
  default: break;
  }
  if (NeedsExplicitCast) {
    Out << "((";
    if (Ty->isIntegerTy() && Ty != Type::getInt1Ty(Ty->getContext()))
      printSimpleType(Out, Ty, TypeIsSigned);
    else
      printType(Out, Ty); // not integer, sign doesn't matter
    Out << ")(";
  }
  return NeedsExplicitCast;
}
void CWriter::printConstantWithCast(Constant* CPV, unsigned Opcode) {
  Type* OpTy = CPV->getType();
  bool shouldCast = false;
  bool typeIsSigned = false;
  switch (Opcode) {
    default:
      break;
    case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
    case Instruction::LShr: case Instruction::UDiv: case Instruction::URem:
      shouldCast = true;
      break;
    case Instruction::AShr: case Instruction::SDiv: case Instruction::SRem:
      shouldCast = true;
      typeIsSigned = true;
      break;
  }
  if (shouldCast) {
    Out << "((";
    printSimpleType(Out, OpTy, typeIsSigned);
    Out << ")";
    printConstant(CPV, false);
    Out << ")";
  } else
    printConstant(CPV, false);
}
std::string CWriter::GetValueName(const Value *Operand) {
  if (const GlobalAlias *GA = dyn_cast<GlobalAlias>(Operand)) {
    if (const Value *V = GA->resolveAliasedGlobal(false))
      Operand = V;
  }
  if (const GlobalValue *GV = dyn_cast<GlobalValue>(Operand)) {
    SmallString<128> Str;
printf("[%s:%d] manglegetname\n", __FUNCTION__, __LINE__);
    //Mang->getNameWithPrefix(Str, GV, false);
    return CBEMangle(Str.str().str());
  }
  std::string Name = Operand->getName();
  if (Name.empty()) { // Assign unique names to local temporaries.
    unsigned &No = AnonValueNumbers[Operand];
    if (No == 0)
      No = ++NextAnonValueNumber;
    Name = "tmp__" + utostr(No);
  }
  std::string VarName;
  VarName.reserve(Name.capacity());
  for (std::string::iterator I = Name.begin(), E = Name.end();
       I != E; ++I) {
    char ch = *I;
    if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
          (ch >= '0' && ch <= '9') || ch == '_')) {
      char buffer[5];
      sprintf(buffer, "_%x_", ch);
      VarName += buffer;
    } else
      VarName += ch;
  }
  return "llvm_cbe_" + VarName;
}
void CWriter::writeInstComputationInline(Instruction &I) {
  Type *Ty = I.getType();
  if (Ty->isIntegerTy() && (Ty!=Type::getInt1Ty(I.getContext()) &&
        Ty!=Type::getInt8Ty(I.getContext()) &&
        Ty!=Type::getInt16Ty(I.getContext()) &&
        Ty!=Type::getInt32Ty(I.getContext()) &&
        Ty!=Type::getInt64Ty(I.getContext()))) {
      report_fatal_error("The C backend does not currently support integer "
                        "types of widths other than 1, 8, 16, 32, 64.\n"
                        "This is being tracked as PR 4158.");
  }
  bool NeedBoolTrunc = false;
  if (I.getType() == Type::getInt1Ty(I.getContext()) &&
      !isa<ICmpInst>(I) && !isa<FCmpInst>(I))
    NeedBoolTrunc = true;
  if (NeedBoolTrunc)
    Out << "((";
  visit(I);
  if (NeedBoolTrunc)
    Out << ")&1)";
}
void CWriter::writeOperandInternal(Value *Operand, bool Static) {
  if (Instruction *I = dyn_cast<Instruction>(Operand))
    if (isInlinableInst(*I) && !isDirectAlloca(I)) {
      Out << '(';
      writeInstComputationInline(*I);
      Out << ')';
      return;
    }
  Constant* CPV = dyn_cast<Constant>(Operand);
  if (CPV && !isa<GlobalValue>(CPV))
    printConstant(CPV, Static);
  else
    Out << GetValueName(Operand);
}
void CWriter::writeOperand(Value *Operand, bool Static) {
  bool isAddressImplicit = isAddressExposed(Operand);
  if (isAddressImplicit)
    Out << "(&";  // Global variables are referenced as their addresses by llvm
  writeOperandInternal(Operand, Static);
  if (isAddressImplicit)
    Out << ')';
}
bool CWriter::writeInstructionCast(const Instruction &I) {
  Type *Ty = I.getOperand(0)->getType();
  switch (I.getOpcode()) {
  case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
  case Instruction::LShr: case Instruction::URem: case Instruction::UDiv:
    Out << "((";
    printSimpleType(Out, Ty, false);
    Out << ")(";
    return true;
  case Instruction::AShr: case Instruction::SRem: case Instruction::SDiv:
    Out << "((";
    printSimpleType(Out, Ty, true);
    Out << ")(";
    return true;
  default: break;
  }
  return false;
}
void CWriter::writeOperandWithCast(Value* Operand, unsigned Opcode) {
  Type* OpTy = Operand->getType();
  bool shouldCast = false;
  bool castIsSigned = false;
  switch (Opcode) {
    default:
      break;
    case Instruction::Add: case Instruction::Sub: case Instruction::Mul:
    case Instruction::LShr: case Instruction::UDiv:
    case Instruction::URem: // Cast to unsigned first
      shouldCast = true;
      castIsSigned = false;
      break;
    case Instruction::GetElementPtr:
    case Instruction::AShr:
    case Instruction::SDiv:
    case Instruction::SRem: // Cast to signed first
      shouldCast = true;
      castIsSigned = true;
      break;
  }
  if (shouldCast) {
    Out << "((";
    printSimpleType(Out, OpTy, castIsSigned);
    Out << ")";
    writeOperand(Operand);
    Out << ")";
  } else
    writeOperand(Operand);
}
void CWriter::writeOperandWithCast(Value* Operand, const ICmpInst &Cmp) {
  bool shouldCast = Cmp.isRelational();
  if (!shouldCast) {
    writeOperand(Operand);
    return;
  }
  bool castIsSigned = Cmp.isSigned();
  Type* OpTy = Operand->getType();
  if (OpTy->isPointerTy()) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
exit(1);
    //OpTy = TD->getIntPtrType(Operand->getContext());
}
  Out << "((";
  printSimpleType(Out, OpTy, castIsSigned);
  Out << ")";
  writeOperand(Operand);
  Out << ")";
}
static void FindStaticTors(GlobalVariable *GV, std::set<Function*> &StaticTors){
  ConstantArray *InitList = dyn_cast<ConstantArray>(GV->getInitializer());
  if (!InitList) return;
  for (unsigned i = 0, e = InitList->getNumOperands(); i != e; ++i)
    if (ConstantStruct *CS = dyn_cast<ConstantStruct>(InitList->getOperand(i))){
      if (CS->getNumOperands() != 2) return;  // Not array of 2-element structs.
      if (CS->getOperand(1)->isNullValue())
        return;  // Found a null terminator, exit printing.
      Constant *FP = CS->getOperand(1);
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(FP))
        if (CE->isCast())
          FP = CE->getOperand(0);
      if (Function *F = dyn_cast<Function>(FP))
        StaticTors.insert(F);
    }
}
enum SpecialGlobalClass {
  NotSpecial = 0,
  GlobalCtors, GlobalDtors,
  NotPrinted
};
static SpecialGlobalClass getGlobalVariableClass(const GlobalVariable *GV) {
  if (GV->hasAppendingLinkage() && GV->use_empty()) {
    if (GV->getName() == "llvm.global_ctors")
      return GlobalCtors;
    else if (GV->getName() == "llvm.global_dtors")
      return GlobalDtors;
  }
  if (GV->getSection() == "llvm.metadata")
    return NotPrinted;
  return NotSpecial;
}
static void PrintEscapedString(const char *Str, unsigned Length, raw_ostream &Out) {
  for (unsigned i = 0; i != Length; ++i) {
    unsigned char C = Str[i];
    if (isprint(C) && C != '\\' && C != '"')
      Out << C;
    else if (C == '\\')
      Out << "\\\\";
    else if (C == '\"')
      Out << "\\\"";
    else if (C == '\t')
      Out << "\\t";
    else
      Out << "\\x" << hexdigit(C >> 4) << hexdigit(C & 0x0F);
  }
}
static void PrintEscapedString(const std::string &Str, raw_ostream &Out) {
  PrintEscapedString(Str.c_str(), Str.size(), Out);
}
bool CWriter::doInitialization(Module &M) {
  FunctionPass::doInitialization(M);
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
#if 0
  //TheModule = &M;
  //TD = new TargetData(&M);
  //IL = new IntrinsicLowering(*EE->getDataLayout());//*TD);
  //IL->AddPrototypes(M);
  std::string Triple = TheModule->getTargetTriple();
  if (Triple.empty())
    Triple = llvm::sys::getHostTriple();
  std::string E;
  if (const Target *Match = TargetRegistry::lookupTarget(Triple, E))
    TAsm = Match->createMCAsmInfo(Triple);
  TAsm = new CBEMCAsmInfo();
  //MRI  = new MCRegisterInfo();
  //TCtx = new MCContext(*TAsm, *MRI, NULL);
  //Mang = new Mangler(*TCtx, *EE->getDataLayout());//, *TD);
#endif
  std::set<Function*> StaticCtors, StaticDtors;
  for (Module::global_iterator I = M.global_begin(), E = M.global_end();
       I != E; ++I) {
    switch (getGlobalVariableClass(I)) {
    default: break;
    case GlobalCtors:
      FindStaticTors(I, StaticCtors);
      break;
    case GlobalDtors:
      FindStaticTors(I, StaticDtors);
      break;
    }
  }
  Out << "/* Provide Declarations */\n";
  Out << "#include <stdarg.h>\n";      // Varargs support
  Out << "#include <setjmp.h>\n";      // Unwind support
  Out << "#include <limits.h>\n";      // With overflow intrinsics support.
  //generateCompilerSpecificCode(Out, TD);
  Out << "\n"
      << "#ifndef __cplusplus\ntypedef unsigned char bool;\n#endif\n"
      << "\n\n/* Support for floating point constants */\n"
      << "typedef unsigned long long ConstantDoubleTy;\n"
      << "typedef unsigned int        ConstantFloatTy;\n"
      << "typedef struct { unsigned long long f1; unsigned short f2; "
         "unsigned short pad[3]; } ConstantFP80Ty;\n"
      << "typedef struct { unsigned long long f1; unsigned long long f2; }"
         " ConstantFP128Ty;\n"
      << "\n\n/* Global Declarations */\n";
  if (!M.getModuleInlineAsm().empty()) {
    Out << "/* Module asm statements */\n"
        << "asm(";
    std::string Asm = M.getModuleInlineAsm();
    size_t CurPos = 0;
    size_t NewLine = Asm.find_first_of('\n', CurPos);
    while (NewLine != std::string::npos) {
      Out << "\"";
      PrintEscapedString(std::string(Asm.begin()+CurPos, Asm.begin()+NewLine),
                         Out);
      Out << "\\n\"\n";
      CurPos = NewLine+1;
      NewLine = Asm.find_first_of('\n', CurPos);
    }
    Out << "\"";
    PrintEscapedString(std::string(Asm.begin()+CurPos, Asm.end()), Out);
    Out << "\");\n"
        << "/* End Module asm statements */\n";
  }
  printModuleTypes();
  if (!M.global_empty()) {
    Out << "\n/* External Global Variable Declarations */\n";
    for (Module::global_iterator I = M.global_begin(), E = M.global_end();
         I != E; ++I) {
      if (I->hasExternalLinkage() || I->hasExternalWeakLinkage() ||
          I->hasCommonLinkage())
        Out << "extern ";
      else if (I->hasDLLImportLinkage())
        Out << "__declspec(dllimport) ";
      else
        continue; // Internal Global
      if (I->isThreadLocal())
        Out << "__thread ";
      printType(Out, I->getType()->getElementType(), false, GetValueName(I));
      if (I->hasExternalWeakLinkage())
         Out << " __EXTERNAL_WEAK__";
      Out << ";\n";
    }
  }
  Out << "\n/* Function Declarations */\n";
  Out << "double fmod(double, double);\n";   // Support for FP rem
  Out << "float fmodf(float, float);\n";
  Out << "long double fmodl(long double, long double);\n";
  SmallVector<const Function*, 8> intrinsicsToDefine;
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    if (I->isIntrinsic()) {
      switch (I->getIntrinsicID()) {
        default:
          break;
        case Intrinsic::uadd_with_overflow:
        case Intrinsic::sadd_with_overflow:
          intrinsicsToDefine.push_back(I);
          break;
      }
      continue;
    }
    if (I->getName() == "setjmp" ||
        I->getName() == "longjmp" || I->getName() == "_setjmp")
      continue;
    if (I->hasExternalWeakLinkage())
      Out << "extern ";
    printFunctionSignature(I, true);
    if (I->hasWeakLinkage() || I->hasLinkOnceLinkage())
      Out << " __ATTRIBUTE_WEAK__";
    if (I->hasExternalWeakLinkage())
      Out << " __EXTERNAL_WEAK__";
    if (StaticCtors.count(I))
      Out << " __ATTRIBUTE_CTOR__";
    if (StaticDtors.count(I))
      Out << " __ATTRIBUTE_DTOR__";
    if (I->hasHiddenVisibility())
      Out << " __HIDDEN__";
    if (I->hasName() && I->getName()[0] == 1)
      Out << " LLVM_ASM(\"" << I->getName().substr(1) << "\")";
    Out << ";\n";
  }
  if (!M.global_empty()) {
    Out << "\n\n/* Global Variable Declarations */\n";
    for (Module::global_iterator I = M.global_begin(), E = M.global_end();
         I != E; ++I)
      if (!I->isDeclaration()) {
        if (getGlobalVariableClass(I))
          continue;
        if (I->hasLocalLinkage())
          Out << "static ";
        else
          Out << "extern ";
        if (I->isThreadLocal())
          Out << "__thread ";
        printType(Out, I->getType()->getElementType(), false,
                  GetValueName(I));
        if (I->hasLinkOnceLinkage())
          Out << " __attribute__((common))";
        else if (I->hasCommonLinkage())     // FIXME is this right?
          Out << " __ATTRIBUTE_WEAK__";
        else if (I->hasWeakLinkage())
          Out << " __ATTRIBUTE_WEAK__";
        else if (I->hasExternalWeakLinkage())
          Out << " __EXTERNAL_WEAK__";
        if (I->hasHiddenVisibility())
          Out << " __HIDDEN__";
        Out << ";\n";
      }
  }
  if (!M.global_empty()) {
    Out << "\n\n/* Global Variable Definitions and Initialization */\n";
    for (Module::global_iterator I = M.global_begin(), E = M.global_end();
         I != E; ++I)
      if (!I->isDeclaration()) {
        if (getGlobalVariableClass(I))
          continue;
        if (I->hasLocalLinkage())
          Out << "static ";
        else if (I->hasDLLImportLinkage())
          Out << "__declspec(dllimport) ";
        else if (I->hasDLLExportLinkage())
          Out << "__declspec(dllexport) ";
        if (I->isThreadLocal())
          Out << "__thread ";
        printType(Out, I->getType()->getElementType(), false,
                  GetValueName(I));
        if (I->hasLinkOnceLinkage())
          Out << " __attribute__((common))";
        else if (I->hasWeakLinkage())
          Out << " __ATTRIBUTE_WEAK__";
        else if (I->hasCommonLinkage())
          Out << " __ATTRIBUTE_WEAK__";
        if (I->hasHiddenVisibility())
          Out << " __HIDDEN__";
        if (!I->getInitializer()->isNullValue()) {
          Out << " = " ;
          writeOperand(I->getInitializer(), true);
        } else if (I->hasWeakLinkage()) {
          Out << " = " ;
          if (I->getInitializer()->getType()->isStructTy() ||
              I->getInitializer()->getType()->isVectorTy()) {
            Out << "{ 0 }";
          } else if (I->getInitializer()->getType()->isArrayTy()) {
            Out << "{ { 0 } }";
          } else {
            writeOperand(I->getInitializer(), true);
          }
        }
        Out << ";\n";
      }
  }
  if (!M.empty())
    Out << "\n\n/* Function Bodies */\n";
  Out << "static inline int llvm_fcmp_ord(double X, double Y) { ";
  Out << "return X == X && Y == Y; }\n";
  Out << "static inline int llvm_fcmp_uno(double X, double Y) { ";
  Out << "return X != X || Y != Y; }\n";
  Out << "static inline int llvm_fcmp_ueq(double X, double Y) { ";
  Out << "return X == Y || llvm_fcmp_uno(X, Y); }\n";
  Out << "static inline int llvm_fcmp_une(double X, double Y) { ";
  Out << "return X != Y; }\n";
  Out << "static inline int llvm_fcmp_ult(double X, double Y) { ";
  Out << "return X <  Y || llvm_fcmp_uno(X, Y); }\n";
  Out << "static inline int llvm_fcmp_ugt(double X, double Y) { ";
  Out << "return X >  Y || llvm_fcmp_uno(X, Y); }\n";
  Out << "static inline int llvm_fcmp_ule(double X, double Y) { ";
  Out << "return X <= Y || llvm_fcmp_uno(X, Y); }\n";
  Out << "static inline int llvm_fcmp_uge(double X, double Y) { ";
  Out << "return X >= Y || llvm_fcmp_uno(X, Y); }\n";
  Out << "static inline int llvm_fcmp_oeq(double X, double Y) { ";
  Out << "return X == Y ; }\n";
  Out << "static inline int llvm_fcmp_one(double X, double Y) { ";
  Out << "return X != Y && llvm_fcmp_ord(X, Y); }\n";
  Out << "static inline int llvm_fcmp_olt(double X, double Y) { ";
  Out << "return X <  Y ; }\n";
  Out << "static inline int llvm_fcmp_ogt(double X, double Y) { ";
  Out << "return X >  Y ; }\n";
  Out << "static inline int llvm_fcmp_ole(double X, double Y) { ";
  Out << "return X <= Y ; }\n";
  Out << "static inline int llvm_fcmp_oge(double X, double Y) { ";
  Out << "return X >= Y ; }\n";
  for (SmallVector<const Function*, 8>::const_iterator
       I = intrinsicsToDefine.begin(),
       E = intrinsicsToDefine.end(); I != E; ++I) {
    printIntrinsicDefinition(**I, Out);
  }
  return false;
}
void CWriter::printModuleTypes() {
  Out << "/* Helper union for bitcasts */\n";
  Out << "typedef union {\n";
  Out << "  unsigned int Int32;\n";
  Out << "  unsigned long long Int64;\n";
  Out << "  float Float;\n";
  Out << "  double Double;\n";
  Out << "} llvmBitCastUnion;\n";
  std::vector<StructType*> StructTypes;
  //TheModule->findUsedStructTypes(StructTypes);
  if (StructTypes.empty()) return;
  Out << "/* Structure forward decls */\n";
  unsigned NextTypeID = 0;
  for (unsigned i = 0, e = StructTypes.size(); i != e; ++i) {
    StructType *ST = StructTypes[i];
    if (ST->isLiteral() || ST->getName().empty())
      UnnamedStructIDs[ST] = NextTypeID++;
    std::string Name = getStructName(ST);
    Out << "typedef struct " << Name << ' ' << Name << ";\n";
  }
  Out << '\n';
  SmallPtrSet<Type *, 16> StructPrinted;
  Out << "/* Structure contents */\n";
  for (unsigned i = 0, e = StructTypes.size(); i != e; ++i)
    if (StructTypes[i]->isStructTy())
      printContainedStructs(StructTypes[i], StructPrinted);
}
void CWriter::printContainedStructs(Type *Ty,
                                SmallPtrSet<Type *, 16> &StructPrinted) {
  if (Ty->isPointerTy() || Ty->isPrimitiveType() || Ty->isIntegerTy())
    return;
  for (Type::subtype_iterator I = Ty->subtype_begin(),
       E = Ty->subtype_end(); I != E; ++I)
    printContainedStructs(*I, StructPrinted);
  if (StructType *ST = dyn_cast<StructType>(Ty)) {
    if (!StructPrinted.insert(Ty)) return;
    printType(Out, ST, false, getStructName(ST), true);
    Out << ";\n\n";
  }
}
void CWriter::printFunctionSignature(const Function *F, bool Prototype) {
  bool isStructReturn = F->hasStructRetAttr();
  if (F->hasLocalLinkage()) Out << "static ";
  if (F->hasDLLImportLinkage()) Out << "__declspec(dllimport) ";
  if (F->hasDLLExportLinkage()) Out << "__declspec(dllexport) ";
  switch (F->getCallingConv()) {
   case CallingConv::X86_StdCall:
    Out << "__attribute__((stdcall)) ";
    break;
   case CallingConv::X86_FastCall:
    Out << "__attribute__((fastcall)) ";
    break;
   case CallingConv::X86_ThisCall:
    Out << "__attribute__((thiscall)) ";
    break;
   default:
    break;
  }
  FunctionType *FT = cast<FunctionType>(F->getFunctionType());
  //const AttributeSet &PAL = F->getAttributes();
  std::string tstr;
  raw_string_ostream FunctionInnards(tstr);
  FunctionInnards << GetValueName(F) << '(';
  bool PrintedArg = false;
  if (!F->isDeclaration()) {
    if (!F->arg_empty()) {
      Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end();
      unsigned Idx = 1;
      if (isStructReturn) {
        assert(I != E && "Invalid struct return function!");
        ++I;
        ++Idx;
      }
      std::string ArgName;
      for (; I != E; ++I) {
        if (PrintedArg) FunctionInnards << ", ";
        if (I->hasName() || !Prototype)
          ArgName = GetValueName(I);
        else
          ArgName = "";
        Type *ArgTy = I->getType();
        //if (PAL.paramHasAttr(Idx, Attribute::ByVal)) {
          //ArgTy = cast<PointerType>(ArgTy)->getElementType();
          //ByValParams.insert(I);
        //}
        printType(FunctionInnards, ArgTy,
            /*isSigned=*/false/*PAL.paramHasAttr(Idx, Attribute::SExt)*/,
            ArgName);
        PrintedArg = true;
        ++Idx;
      }
    }
  } else {
    FunctionType::param_iterator I = FT->param_begin(), E = FT->param_end();
    unsigned Idx = 1;
    if (isStructReturn) {
      assert(I != E && "Invalid struct return function!");
      ++I;
      ++Idx;
    }
    for (; I != E; ++I) {
      if (PrintedArg) FunctionInnards << ", ";
      Type *ArgTy = *I;
      //if (PAL.paramHasAttr(Idx, Attribute::ByVal)) {
        //assert(ArgTy->isPointerTy());
        //ArgTy = cast<PointerType>(ArgTy)->getElementType();
      //}
      printType(FunctionInnards, ArgTy,
             /*isSigned=*/false/*PAL.paramHasAttr(Idx, Attribute::SExt)*/);
      PrintedArg = true;
      ++Idx;
    }
  }
  if (!PrintedArg && FT->isVarArg()) {
    FunctionInnards << "int vararg_dummy_arg";
    PrintedArg = true;
  }
  if (FT->isVarArg() && PrintedArg) {
    FunctionInnards << ",...";  // Output varargs portion of signature!
  } else if (!FT->isVarArg() && !PrintedArg) {
    FunctionInnards << "void"; // ret() -> ret(void) in C.
  }
  FunctionInnards << ')';
  Type *RetTy;
  if (!isStructReturn)
    RetTy = F->getReturnType();
  else {
    RetTy = cast<PointerType>(FT->getParamType(0))->getElementType();
  }
  printType(Out, RetTy,
            /*isSigned=*/false, //PAL.paramHasAttr(0, Attribute::SExt),
            FunctionInnards.str());
}
static inline bool isFPIntBitCast(const Instruction &I) {
  if (!isa<BitCastInst>(I))
    return false;
  Type *SrcTy = I.getOperand(0)->getType();
  Type *DstTy = I.getType();
  return (SrcTy->isFloatingPointTy() && DstTy->isIntegerTy()) ||
         (DstTy->isFloatingPointTy() && SrcTy->isIntegerTy());
}
void CWriter::printFunction(Function &F) {
  bool isStructReturn = F.hasStructRetAttr();
  printFunctionSignature(&F, false);
  Out << " {\n";
  if (isStructReturn) {
    Type *StructTy =
      cast<PointerType>(F.arg_begin()->getType())->getElementType();
    Out << "  ";
    printType(Out, StructTy, false, "StructReturn");
    Out << ";  /* Struct return temporary */\n";
    Out << "  ";
    printType(Out, F.arg_begin()->getType(), false,
              GetValueName(F.arg_begin()));
    Out << " = &StructReturn;\n";
  }
  bool PrintedVar = false;
  for (inst_iterator I = inst_begin(&F), E = inst_end(&F); I != E; ++I) {
    if (const AllocaInst *AI = isDirectAlloca(&*I)) {
      Out << "  ";
      printType(Out, AI->getAllocatedType(), false, GetValueName(AI));
      Out << ";    /* Address-exposed local */\n";
      PrintedVar = true;
    } else if (I->getType() != Type::getVoidTy(F.getContext()) &&
               !isInlinableInst(*I)) {
      Out << "  ";
      printType(Out, I->getType(), false, GetValueName(&*I));
      Out << ";\n";
      if (isa<PHINode>(*I)) {  // Print out PHI node temporaries as well...
        Out << "  ";
        printType(Out, I->getType(), false,
                  GetValueName(&*I)+"__PHI_TEMPORARY");
        Out << ";\n";
      }
      PrintedVar = true;
    }
    if (isFPIntBitCast(*I)) {
      Out << "  llvmBitCastUnion " << GetValueName(&*I)
          << "__BITCAST_TEMPORARY;\n";
      PrintedVar = true;
    }
  }
  if (PrintedVar)
    Out << '\n';
  if (F.hasExternalLinkage() && F.getName() == "main")
    Out << "  CODE_FOR_MAIN();\n";
  for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
    //if (Loop *L = LI->getLoopFor(BB)) {
      //if (L->getHeader() == BB && L->getParentLoop() == 0)
        //printLoop(L);
    //} else {
      printBasicBlock(BB);
    //}
  }
  Out << "}\n\n";
}
void CWriter::printBasicBlock(BasicBlock *BB) {
  bool NeedsLabel = false;
  for (pred_iterator PI = pred_begin(BB), E = pred_end(BB); PI != E; ++PI)
    if (isGotoCodeNecessary(*PI, BB)) {
      NeedsLabel = true;
      break;
    }
  if (NeedsLabel) Out << GetValueName(BB) << ":\n";
  for (BasicBlock::iterator II = BB->begin(), E = --BB->end(); II != E; ++II) {
    if (!isInlinableInst(*II) && !isDirectAlloca(II)) {
      if (II->getType() != Type::getVoidTy(BB->getContext()) && !isInlineAsm(*II))
        outputLValue(II);
      else
        Out << "  ";
      writeInstComputationInline(*II);
      Out << ";\n";
    }
  }
  visit(*BB->getTerminator());
}
void CWriter::visitReturnInst(ReturnInst &I) {
  bool isStructReturn = I.getParent()->getParent()->hasStructRetAttr();
  if (isStructReturn) {
    Out << "  return StructReturn;\n";
    return;
  }
  if (I.getNumOperands() == 0 &&
      &*--I.getParent()->getParent()->end() == I.getParent() &&
      !I.getParent()->size() == 1) {
    return;
  }
  Out << "  return";
  if (I.getNumOperands()) {
    Out << ' ';
    writeOperand(I.getOperand(0));
  }
  Out << ";\n";
}
void CWriter::visitSwitchInst(SwitchInst &SI) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
exit(1);
}
void CWriter::visitIndirectBrInst(IndirectBrInst &IBI) {
  Out << "  goto *(void*)(";
  writeOperand(IBI.getOperand(0));
  Out << ");\n";
}
void CWriter::visitUnreachableInst(UnreachableInst &I) {
  Out << "  /*UNREACHABLE*/;\n";
}
bool CWriter::isGotoCodeNecessary(BasicBlock *From, BasicBlock *To) {
  return true;
  if (llvm::next(Function::iterator(From)) != Function::iterator(To))
    return true;  // Not the direct successor, we need a goto.
  //if (LI->getLoopFor(From) != LI->getLoopFor(To))
    //return true;
  return false;
}
void CWriter::printPHICopiesForSuccessor (BasicBlock *CurBlock, BasicBlock *Successor, unsigned Indent) {
  for (BasicBlock::iterator I = Successor->begin(); isa<PHINode>(I); ++I) {
    PHINode *PN = cast<PHINode>(I);
    Value *IV = PN->getIncomingValueForBlock(CurBlock);
    if (!isa<UndefValue>(IV)) {
      Out << std::string(Indent, ' ');
      Out << "  " << GetValueName(I) << "__PHI_TEMPORARY = ";
      writeOperand(IV);
      Out << ";   /* for PHI node */\n";
    }
  }
}
void CWriter::printBranchToBlock(BasicBlock *CurBB, BasicBlock *Succ, unsigned Indent) {
  if (isGotoCodeNecessary(CurBB, Succ)) {
    Out << std::string(Indent, ' ') << "  goto ";
    writeOperand(Succ);
    Out << ";\n";
  }
}
void CWriter::visitBranchInst(BranchInst &I) {
  if (I.isConditional()) {
    if (isGotoCodeNecessary(I.getParent(), I.getSuccessor(0))) {
      Out << "  if (";
      writeOperand(I.getCondition());
      Out << ") {\n";
      printPHICopiesForSuccessor (I.getParent(), I.getSuccessor(0), 2);
      printBranchToBlock(I.getParent(), I.getSuccessor(0), 2);
      if (isGotoCodeNecessary(I.getParent(), I.getSuccessor(1))) {
        Out << "  } else {\n";
        printPHICopiesForSuccessor (I.getParent(), I.getSuccessor(1), 2);
        printBranchToBlock(I.getParent(), I.getSuccessor(1), 2);
      }
    } else {
      Out << "  if (!";
      writeOperand(I.getCondition());
      Out << ") {\n";
      printPHICopiesForSuccessor (I.getParent(), I.getSuccessor(1), 2);
      printBranchToBlock(I.getParent(), I.getSuccessor(1), 2);
    }
    Out << "  }\n";
  } else {
    printPHICopiesForSuccessor (I.getParent(), I.getSuccessor(0), 0);
    printBranchToBlock(I.getParent(), I.getSuccessor(0), 0);
  }
  Out << "\n";
}
void CWriter::visitPHINode(PHINode &I) {
  writeOperand(&I);
  Out << "__PHI_TEMPORARY";
}
void CWriter::visitBinaryOperator(Instruction &I) {
  assert(!I.getType()->isPointerTy());
  bool needsCast = false;
  if ((I.getType() == Type::getInt8Ty(I.getContext())) ||
      (I.getType() == Type::getInt16Ty(I.getContext()))
      || (I.getType() == Type::getFloatTy(I.getContext()))) {
    needsCast = true;
    Out << "((";
    printType(Out, I.getType(), false);
    Out << ")(";
  }
  if (BinaryOperator::isNeg(&I)) {
    Out << "-(";
    writeOperand(BinaryOperator::getNegArgument(cast<BinaryOperator>(&I)));
    Out << ")";
  } else if (BinaryOperator::isFNeg(&I)) {
    Out << "-(";
    writeOperand(BinaryOperator::getFNegArgument(cast<BinaryOperator>(&I)));
    Out << ")";
  } else if (I.getOpcode() == Instruction::FRem) {
    if (I.getType() == Type::getFloatTy(I.getContext()))
      Out << "fmodf(";
    else if (I.getType() == Type::getDoubleTy(I.getContext()))
      Out << "fmod(";
    else  // all 3 flavors of long double
      Out << "fmodl(";
    writeOperand(I.getOperand(0));
    Out << ", ";
    writeOperand(I.getOperand(1));
    Out << ")";
  } else {
    bool NeedsClosingParens = writeInstructionCast(I);
    writeOperandWithCast(I.getOperand(0), I.getOpcode());
    switch (I.getOpcode()) {
    case Instruction::Add: case Instruction::FAdd: Out << " + "; break;
    case Instruction::Sub: case Instruction::FSub: Out << " - "; break;
    case Instruction::Mul: case Instruction::FMul: Out << " * "; break;
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem: Out << " % "; break;
    case Instruction::UDiv: case Instruction::SDiv: case Instruction::FDiv: Out << " / "; break;
    case Instruction::And:  Out << " & "; break;
    case Instruction::Or:   Out << " | "; break;
    case Instruction::Xor:  Out << " ^ "; break;
    case Instruction::Shl : Out << " << "; break;
    case Instruction::LShr: case Instruction::AShr: Out << " >> "; break;
    default:
       errs() << "Invalid operator type!" << I;
       llvm_unreachable(0);
    }
    writeOperandWithCast(I.getOperand(1), I.getOpcode());
    if (NeedsClosingParens)
      Out << "))";
  }
  if (needsCast) {
    Out << "))";
  }
}
void CWriter::visitICmpInst(ICmpInst &I) {
  bool needsCast = false;
  bool NeedsClosingParens = writeInstructionCast(I);
  writeOperandWithCast(I.getOperand(0), I);
  switch (I.getPredicate()) {
  case ICmpInst::ICMP_EQ:  Out << " == "; break;
  case ICmpInst::ICMP_NE:  Out << " != "; break;
  case ICmpInst::ICMP_ULE: case ICmpInst::ICMP_SLE: Out << " <= "; break;
  case ICmpInst::ICMP_UGE: case ICmpInst::ICMP_SGE: Out << " >= "; break;
  case ICmpInst::ICMP_ULT: case ICmpInst::ICMP_SLT: Out << " < "; break;
  case ICmpInst::ICMP_UGT: case ICmpInst::ICMP_SGT: Out << " > "; break;
  default:
    errs() << "Invalid icmp predicate!" << I;
    llvm_unreachable(0);
  }
  writeOperandWithCast(I.getOperand(1), I);
  if (NeedsClosingParens)
    Out << "))";
  if (needsCast) {
    Out << "))";
  }
}
void CWriter::visitFCmpInst(FCmpInst &I) {
  if (I.getPredicate() == FCmpInst::FCMP_FALSE) {
    Out << "0";
    return;
  }
  if (I.getPredicate() == FCmpInst::FCMP_TRUE) {
    Out << "1";
    return;
  }
  const char* op = 0;
  switch (I.getPredicate()) {
  default: llvm_unreachable("Illegal FCmp predicate");
  case FCmpInst::FCMP_ORD: op = "ord"; break;
  case FCmpInst::FCMP_UNO: op = "uno"; break;
  case FCmpInst::FCMP_UEQ: op = "ueq"; break;
  case FCmpInst::FCMP_UNE: op = "une"; break;
  case FCmpInst::FCMP_ULT: op = "ult"; break;
  case FCmpInst::FCMP_ULE: op = "ule"; break;
  case FCmpInst::FCMP_UGT: op = "ugt"; break;
  case FCmpInst::FCMP_UGE: op = "uge"; break;
  case FCmpInst::FCMP_OEQ: op = "oeq"; break;
  case FCmpInst::FCMP_ONE: op = "one"; break;
  case FCmpInst::FCMP_OLT: op = "olt"; break;
  case FCmpInst::FCMP_OLE: op = "ole"; break;
  case FCmpInst::FCMP_OGT: op = "ogt"; break;
  case FCmpInst::FCMP_OGE: op = "oge"; break;
  }
  Out << "llvm_fcmp_" << op << "(";
  writeOperand(I.getOperand(0));
  Out << ", ";
  writeOperand(I.getOperand(1));
  Out << ")";
}
static const char * getFloatBitCastField(Type *Ty) {
  switch (Ty->getTypeID()) {
    default: llvm_unreachable("Invalid Type");
    case Type::FloatTyID:  return "Float";
    case Type::DoubleTyID: return "Double";
    case Type::IntegerTyID: {
      unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
      if (NumBits <= 32)
        return "Int32";
      else
        return "Int64";
    }
  }
}
void CWriter::visitCastInst(CastInst &I) {
  Type *DstTy = I.getType();
  Type *SrcTy = I.getOperand(0)->getType();
  if (isFPIntBitCast(I)) {
    Out << '(';
    Out << GetValueName(&I) << "__BITCAST_TEMPORARY."
        << getFloatBitCastField(I.getOperand(0)->getType()) << " = ";
    writeOperand(I.getOperand(0));
    Out << ", " << GetValueName(&I) << "__BITCAST_TEMPORARY."
        << getFloatBitCastField(I.getType());
    Out << ')';
    return;
  }
  Out << '(';
  printCast(I.getOpcode(), SrcTy, DstTy);
  if (SrcTy == Type::getInt1Ty(I.getContext()) &&
      I.getOpcode() == Instruction::SExt)
    Out << "0-";
  writeOperand(I.getOperand(0));
  if (DstTy == Type::getInt1Ty(I.getContext()) &&
      (I.getOpcode() == Instruction::Trunc ||
       I.getOpcode() == Instruction::FPToUI ||
       I.getOpcode() == Instruction::FPToSI ||
       I.getOpcode() == Instruction::PtrToInt)) {
    Out << "&1u";
  }
  Out << ')';
}
void CWriter::visitSelectInst(SelectInst &I) {
  Out << "((";
  writeOperand(I.getCondition());
  Out << ") ? (";
  writeOperand(I.getTrueValue());
  Out << ") : (";
  writeOperand(I.getFalseValue());
  Out << "))";
}
static void printLimitValue(IntegerType &Ty, bool isSigned, bool isMax, raw_ostream &Out) {
  const char* type;
  const char* sprefix = "";
  unsigned NumBits = Ty.getBitWidth();
  if (NumBits <= 8) {
    type = "CHAR";
    sprefix = "S";
  } else if (NumBits <= 16) {
    type = "SHRT";
  } else if (NumBits <= 32) {
    type = "INT";
  } else if (NumBits <= 64) {
    type = "LLONG";
  } else {
    llvm_unreachable("Bit widths > 64 not implemented yet");
  }
  if (isSigned)
    Out << sprefix << type << (isMax ? "_MAX" : "_MIN");
  else
    Out << "U" << type << (isMax ? "_MAX" : "0");
}
static bool isSupportedIntegerSize(IntegerType &T) {
  return T.getBitWidth() == 8 || T.getBitWidth() == 16 ||
         T.getBitWidth() == 32 || T.getBitWidth() == 64;
}
void CWriter::printIntrinsicDefinition(const Function &F, raw_ostream &Out) {
  FunctionType *funT = F.getFunctionType();
  Type *retT = F.getReturnType();
  IntegerType *elemT = cast<IntegerType>(funT->getParamType(1));
  assert(isSupportedIntegerSize(*elemT) &&
         "CBackend does not support arbitrary size integers.");
  assert(cast<StructType>(retT)->getElementType(0) == elemT &&
         elemT == funT->getParamType(0) && funT->getNumParams() == 2);
  switch (F.getIntrinsicID()) {
  default:
    llvm_unreachable("Unsupported Intrinsic.");
  case Intrinsic::uadd_with_overflow:
    Out << "static inline ";
    printType(Out, retT);
    Out << GetValueName(&F);
    Out << "(";
    printSimpleType(Out, elemT, false);
    Out << "a,";
    printSimpleType(Out, elemT, false);
    Out << "b) {\n  ";
    printType(Out, retT);
    Out << "r;\n";
    Out << "  r.field0 = a + b;\n";
    Out << "  r.field1 = (r.field0 < a);\n";
    Out << "  return r;\n}\n";
    break;
  case Intrinsic::sadd_with_overflow:            
    Out << "static ";
    printType(Out, retT);
    Out << GetValueName(&F);
    Out << "(";
    printSimpleType(Out, elemT, true);
    Out << "a,";
    printSimpleType(Out, elemT, true);
    Out << "b) {\n  ";
    printType(Out, retT);
    Out << "r;\n";
    Out << "  r.field1 = (b > 0 && a > ";
    printLimitValue(*elemT, true, true, Out);
    Out << " - b) || (b < 0 && a < ";
    printLimitValue(*elemT, true, false, Out);
    Out << " - b);\n";
    Out << "  r.field0 = r.field1 ? 0 : a + b;\n";
    Out << "  return r;\n}\n";
    break;
  }
}
void CWriter::lowerIntrinsics(Function &F) {
  std::vector<Function*> prototypesToGen;
  for (Function::iterator BB = F.begin(), EE = F.end(); BB != EE; ++BB)
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; )
      if (CallInst *CI = dyn_cast<CallInst>(I++))
        if (Function *F = CI->getCalledFunction())
          switch (F->getIntrinsicID()) {
          case Intrinsic::not_intrinsic: case Intrinsic::vastart: case Intrinsic::vacopy:
          case Intrinsic::vaend: case Intrinsic::returnaddress: case Intrinsic::frameaddress:
          case Intrinsic::setjmp: case Intrinsic::longjmp: case Intrinsic::prefetch:
          case Intrinsic::powi:
          case Intrinsic::x86_sse_cmp_ss: case Intrinsic::x86_sse_cmp_ps:
          case Intrinsic::x86_sse2_cmp_sd: case Intrinsic::x86_sse2_cmp_pd:
          case Intrinsic::ppc_altivec_lvsl:
          case Intrinsic::uadd_with_overflow: case Intrinsic::sadd_with_overflow:
            break;
          default:
            const char *BuiltinName = "";
#define GET_GCC_BUILTIN_NAME
#include "llvm/IR/Intrinsics.gen"
#undef GET_GCC_BUILTIN_NAME
            if (BuiltinName[0]) break;
            Instruction *Before = 0;
            if (CI != &BB->front())
              Before = prior(BasicBlock::iterator(CI));
            //IL->LowerIntrinsicCall(CI);
            if (Before) {        // Move iterator to instruction after call
              I = Before; ++I;
            } else {
              I = BB->begin();
            }
            if (CallInst *Call = dyn_cast<CallInst>(I))
              if (Function *NewF = Call->getCalledFunction())
                if (!NewF->isDeclaration())
                  prototypesToGen.push_back(NewF);
            break;
          }
  std::vector<Function*>::iterator I = prototypesToGen.begin();
  std::vector<Function*>::iterator E = prototypesToGen.end();
  for ( ; I != E; ++I) {
    if (intrinsicPrototypesAlreadyGenerated.insert(*I).second) {
      Out << '\n';
      printFunctionSignature(*I, true);
      Out << ";\n";
    }
  }
}
void CWriter::visitCallInst(CallInst &I) {
  if (isa<InlineAsm>(I.getCalledValue()))
    return visitInlineAsm(I);
  bool WroteCallee = false;
  if (Function *F = I.getCalledFunction())
    if (Intrinsic::ID ID = (Intrinsic::ID)F->getIntrinsicID())
      if (visitBuiltinCall(I, ID, WroteCallee))
        return;
  Value *Callee = I.getCalledValue();
  PointerType  *PTy   = cast<PointerType>(Callee->getType());
  FunctionType *FTy   = cast<FunctionType>(PTy->getElementType());
  const AttributeSet &PAL = I.getAttributes();
  bool hasByVal = I.hasByValArgument();
  bool isStructRet = I.hasStructRetAttr();
  if (isStructRet) {
    writeOperandDeref(I.getArgOperand(0));
    Out << " = ";
  }
  if (I.isTailCall()) Out << " /*tail*/ ";
  if (!WroteCallee) {
    bool NeedsCast = (hasByVal || isStructRet) && !isa<Function>(Callee);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(Callee))
      if (CE->isCast())
        if (Function *RF = dyn_cast<Function>(CE->getOperand(0))) {
          NeedsCast = true;
          Callee = RF;
        }
    if (NeedsCast) {
      Out << "((";
      if (isStructRet)
        printStructReturnPointerFunctionType(Out, PAL,
                             cast<PointerType>(I.getCalledValue()->getType()));
      else if (hasByVal)
        printType(Out, I.getCalledValue()->getType(), false, "", true, PAL);
      else
        printType(Out, I.getCalledValue()->getType());
      Out << ")(void*)";
    }
    writeOperand(Callee);
    if (NeedsCast) Out << ')';
  }
  Out << '(';
  bool PrintedArg = false;
  if(FTy->isVarArg() && !FTy->getNumParams()) {
    Out << "0 /*dummy arg*/";
    PrintedArg = true;
  }
  unsigned NumDeclaredParams = FTy->getNumParams();
  CallSite CS(&I);
  CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
  unsigned ArgNo = 0;
  if (isStructRet) {   // Skip struct return argument.
    ++AI;
    ++ArgNo;
  }
  for (; AI != AE; ++AI, ++ArgNo) {
    if (PrintedArg) Out << ", ";
    if (ArgNo < NumDeclaredParams &&
        (*AI)->getType() != FTy->getParamType(ArgNo)) {
      Out << '(';
      printType(Out, FTy->getParamType(ArgNo),
            /*isSigned=*/false); //PAL.paramHasAttr(ArgNo+1, Attribute::SExt));
      Out << ')';
    }
#if 0
    if (I.paramHasAttr(ArgNo+1, Attribute::ByVal))
      writeOperandDeref(*AI);
    else
#endif
      writeOperand(*AI);
    PrintedArg = true;
  }
  Out << ')';
}
bool CWriter::visitBuiltinCall(CallInst &I, Intrinsic::ID ID,
                               bool &WroteCallee) {
  switch (ID) {
  default: {
    const char *BuiltinName = "";
    //Function *F = I.getCalledFunction();
#define GET_GCC_BUILTIN_NAME
#include "llvm/IR/Intrinsics.gen"
#undef GET_GCC_BUILTIN_NAME
    assert(BuiltinName[0] && "Unknown LLVM intrinsic!");
    Out << BuiltinName;
    WroteCallee = true;
    return false;
  }
  case Intrinsic::vastart:
    Out << "0; ";
    Out << "va_start(*(va_list*)";
    writeOperand(I.getArgOperand(0));
    Out << ", ";
    if (I.getParent()->getParent()->arg_empty())
      Out << "vararg_dummy_arg";
    else
      writeOperand(--I.getParent()->getParent()->arg_end());
    Out << ')';
    return true;
  case Intrinsic::vaend:
    if (!isa<ConstantPointerNull>(I.getArgOperand(0))) {
      Out << "0; va_end(*(va_list*)";
      writeOperand(I.getArgOperand(0));
      Out << ')';
    } else {
      Out << "va_end(*(va_list*)0)";
    }
    return true;
  case Intrinsic::vacopy:
    Out << "0; ";
    Out << "va_copy(*(va_list*)";
    writeOperand(I.getArgOperand(0));
    Out << ", *(va_list*)";
    writeOperand(I.getArgOperand(1));
    Out << ')';
    return true;
  case Intrinsic::returnaddress:
    Out << "__builtin_return_address(";
    writeOperand(I.getArgOperand(0));
    Out << ')';
    return true;
  case Intrinsic::frameaddress:
    Out << "__builtin_frame_address(";
    writeOperand(I.getArgOperand(0));
    Out << ')';
    return true;
  case Intrinsic::powi:
    Out << "__builtin_powi(";
    writeOperand(I.getArgOperand(0));
    Out << ", ";
    writeOperand(I.getArgOperand(1));
    Out << ')';
    return true;
  case Intrinsic::setjmp:
    Out << "setjmp(*(jmp_buf*)";
    writeOperand(I.getArgOperand(0));
    Out << ')';
    return true;
  case Intrinsic::longjmp:
    Out << "longjmp(*(jmp_buf*)";
    writeOperand(I.getArgOperand(0));
    Out << ", ";
    writeOperand(I.getArgOperand(1));
    Out << ')';
    return true;
  case Intrinsic::prefetch:
    Out << "LLVM_PREFETCH((const void *)";
    writeOperand(I.getArgOperand(0));
    Out << ", ";
    writeOperand(I.getArgOperand(1));
    Out << ", ";
    writeOperand(I.getArgOperand(2));
    Out << ")";
    return true;
  case Intrinsic::stacksave:
    Out << "0; *((void**)&" << GetValueName(&I)
        << ") = __builtin_stack_save()";
    return true;
  case Intrinsic::x86_sse_cmp_ss:
  case Intrinsic::x86_sse_cmp_ps:
  case Intrinsic::x86_sse2_cmp_sd:
  case Intrinsic::x86_sse2_cmp_pd:
    Out << '(';
    printType(Out, I.getType());
    Out << ')';
    switch (cast<ConstantInt>(I.getArgOperand(2))->getZExtValue()) {
    default: llvm_unreachable("Invalid llvm.x86.sse.cmp!");
    case 0: Out << "__builtin_ia32_cmpeq"; break;
    case 1: Out << "__builtin_ia32_cmplt"; break;
    case 2: Out << "__builtin_ia32_cmple"; break;
    case 3: Out << "__builtin_ia32_cmpunord"; break;
    case 4: Out << "__builtin_ia32_cmpneq"; break;
    case 5: Out << "__builtin_ia32_cmpnlt"; break;
    case 6: Out << "__builtin_ia32_cmpnle"; break;
    case 7: Out << "__builtin_ia32_cmpord"; break;
    }
    if (ID == Intrinsic::x86_sse_cmp_ps || ID == Intrinsic::x86_sse2_cmp_pd)
      Out << 'p';
    else
      Out << 's';
    if (ID == Intrinsic::x86_sse_cmp_ss || ID == Intrinsic::x86_sse_cmp_ps)
      Out << 's';
    else
      Out << 'd';
    Out << "(";
    writeOperand(I.getArgOperand(0));
    Out << ", ";
    writeOperand(I.getArgOperand(1));
    Out << ")";
    return true;
  case Intrinsic::ppc_altivec_lvsl:
    Out << '(';
    printType(Out, I.getType());
    Out << ')';
    Out << "__builtin_altivec_lvsl(0, (void*)";
    writeOperand(I.getArgOperand(0));
    Out << ")";
    return true;
  case Intrinsic::uadd_with_overflow:
  case Intrinsic::sadd_with_overflow:
    Out << GetValueName(I.getCalledFunction()) << "(";
    writeOperand(I.getArgOperand(0));
    Out << ", ";
    writeOperand(I.getArgOperand(1));
    Out << ")";
    return true;
  }
}
void CWriter::visitInlineAsm(CallInst &CI) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
exit(1);
}
void CWriter::visitAllocaInst(AllocaInst &I) {
  Out << '(';
  printType(Out, I.getType());
  Out << ") alloca(sizeof(";
  printType(Out, I.getType()->getElementType());
  Out << ')';
  if (I.isArrayAllocation()) {
    Out << " * " ;
    writeOperand(I.getOperand(0));
  }
  Out << ')';
}
void CWriter::printGEPExpression(Value *Ptr, gep_type_iterator I,
                                 gep_type_iterator E, bool Static) {
  if (I == E) {
    writeOperand(Ptr);
    return;
  }
  VectorType *LastIndexIsVector = 0;
  {
    for (gep_type_iterator TmpI = I; TmpI != E; ++TmpI)
      LastIndexIsVector = dyn_cast<VectorType>(*TmpI);
  }
  Out << "(";
  if (LastIndexIsVector) {
    Out << "((";
    printType(Out, PointerType::getUnqual(LastIndexIsVector->getElementType()));
    Out << ")(";
  }
  Out << '&';
  Value *FirstOp = I.getOperand();
  if (!isa<Constant>(FirstOp) || !cast<Constant>(FirstOp)->isNullValue()) {
    writeOperand(Ptr);
  } else {
    ++I;  // Skip the zero index.
    if (isAddressExposed(Ptr)) {
      writeOperandInternal(Ptr, Static);
    } else if (I != E && (*I)->isStructTy()) {
      writeOperand(Ptr);
      Out << "->field" << cast<ConstantInt>(I.getOperand())->getZExtValue();
      ++I;  // eat the struct index as well.
    } else {
      Out << "(*";
      writeOperand(Ptr);
      Out << ")";
    }
  }
  for (; I != E; ++I) {
    if ((*I)->isStructTy()) {
      Out << ".field" << cast<ConstantInt>(I.getOperand())->getZExtValue();
    } else if ((*I)->isArrayTy()) {
      Out << ".array[";
      writeOperandWithCast(I.getOperand(), Instruction::GetElementPtr);
      Out << ']';
    } else if (!(*I)->isVectorTy()) {
      Out << '[';
      writeOperandWithCast(I.getOperand(), Instruction::GetElementPtr);
      Out << ']';
    } else {
      if (isa<Constant>(I.getOperand()) &&
          cast<Constant>(I.getOperand())->isNullValue()) {
        Out << "))";  // avoid "+0".
      } else {
        Out << ")+(";
        writeOperandWithCast(I.getOperand(), Instruction::GetElementPtr);
        Out << "))";
      }
    }
  }
  Out << ")";
}
void CWriter::writeMemoryAccess(Value *Operand, Type *OperandType,
                                bool IsVolatile, unsigned Alignment) {
  bool IsUnaligned = Alignment &&
    1; //Alignment < TD->getABITypeAlignment(OperandType);
  if (!IsUnaligned)
    Out << '*';
  if (IsVolatile || IsUnaligned) {
    Out << "((";
    if (IsUnaligned)
      Out << "struct __attribute__ ((packed, aligned(" << Alignment << "))) {";
    printType(Out, OperandType, false, IsUnaligned ? "data" : "volatile*");
    if (IsUnaligned) {
      Out << "; } ";
      if (IsVolatile) Out << "volatile ";
      Out << "*";
    }
    Out << ")";
  }
  writeOperand(Operand);
  if (IsVolatile || IsUnaligned) {
    Out << ')';
    if (IsUnaligned)
      Out << "->data";
  }
}
void CWriter::visitLoadInst(LoadInst &I) {
  writeMemoryAccess(I.getOperand(0), I.getType(), I.isVolatile(),
                    I.getAlignment());
}
void CWriter::visitStoreInst(StoreInst &I) {
  writeMemoryAccess(I.getPointerOperand(), I.getOperand(0)->getType(),
                    I.isVolatile(), I.getAlignment());
  Out << " = ";
  Value *Operand = I.getOperand(0);
  Constant *BitMask = 0;
  if (IntegerType* ITy = dyn_cast<IntegerType>(Operand->getType()))
    if (!ITy->isPowerOf2ByteWidth())
      BitMask = ConstantInt::get(ITy, ITy->getBitMask());
  if (BitMask)
    Out << "((";
  writeOperand(Operand);
  if (BitMask) {
    Out << ") & ";
    printConstant(BitMask, false);
    Out << ")";
  }
}
void CWriter::visitGetElementPtrInst(GetElementPtrInst &I) {
  printGEPExpression(I.getPointerOperand(), gep_type_begin(I),
                     gep_type_end(I), false);
}
void CWriter::visitVAArgInst(VAArgInst &I) {
  Out << "va_arg(*(va_list*)";
  writeOperand(I.getOperand(0));
  Out << ", ";
  printType(Out, I.getType());
  Out << ");\n ";
}
void CWriter::visitInsertElementInst(InsertElementInst &I) {
  Type *EltTy = I.getType()->getElementType();
  writeOperand(I.getOperand(0));
  Out << ";\n  ";
  Out << "((";
  printType(Out, PointerType::getUnqual(EltTy));
  Out << ")(&" << GetValueName(&I) << "))[";
  writeOperand(I.getOperand(2));
  Out << "] = (";
  writeOperand(I.getOperand(1));
  Out << ")";
}
void CWriter::visitExtractElementInst(ExtractElementInst &I) {
  Out << "((";
  Type *EltTy =
    cast<VectorType>(I.getOperand(0)->getType())->getElementType();
  printType(Out, PointerType::getUnqual(EltTy));
  Out << ")(&" << GetValueName(I.getOperand(0)) << "))[";
  writeOperand(I.getOperand(1));
  Out << "]";
}
void CWriter::visitShuffleVectorInst(ShuffleVectorInst &SVI) {
  Out << "(";
  printType(Out, SVI.getType());
  Out << "){ ";
  VectorType *VT = SVI.getType();
  unsigned NumElts = VT->getNumElements();
  Type *EltTy = VT->getElementType();
  for (unsigned i = 0; i != NumElts; ++i) {
    if (i) Out << ", ";
    int SrcVal = SVI.getMaskValue(i);
    if ((unsigned)SrcVal >= NumElts*2) {
      Out << " 0/*undef*/ ";
    } else {
      Value *Op = SVI.getOperand((unsigned)SrcVal >= NumElts);
      if (isa<Instruction>(Op)) {
        Out << "((";
        printType(Out, PointerType::getUnqual(EltTy));
        Out << ")(&" << GetValueName(Op)
            << "))[" << (SrcVal & (NumElts-1)) << "]";
      } else if (isa<ConstantAggregateZero>(Op) || isa<UndefValue>(Op)) {
        Out << "0";
      } else {
        printConstant(cast<ConstantVector>(Op)->getOperand(SrcVal &
                                                           (NumElts-1)),
                      false);
      }
    }
  }
  Out << "}";
}
void CWriter::visitInsertValueInst(InsertValueInst &IVI) {
  writeOperand(IVI.getOperand(0));
  Out << ";\n  ";
  Out << GetValueName(&IVI);
  for (const unsigned *b = IVI.idx_begin(), *i = b, *e = IVI.idx_end();
       i != e; ++i) {
    Type *IndexedTy =
      ExtractValueInst::getIndexedType(IVI.getOperand(0)->getType(),
                                       makeArrayRef(b, i+1));
    if (IndexedTy->isArrayTy())
      Out << ".array[" << *i << "]";
    else
      Out << ".field" << *i;
  }
  Out << " = ";
  writeOperand(IVI.getOperand(1));
}
void CWriter::visitExtractValueInst(ExtractValueInst &EVI) {
  Out << "(";
  if (isa<UndefValue>(EVI.getOperand(0))) {
    Out << "(";
    printType(Out, EVI.getType());
    Out << ") 0/*UNDEF*/";
  } else {
    Out << GetValueName(EVI.getOperand(0));
    for (const unsigned *b = EVI.idx_begin(), *i = b, *e = EVI.idx_end();
         i != e; ++i) {
      Type *IndexedTy =
        ExtractValueInst::getIndexedType(EVI.getOperand(0)->getType(),
                                         makeArrayRef(b, i+1));
      if (IndexedTy->isArrayTy())
        Out << ".array[" << *i << "]";
      else
        Out << ".field" << *i;
    }
  }
  Out << ")";
}
#if 0
bool CTargetMachine::addPassesToEmitFile(PassManagerBase &PM,
                                         formatted_raw_ostream &o,
                                         CodeGenFileType FileType,
                                         CodeGenOpt::Level OptLevel,
                                         bool DisableVerify) {
  if (FileType != TargetMachine::CGFT_AssemblyFile) return true;
  PM.add(createGCLoweringPass());
  PM.add(createLowerInvokePass());
  PM.add(createCFGSimplificationPass());   // clean up after lower invoke.
  PM.add(new CWriter(o));
  PM.add(createGCInfoDeleter());
  return false;
}
#endif
#endif
