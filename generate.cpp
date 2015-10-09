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
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/GenericValue.h"

using namespace llvm;

#include "declarations.h"

int trace_translate ;//= 1;
std::list<VTABLE_WORK> vtablework;
const Function *EntryFn;
const char *globalName;

static int slotmapIndex = 1;
static std::map<const Value *, int> slotmap;
INTMAP_TYPE predText[] = {
    {FCmpInst::FCMP_FALSE, "false"}, {FCmpInst::FCMP_OEQ, "oeq"},
    {FCmpInst::FCMP_OGT, "ogt"}, {FCmpInst::FCMP_OGE, "oge"},
    {FCmpInst::FCMP_OLT, "olt"}, {FCmpInst::FCMP_OLE, "ole"},
    {FCmpInst::FCMP_ONE, "one"}, {FCmpInst::FCMP_ORD, "ord"},
    {FCmpInst::FCMP_UNO, "uno"}, {FCmpInst::FCMP_UEQ, "ueq"},
    {FCmpInst::FCMP_UGT, "ugt"}, {FCmpInst::FCMP_UGE, "uge"},
    {FCmpInst::FCMP_ULT, "ult"}, {FCmpInst::FCMP_ULE, "ule"},
    {FCmpInst::FCMP_UNE, "une"}, {FCmpInst::FCMP_TRUE, "true"},
    {ICmpInst::ICMP_EQ, "=="}, {ICmpInst::ICMP_NE, "!="},
    {ICmpInst::ICMP_SGT, ">"}, {ICmpInst::ICMP_SGE, ">="},
    {ICmpInst::ICMP_SLT, "<"}, {ICmpInst::ICMP_SLE, "<="},
    {ICmpInst::ICMP_UGT, ">"}, {ICmpInst::ICMP_UGE, ">="},
    {ICmpInst::ICMP_ULT, "<"}, {ICmpInst::ICMP_ULE, "<="}, {}};
INTMAP_TYPE opcodeMap[] = {
    {Instruction::Add, "+"}, {Instruction::FAdd, "+"},
    {Instruction::Sub, "-"}, {Instruction::FSub, "-"},
    {Instruction::Mul, "*"}, {Instruction::FMul, "*"},
    {Instruction::UDiv, "/"}, {Instruction::SDiv, "/"}, {Instruction::FDiv, "/"},
    {Instruction::URem, "%"}, {Instruction::SRem, "%"}, {Instruction::FRem, "%"},
    {Instruction::And, "&"}, {Instruction::Or, "|"}, {Instruction::Xor, "^"},
    {Instruction::Shl, "<<"}, {Instruction::LShr, ">>"}, {Instruction::AShr, " >> "}, {}};

/*
 * Common utilities for processing Instruction lists
 */

const char *intmapLookup(INTMAP_TYPE *map, int value)
{
    while (map->name) {
        if (map->value == value)
            return map->name;
        map++;
    }
    return "unknown";
}

void recursiveDelete(Value *V)
{
    Instruction *I = dyn_cast<Instruction>(V);
    if (!I)
        return;
    for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i) {
        Value *OpV = I->getOperand(i);
        I->setOperand(i, 0);
        if (OpV->use_empty())
            recursiveDelete(OpV);
    }
    I->eraseFromParent();
}

/*
 * GEP and Load instructions interpreter functions
 * (just execute using the memory areas allocated by the constructors)
 */
uint64_t executeGEPOperation(gep_type_iterator I, gep_type_iterator E)
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
            const Constant *CV = dyn_cast<Constant>(I.getOperand());
            const ConstantInt *CI;

            if (!CV || isa<GlobalValue>(CV) || !(CI = dyn_cast<ConstantInt>(CV))) {
                printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                exit(1);
            }
            Total += TD->getTypeAllocSize(ST->getElementType()) * CI->getZExtValue();
        }
    }
    return Total;
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
        if (LoadBytes > sizeof(rv)) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        memcpy(&rv, (uint8_t*)Ptr, LoadBytes);
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
        outs() << "Cannot load value of type " << *Ty << "!";
        exit(1);
    }
    if (trace_full)
        printf("[%s:%d] rv %llx\n", __FUNCTION__, __LINE__, (long long)rv);
    return rv;
}

const char *processInstruction(Function ***thisp, Instruction *ins, int generate)
{
    //outs() << "processinst" << *ins;
    switch (ins->getOpcode()) {
    case Instruction::GetElementPtr:
        {
        GetElementPtrInst &IG = static_cast<GetElementPtrInst&>(*ins);
        return printGEPExpression(thisp, IG.getPointerOperand(), gep_type_begin(IG), gep_type_end(IG));
        }
        break;
    case Instruction::Load:
        {
        LoadInst &IL = static_cast<LoadInst&>(*ins);
        ERRORIF (IL.isVolatile());
        return getOperand(thisp, ins->getOperand(0), true);
        }
        break;
    default:
        {
        if (generate == 2)
            return processCInstruction(thisp, *ins);
        else
            return generate ? generateVerilog(thisp, *ins)
                            : calculateGuardUpdate(thisp, *ins);
        }
        break;
    case Instruction::Alloca: // ignore
        if (generate == 2) {
            if (const AllocaInst *AI = isDirectAlloca(&*ins))
              return printType(AI->getAllocatedType(), false, GetValueName(AI), "    ", ";    /* Address-exposed local */\n");
        }
        break;
    }
    return NULL;
}

/*
 * Walk all BasicBlocks for a Function, calling requested processing function
 */
static std::map<const Function *, int> funcSeen;
static void processFunction(VTABLE_WORK &work, int generate, FILE *outputFile)
{
    Function *func = work.f;
    const char *guardName = NULL;
    globalName = strdup(func->getName().str().c_str());
printf("[%s:%d] %p processing %s\n", __FUNCTION__, __LINE__, func, globalName);
//func->dump();
//fprintf(stderr, "\nENDFUNC\n");
    NextAnonValueNumber = 0;
    if (generate == 1 && endswith(globalName, "updateEv")) {
        char temp[MAX_CHAR_BUFFER];
        strcpy(temp, globalName);
        temp[strlen(globalName) - 9] = 0;  // truncate "updateEv"
        guardName = strdup(temp);
    }
    nameMap.clear();
    slotmap.clear();
    slotmapIndex = 1;
    if (trace_translate) {
        printf("FULL_AFTER_OPT: %s\n", func->getName().str().c_str());
        func->dump();
        printf("TRANSLATE:\n");
    }
#if 0
    /* connect up argument formal param names with actual values */
    for (Function::const_arg_iterator AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
        int slotindex = getLocalSlot(AI);
        if (AI->hasByValAttr()) {
            printf("[%s] hasByVal param not supported\n", __FUNCTION__);
            exit(1);
        }
        slotarray[slotindex].name = strdup(AI->getName().str().c_str());
        if (trace_full)
            printf("%s: [%d] '%s'\n", __FUNCTION__, slotindex, slotarray[slotindex].name);
        if (!strcmp(slotarray[slotindex].name, "this"))
            slotarray[slotindex].svalue = (uint8_t *)work.thisp;
        else if (!strcmp(slotarray[slotindex].name, "v")) {
            slotarray[slotindex] = work.arg;
        }
        else
            printf("%s: unknown parameter!! [%d] '%s'\n", __FUNCTION__, slotindex, slotarray[slotindex].name);
    }
#endif
    /* If this is an 'update' method, generate 'if guard' around instruction stream */
    int already_printed_header = 0;
    /* Generate Verilog for all instructions.  Record function calls for post processing */
    if (generate == 2) {
        int status;
        const char *demang = abi::__cxa_demangle(globalName, 0, 0, &status);
        std::map<const Function *, int>::iterator MI = funcSeen.find(func);
        if ((demang && strstr(demang, "::~"))
         || func->isDeclaration() || !strcmp(globalName, "_Z16run_main_programv") || !strcmp(globalName, "main")
         || !strcmp(globalName, "__dtor_echoTest")
         || MI != funcSeen.end())
            return; // MI->second->name;
        funcSeen[func] = 1;
        fprintf(outputFile, "%s", printFunctionSignature(func, false, " {\n", false));
    }
    //manually done (only for methods) nameMap["Vthis"] = work.thisp;
    for (Function::iterator BB = func->begin(), E = func->end(); BB != E; ++BB) {
        if (trace_translate && BB->hasName())         // Print out the label if it exists...
            printf("LLLLL: %s\n", BB->getName().str().c_str());
        for (BasicBlock::iterator ins = BB->begin(), ins_end = BB->end(); ins != ins_end;) {
            char instruction_label[MAX_CHAR_BUFFER];

            BasicBlock::iterator next_ins = llvm::next(BasicBlock::iterator(ins));
            if (trace_translate)
            printf("%s    XLAT:%14s", instruction_label, ins->getOpcodeName());
            if (!isInlinableInst(*ins)) {
                if (trace_translate && generate == 2)
                    printf("/*before %p opcode %d.=%s*/\n", &*ins, ins->getOpcode(), ins->getOpcodeName());
            const char *vout = processInstruction(work.thisp, ins, generate);
            if (vout) {
                if (vout[0] == '&' && !isDirectAlloca(&*ins) && ins->getType() != Type::getVoidTy(BB->getContext())
                 && ins->use_begin() != ins->use_end()) {
                    std::string name = GetValueName(&*ins);
                    void *tval = mapLookup(vout+1);
                    if (tval)
                        nameMap[name] = tval;
                }
                if (generate == 2) {
                    if (strcmp(vout, "")) {
                        if (!isDirectAlloca(&*ins) && ins->getType() != Type::getVoidTy(BB->getContext())
                         && ins->use_begin() != ins->use_end()) {
                            std::string name = GetValueName(&*ins);
                            fprintf(outputFile, "%s", printType(ins->getType(), false, name, "", " = "));
                        }
                        fprintf(outputFile, "        %s;\n", vout);
                    }
                }
                else {
                    if (!already_printed_header) {
                        fprintf(outputFile, "\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n; %s\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n", globalName);
                        if (guardName)
                            fprintf(outputFile, "if (%s5guardEv && %s6enableEv) then begin\n", guardName, guardName);
                    }
                    already_printed_header = 1;
                    fprintf(outputFile, "        %s\n", vout);
                }
            }
            }
            if (trace_translate)
                printf("\n");
            ins = next_ins;
        }
    }
    if (generate == 2)
        fprintf(outputFile, "}\n\n");
    if (guardName && already_printed_header)
        fprintf(outputFile, "end;\n");
}

/*
 * Symbolically run through all rules, running either preprocessing or
 * generating verilog.
 */
static void processRules(Function ***modp, int generate, FILE *outputFile)
{
    int ModuleRfirst= lookup_field("class.Module", "rfirst")/sizeof(uint64_t);
    int ModuleNext  = lookup_field("class.Module", "next")/sizeof(uint64_t);
    int RuleNext    = lookup_field("class.Rule", "next")/sizeof(uint64_t);

    // Walk the rule lists for all modules, generating work items
    while (modp) {                   // loop through all modules
        printf("Module %p: rfirst %p next %p\n", modp, modp[ModuleRfirst], modp[ModuleNext]);
        Function ***rulep = (Function ***)modp[ModuleRfirst];        // Module.rfirst
        while (rulep) {                      // loop through all rules for module
            printf("Rule %p: next %p\n", rulep, rulep[RuleNext]);
            static std::string method[] = { "update", "guard", ""};
            std::string *p = method;
            do {
                vtablework.push_back(VTABLE_WORK(rulep[0][lookup_method("class.Rule", *p)], (Function ***)rulep));
            } while (*++p != "");
            rulep = (Function ***)rulep[RuleNext];           // Rule.next
        }
        modp = (Function ***)modp[ModuleNext]; // Module.next
    }

    // Walk list of work items, generating code
    while (vtablework.begin() != vtablework.end()) {
        processFunction(*vtablework.begin(), generate, outputFile);
        vtablework.pop_front();
    }
}

char GeneratePass::ID = 0;
bool GeneratePass::runOnModule(Module &Mod)
{
    std::string ErrorMsg;
    // preprocessing dwarf debuf info before running anything
    NamedMDNode *CU_Nodes = Mod.getNamedMetadata("llvm.dbg.cu");
    if (CU_Nodes)
        process_metadata(CU_Nodes);

    EngineBuilder builder(&Mod);
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

    Function **** modfirst = (Function ****)EE->getPointerToGlobal(Mod.getNamedValue("_ZN6Module5firstE"));
    EntryFn = Mod.getFunction("main");
    if (!EntryFn || !modfirst) {
        printf("'main' function not found in module.\n");
        exit(1);
    }

    *modfirst = NULL;       // init the Module list before calling constructors
    // run Constructors
    EE->runStaticConstructorsDestructors(false);

    // Construct the address -> symbolic name map using dwarf debug info
    constructAddressMap(CU_Nodes);

    // Preprocess the body rules, creating shadow variables and moving items to guard() and update()
    processRules(*modfirst, 0, outputFile);

    // package together referenced classes
    createClassInstances();

    // Generating code for all rules
    processRules(*modfirst, 1, outputFile);

    // Generate cpp code
    generateCppData(Out, Mod);

    // Generating code for all rules
    processRules(*modfirst, 2, Out);

    generateCppHeader(Mod, OutHeader);
    return false;
}
