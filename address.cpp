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
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

#include "declarations.h"

static int trace_malloc;// = 1;
static int trace_fixup;// = 1;
static int trace_mapt;// = 1;
static int trace_hoist;// = 1;

typedef  struct {
    void *p;
    size_t size;
    Type *type;
    const StructType *STy;
} MEMORY_REGION;

typedef struct {
    const void *addr;
    const void *type;
} MAPSEEN_TYPE;

struct MAPSEENcomp {
    bool operator() (const MAPSEEN_TYPE& lhs, const MAPSEEN_TYPE& rhs) const {
        if (lhs.addr < rhs.addr)
            return true;
        if (lhs.addr > rhs.addr)
            return false;
        return lhs.type < rhs.type;
    }
};

#define GIANT_SIZE 1024
static std::map<MAPSEEN_TYPE, int, MAPSEENcomp> addressTypeAlreadyProcessed;
static std::list<MEMORY_REGION> memoryRegion;

/*
 * Allocated memory region management
 */
extern "C" void *llvm_translate_malloc(size_t size, Type *type, const StructType *STy)
{
    size_t newsize = size * 2 + MAX_BASIC_BLOCK_FLAGS * sizeof(int) + GIANT_SIZE;
    void *ptr = malloc(newsize);
    memset(ptr, 0x5a, newsize);
    if (trace_malloc)
        printf("[%s:%d] %ld = %p type %p sty %p\n", __FUNCTION__, __LINE__, size, ptr, type, STy);
    memoryRegion.push_back(MEMORY_REGION{ptr, newsize, type, STy});
    return ptr;
}

static void recursiveDelete(Value *V) //nee: RecursivelyDeleteTriviallyDeadInstructions
{
    Instruction *I = dyn_cast<Instruction>(V);
    if (!I)
        return;
    for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i) {
        Value *OpV = I->getOperand(i);
        I->setOperand(i, nullptr);
        if (OpV->use_empty())
            recursiveDelete(OpV);
    }
    I->eraseFromParent();
}

static void call2runOnFunction(Function *currentFunction, Function &F)
{
    bool changed = false;
    Module *Mod = F.getParent();
    std::string fname = F.getName();
    const StructType *callingSTy = findThisArgumentType(F.getType());
//printf("CallProcessPass2: %s\n", fname.c_str());
    for (auto BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
        for (auto II = BB->begin(), IE = BB->end(); II != IE;) {
            auto PI = std::next(BasicBlock::iterator(II));
            int opcode = II->getOpcode();
            switch (opcode) {
            case Instruction::Alloca: {
                Value *retv = (Value *)II;
                std::string name = II->getName();
                int ind = name.find("block");
//printf("       ALLOCA %s;", name.c_str());
                if (II->hasName() && ind == -1 && endswith(name, ".addr")) {
                    Value *newt = NULL;
                    auto PN = PI;
                    while (PN != IE) {
                        auto PNN = std::next(BasicBlock::iterator(PN));
                        if (PN->getOpcode() == Instruction::Store && retv == PN->getOperand(1)) {
                            newt = PN->getOperand(0); // Remember value we were storing in temp
                            if (PI == PN)
                                PI = PNN;
                            PN->eraseFromParent(); // delete Store instruction
                        }
                        else if (PN->getOpcode() == Instruction::Load && retv == PN->getOperand(0)) {
                            PN->replaceAllUsesWith(newt); // replace with stored value
                            if (PI == PN)
                                PI = PNN;
                            PN->eraseFromParent(); // delete Load instruction
                        }
                        PN = PNN;
                    }
//printf("del1");
                    II->eraseFromParent(); // delete Alloca instruction
                    changed = true;
                }
//printf("\n");
                break;
                }
            case Instruction::Call: {
                CallInst *ICL = dyn_cast<CallInst>(II);
                Value *callV = ICL->getCalledValue();
                Function *func = dyn_cast<Function>(callV);
                if (Instruction *oldOp = dyn_cast<Instruction>(callV)) {
                    func = dyn_cast_or_null<Function>(Mod->getNamedValue(printOperand(callV, false)));
                    if (!func) {
                        printf("%s: %s not an instantiable call!!!!\n", __FUNCTION__, currentFunction->getName().str().c_str());
                        currentFunction->dump();
                        exit(-1);
                    }
                    II->setOperand(II->getNumOperands()-1, func);
                    recursiveDelete(oldOp);
                }
                const StructType *STy = findThisArgumentType(func->getType());
                //printf("%s: %s CALLS %s cSTy %p STy %p\n", __FUNCTION__, fname.c_str(), func->getName().str().c_str(), callingSTy, STy);
                if (func && callingSTy == STy) {
                    fprintf(stdout,"callProcess: cName %s single!!!!\n", func->getName().str().c_str());
                    call2runOnFunction(currentFunction, *func);
                    InlineFunctionInfo IFI;
                    InlineFunction(ICL, IFI, false);
                    changed = true;
                }
                break;
                }
            };
            II = PI;
        }
    }
}

static void addGuard(Instruction *argI, int RDYName, Function *currentFunction)
{
    Function *parentRDYName = ruleRDYFunction[currentFunction];
    if (!parentRDYName || RDYName < 0)
        return;
    TerminatorInst *TI = parentRDYName->begin()->getTerminator();
    Instruction *newI = copyFunction(TI, argI, RDYName, Type::getInt1Ty(TI->getContext()));
    if (CallInst *nc = dyn_cast<CallInst>(newI))
        nc->addAttribute(AttributeSet::ReturnIndex, Attribute::ZExt);
    Value *cond = TI->getOperand(0);
    const ConstantInt *CI = dyn_cast<ConstantInt>(cond);
    if (CI && CI->getType()->isIntegerTy(1) && CI->getZExtValue())
        TI->setOperand(0, newI);
    else {
        // 'And' return value into condition
        Instruction *newBool = BinaryOperator::Create(Instruction::And, newI, newI, "newand", TI);
        cond->replaceAllUsesWith(newBool);
        // we must set this after the 'replaceAllUsesWith'
        newBool->setOperand(0, cond);
    }
}

// Preprocess the body rules, creating shadow variables and moving items to RDY() and ENA()
// Walk list of work items, cleaning up function references and adding to vtableWork
static void processPromote(Function *currentFunction)
{
    Module *Mod = currentFunction->getParent();
    std::string mName = getMethodName(currentFunction->getName());
    int skip = 1;
    for (auto AI = currentFunction->arg_begin(), AE = currentFunction->arg_end(); AI != AE; ++AI) {
        if (!skip)
            AI->setName(mName + "_" + AI->getName());
        skip = 0;
    }
    for (auto BI = currentFunction->begin(), BE = currentFunction->end(); BI != BE; ++BI) {
        for (auto II = BI->begin(), IE = BI->end(); II != IE;) {
            auto INEXT = std::next(BasicBlock::iterator(II));
            if (CallInst *ICL = dyn_cast<CallInst>(II)) {
                Value *callV = ICL->getCalledValue();
                Function *func = dyn_cast<Function>(callV);
                if (trace_hoist)
                    printf("HOIST: CALLER %s calling '%s'\n", currentFunction->getName().str().c_str(), func->getName().str().c_str());
                if (func->isDeclaration() && func->getName() == "_Z14PIPELINEMARKER") {
                    /* for now, just remove the Call.  Later we will push processing of II->getOperand(0) into another block */
                    std::string Fname = currentFunction->getName().str();
                    std::string otherName = Fname.substr(0, Fname.length() - 8) + "2" + "3ENAEv";
                    Function *otherBody = Mod->getFunction(otherName);
                    TerminatorInst *TI = otherBody->begin()->getTerminator();
                    prepareClone(TI, II);
                    Instruction *IT = dyn_cast<Instruction>(II->getOperand(1));
                    Instruction *IC = dyn_cast<Instruction>(II->getOperand(0));
                    Instruction *newIC = cloneTree(IC, TI);
                    Instruction *newIT = cloneTree(IT, TI);
                    printf("[%s:%d] other %s %p\n", __FUNCTION__, __LINE__, otherName.c_str(), otherBody);
                    IRBuilder<> builder(TI->getParent());
                    builder.SetInsertPoint(TI);
                    builder.CreateStore(newIC, newIT);
                    IRBuilder<> oldbuilder(II->getParent());
                    oldbuilder.SetInsertPoint(II);
                    Value *newLoad = oldbuilder.CreateLoad(IT);
                    II->replaceAllUsesWith(newLoad);
                    II->eraseFromParent();
                    return;
                }
                ClassMethodTable *table = classCreate[findThisArgumentType(func->getType())];
                addGuard(II, vtableFind(table, getMethodName(func->getName()) + "__RDY"), currentFunction);
            }
            II = INEXT;
        }
    }
}

static void pushWork(Function *func)
{
    ClassMethodTable *table = classCreate[findThisArgumentType(func->getType())];
    table->method[getMethodName(func->getName())] = func;
    vtableWork.push_back(func);
    // inline intra-class method call bodies
    call2runOnFunction(func, *func);
    // promote guards from contained calls to be guards for this function
    processPromote(func);
}
static void pushPair(Function *enaFunc, Function *rdyFunc)
{
    ruleRDYFunction[enaFunc] = rdyFunc; // must be before pushWork() calls
    pushWork(enaFunc);
    pushWork(rdyFunc); // must be after 'ENA', since hoisting copies guards
}

static Function *fixupFunction(std::string methodName, Function *func)
{
    Function *fnew = NULL;
    ValueToValueMapTy VMap;
    std::list<Instruction *> moveList;
    for (auto BB = func->begin(), BE = func->end(); BB != BE; ++BB) {
        for (auto II = BB->begin(), IE = BB->end(); II != IE; ) {
            BasicBlock::iterator PI = std::next(BasicBlock::iterator(II));
            switch (II->getOpcode()) {
            case Instruction::Load:
                if (II->getName() == "this") {
                    PointerType *PTy = dyn_cast<PointerType>(II->getType());
                    const StructType *STy = dyn_cast<StructType>(PTy->getElementType());
                    std::string className = STy->getName().substr(6);
                    Type *Params[] = {PTy};
                    fnew = Function::Create(FunctionType::get(func->getReturnType(),
                        ArrayRef<Type*>(Params, 1), false), GlobalValue::LinkOnceODRLinkage,
                        "_ZN" + utostr(className.length()) + className
                              + utostr(methodName.length()) + methodName + "Ev",
                        func->getParent());
                    fnew->arg_begin()->setName("this");
                    Argument *newArg = new Argument(PTy, "JJJthis", func);
                    II->replaceAllUsesWith(newArg);
                    VMap[newArg] = fnew->arg_begin();
                    recursiveDelete(II);
                    func->getArgumentList().pop_front(); // remove original argument
                }
                else if (II->use_empty())
                    recursiveDelete(II);
                break;
            case Instruction::Store: {
                std::string name = II->getOperand(0)->getName();
                if (name == ".block_descriptor" || name == "block")
                    recursiveDelete(II);
                break;
                }
            case Instruction::Call:
                if (II->getType() == Type::getVoidTy(II->getParent()->getContext()))
                    moveList.push_back(II); // move all Action calls to end of basic block
                break;
            }
            II = PI;
        }
    }
    for (auto item: moveList)
        item->moveBefore(item->getParent()->getTerminator());
    SmallVector<ReturnInst*, 8> Returns;  // Ignore returns cloned.
    CloneFunctionInto(fnew, func, VMap, false, Returns, "", nullptr);
    if (trace_fixup) {
        printf("[%s:%d] AFTER method %s\n", __FUNCTION__, __LINE__, methodName.c_str());
        fnew->dump();
    }
    func->setName("unused_block_function");
    return fnew;
}

extern "C" void addBaseRule(void *thisp, const char *name, Function **RDY, Function **ENA)
{
    Function *enaFunc = fixupFunction(name, ENA[2]);
    Function *rdyFunc = fixupFunction(std::string(name) + "__RDY", RDY[2]);
    ClassMethodTable *table = classCreate[findThisArgumentType(rdyFunc->getType())];
    table->rules.push_back(name);
    pushPair(enaFunc, rdyFunc);
}

#if 0
static Function *addFunction(std::string name, ClassMethodTable *table)
{
    std::string lname = lookupMethodName(table, vtableFind(table, name));
    if (lname == "") {
        printf("ExportSymbol: could not find method %s\n", name.c_str());
        exit(-1);
    }
    Function *func = EE->FindFunctionNamed(lname.c_str());
    return func;
}
#endif

extern "C" void exportSymbol(void *thisp, const char *name, StructType *STy)
{
#if 0
    ClassMethodTable *table = classCreate[STy];
    Function *enaFunc = addFunction(name, table);
    Function *rdyFunc = addFunction(std::string(name) + "__RDY", table); // must be after 'ENA'
    //pushPair(enaFunc, rdyFunc);
#endif
}

static void dumpMemoryRegions(int arg)
{
    printf("%s: %d\n", __FUNCTION__, arg);
    for (MEMORY_REGION info : memoryRegion) {
        const GlobalValue *g = EE->getGlobalValueAtAddress(info.p);
        std::string gname;
        if (g)
            gname = g->getName();
        printf("%p %s", info.p, gname.c_str());
        if (info.STy)
            printf(" STy %s", info.STy->getName().str().c_str());
        printf("\n");
        if (info.type)
            info.type->dump();
        long size = info.size;
        if (size > GIANT_SIZE) {
           size -= GIANT_SIZE;
           size -= 10 * sizeof(int);
           size = size/2;
        }
        size += 16;
        memdumpl((unsigned char *)info.p, size, "data");
    }
}

int validateAddress(int arg, void *p)
{
    for (MEMORY_REGION info : memoryRegion) {
        if (p >= info.p && (size_t)p < ((size_t)info.p + info.size))
            return 0;
    }
    printf("%s: %d address validation failed %p\n", __FUNCTION__, arg, p);
    for (MEMORY_REGION info : memoryRegion) {
        const GlobalValue *g = EE->getGlobalValueAtAddress(info.p);
        const char *cp = "";
        if (g)
            cp = g->getName().str().c_str();
        printf("%p %s size 0x%lx\n", info.p, cp, info.size);
    }
    return 1;
}

int derivedStruct(const StructType *STyA, const StructType *STyB)
{
    int Idx = 0;
    if (STyA && STyB)
    for (auto I = STyA->element_begin(), E = STyA->element_end(); I != E; ++I, Idx++)
        if (fieldName(STyA, Idx) == "" && dyn_cast<StructType>(*I) && *I == STyB)
            return 1;
    return 0;
}
static int checkDerived(const Type *A, const Type *B)
{
    if (const PointerType *PTyA = cast<PointerType>(A))
    if (const PointerType *PTyB = cast<PointerType>(B))
        return derivedStruct(dyn_cast<StructType>(PTyA->getElementType()),
                             dyn_cast<StructType>(PTyB->getElementType()));
    return 0;
}

static void myReplaceAllUsesWith(Value *Old, Value *New)
{
  //assert(New->getType() == Old->getType() && "replaceAllUses of value with new value of different type!");
  // Notify all ValueHandles (if present) that Old value is going away.
  //if (Old->HasValueHandle)
    //ValueHandleBase::ValueIsRAUWd(Old, New);
  if (Old->isUsedByMetadata())
    ValueAsMetadata::handleRAUW(Old, New);
  while (!Old->use_empty()) {
    Use &U = *Old->use_begin();
    // Must handle Constants specially, we cannot call replaceUsesOfWith on a
    // constant because they are uniqued.
    if (auto *C = dyn_cast<Constant>(U.getUser())) {
      if (!isa<GlobalValue>(C)) {
        C->handleOperandChange(Old, New, &U);
        continue;
      }
    }
    U.set(New);
  }
  if (BasicBlock *BB = dyn_cast<BasicBlock>(Old))
    BB->replaceSuccessorsPhiUsesWith(cast<BasicBlock>(New));
}

static void inlineReferences(Module *Mod, const StructType *STy, uint64_t Idx, Type *newType)
{
    for (auto FB = Mod->begin(), FE = Mod->end(); FB != FE; ++FB) {
        if (FB->getName().substr(0, 21) != "unused_block_function")
        for (auto BB = FB->begin(), BE = FB->end(); BB != BE; ++BB)
            for (auto II = BB->begin(), IE = BB->end(); II != IE; ) {
                BasicBlock::iterator PI = std::next(BasicBlock::iterator(II));
                if (LoadInst *IL = dyn_cast<LoadInst>(II)) {
                if (GetElementPtrInst *IG = dyn_cast<GetElementPtrInst>(IL->getOperand(0))) {
                    gep_type_iterator I = gep_type_begin(IG), E = gep_type_end(IG);
                    Constant *FirstOp = dyn_cast<Constant>(I.getOperand());
                    if (I++ != E && FirstOp && FirstOp->isNullValue()
                     && I != E && STy == dyn_cast<StructType>(*I))
                        if (const ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand()))
                            if (CI->getZExtValue() == Idx) {
                                 IG->mutateType(newType);
                                 myReplaceAllUsesWith(IL, IG);
                                 IL->eraseFromParent();
                            }
                    }
                }
                II = PI;
            }
    }
}

static void mapType(Module *Mod, char *addr, Type *Ty, std::string aname)
{
    const DataLayout *TD = EE->getDataLayout();
    if (!addr || addr == BOGUS_POINTER
     || addressTypeAlreadyProcessed[MAPSEEN_TYPE{addr, Ty}])
        return;
    addressTypeAlreadyProcessed[MAPSEEN_TYPE{addr, Ty}] = 1;
    //printf("[%s:%d] addr %p TID %d Ty %p name %s\n", __FUNCTION__, __LINE__, addr, Ty->getTypeID(), Ty, aname.c_str());
    if (validateAddress(3010, addr))
        printf("[%s:%d] baddd\n", __FUNCTION__, __LINE__);
    switch (Ty->getTypeID()) {
    case Type::StructTyID: {
        StructType *STy = cast<StructType>(Ty);
        const StructLayout *SLO = TD->getStructLayout(STy);
        int Idx = 0;
        for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
            std::string fname = fieldName(STy, Idx);
            char *eaddr = addr + SLO->getElementOffset(Idx);
            Type *element = *I;
            if (PointerType *PTy = dyn_cast<PointerType>(element)) {
                void *p = *(char **)eaddr;
                /* Look up destination address in allocated regions, to see
                 * what if its datatype is a derived type from the pointer
                 * target type.  If so, replace pointer base type.
                 */
                for (MEMORY_REGION info : memoryRegion)
                    if (p >= info.p && (size_t)p < ((size_t)info.p + info.size)
                     && checkDerived(info.type, PTy)) {
                        if (!classCreate[STy])
                            classCreate[STy] = new ClassMethodTable;
                        classCreate[STy]->replaceType[Idx] = info.type;
                        if (STy == info.STy) {
                            classCreate[STy]->allocateLocally[Idx] = true;
                            inlineReferences(Mod, STy, Idx, info.type);
                            classCreate[STy]->replaceType[Idx] = cast<PointerType>(info.type)->getElementType();
                        }
                    }
            }
            if (fname != "")
                mapType(Mod, eaddr, element, aname + "$$" + fname);
            else if (dyn_cast<StructType>(element))
                mapType(Mod, eaddr, element, aname);
        }
        break;
        }
    case Type::PointerTyID:
        mapType(Mod, *(char **)addr, cast<PointerType>(Ty)->getElementType(), aname);
        break;
    default:
        break;
    }
}

std::string fieldName(const StructType *STy, uint64_t ind)
{
    unsigned int subs = 0;
    int idx = ind;
    while (idx-- > 0) {
        while (subs < STy->structFieldMap.length() && STy->structFieldMap[subs] != ',')
            subs++;
        subs++;
    }
    if (subs >= STy->structFieldMap.length())
        return "";
    std::string ret = STy->structFieldMap.substr(subs);
    idx = ret.find(',');
    if (idx >= 0)
        ret = ret.substr(0,idx);
    return ret;
}

int vtableFind(const ClassMethodTable *table, std::string name)
{
    for (unsigned int i = 0; table && i < table->vtableCount; i++)
        if (getMethodName(table->vtable[i]->getName()) == name)
            return i;
    return -1;
}

std::string lookupMethodName(const ClassMethodTable *table, int ind)
{
    if (table && ind >= 0 && ind < (int)table->vtableCount)
        return table->vtable[ind]->getName();
    return "";
}

/*
 * Map calls to 'new()' and 'malloc()' in constructors to call 'llvm_translate_malloc'.
 * This enables llvm-translate to easily maintain a list of valid memory regions
 * during processing.
 */
static void callMemrunOnFunction(CallInst *II)
{
    Module *Mod = II->getParent()->getParent()->getParent();
    Value *called = II->getOperand(II->getNumOperands()-1);
    const Function *CF = dyn_cast<Function>(called);
    Instruction *PI = II->user_back();
    unsigned long tparam = 0;
    unsigned long styparam = (unsigned long)findThisArgumentType(II->getParent()->getParent()->getType());
    //printf("[%s:%d] %s calling %s styparam %lx\n", __FUNCTION__, __LINE__, II->getParent()->getParent()->getName().str().c_str(), CF->getName().str().c_str(), styparam);
    if (PI->getOpcode() == Instruction::BitCast && &*II == PI->getOperand(0))
        tparam = (unsigned long)PI->getType();
    Type *Params[] = {Type::getInt64Ty(Mod->getContext()),
        Type::getInt64Ty(Mod->getContext()), Type::getInt64Ty(Mod->getContext())};
    FunctionType *fty = FunctionType::get(Type::getInt8PtrTy(Mod->getContext()),
        ArrayRef<Type*>(Params, 3), false);
    Function *F = dyn_cast<Function>(Mod->getOrInsertFunction("llvm_translate_malloc", fty));
    F->setCallingConv(CF->getCallingConv());
    F->setDoesNotAlias(0);
    F->setAttributes(CF->getAttributes());
    IRBuilder<> builder(II->getParent());
    builder.SetInsertPoint(II);
    II->replaceAllUsesWith(builder.CreateCall(F,
       {II->getOperand(0), builder.getInt64(tparam), builder.getInt64(styparam)},
       "llvm_translate_malloc"));
    II->eraseFromParent();
}

static std::map<const StructType *, int> STyList;
typedef Function *FPTR;
static void processStruct(const StructType *STy)
{
    if (!STyList[STy])
        return;
    STyList[STy] = 0;
    ClassMethodTable *table = classCreate[STy];
    std::map<std::string, Function *>methodMap;

    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I) {
        if (PointerType *PTy = dyn_cast<PointerType>(*I))
        if (const StructType *STy = dyn_cast<StructType>(PTy->getElementType()))
            processStruct(STy);
    }
    for (unsigned int i = 0; i < table->vtableCount; i++) {
         Function *func = table->vtable[i];
         methodMap[getMethodName(func->getName())] = func;
    }
    for (auto item: methodMap)
        if (endswith(item.first, "__RDY"))
            pushPair(methodMap[item.first.substr(0, item.first.length() - 5)], item.second);
    for (auto info : classCreate)
        if (const StructType *iSTy = info.first)
        if (derivedStruct(iSTy, STy))
            processStruct(iSTy);
}
void preprocessModule(Module *Mod)
{
    STyList.clear();
    for (auto FB = Mod->begin(), FE = Mod->end(); FB != FE; ++FB)
        findThisArgumentType(FB->getType());
    for (auto MI = Mod->global_begin(), ME = Mod->global_end(); MI != ME; MI++) {
        std::string name = MI->getName();
        const ConstantArray *CA;
        int status;
        const char *ret = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
        if (ret && !strncmp(ret, "vtable for ", 11)
         && MI->hasInitializer() && (CA = dyn_cast<ConstantArray>(MI->getInitializer()))) {
            uint64_t numElements = cast<ArrayType>(MI->getType()->getElementType())->getNumElements();
            printf("[%s:%d] global %s ret %s\n", __FUNCTION__, __LINE__, name.c_str(), ret);
            for (auto CI = CA->op_begin(), CE = CA->op_end(); CI != CE; CI++) {
                if (ConstantExpr *vinit = dyn_cast<ConstantExpr>((*CI)))
                if (vinit->getOpcode() == Instruction::BitCast)
                if (Function *func = dyn_cast<Function>(vinit->getOperand(0)))
                if (const StructType *STy = findThisArgumentType(func->getType())) {
                    STyList[STy] = 1;
                    ClassMethodTable *table = classCreate[STy];
                    if (!table->vtable)
                        table->vtable = new FPTR[numElements];
                    table->vtable[table->vtableCount++] = func;
                }
            }
        }
    }
    for (auto item: STyList)
         processStruct(item.first);

    // remove dwarf info, if it was compiled in
    const char *delete_names[] = { "llvm.dbg.declare", "llvm.dbg.value", "atexit", NULL};
    const char **p = delete_names;
    while(*p)
        if (Function *Declare = Mod->getFunction(*p++)) {
            while (!Declare->use_empty()) {
                CallInst *CI = cast<CallInst>(Declare->user_back());
                CI->eraseFromParent();
            }
            Declare->eraseFromParent();
        }

    // before running constructors, remap all calls to 'malloc' and 'new' to our runtime.
    const char *malloc_names[] = { "_Znwm", "malloc", NULL};
    p = malloc_names;
    while(*p)
        if (Function *Declare = Mod->getFunction(*p++))
            for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; I++)
                callMemrunOnFunction(cast<CallInst>(*I));

    // Add StructType parameter to exportSymbol calls
    if (Function *Declare = Mod->getFunction("exportSymbol"))
        for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; I++) {
            CallInst *II = cast<CallInst>(*I);
            II->setOperand(2, ConstantInt::get(Type::getInt64Ty(II->getContext()),
                (unsigned long)findThisArgumentType(II->getParent()->getParent()->getType())));
        }
}

/*
 * Starting from all toplevel global variables, construct symbolic names for
 * all reachable addresses
 */
void constructAddressMap(Module *Mod)
{
    for (auto MI = Mod->global_begin(), ME = Mod->global_end(); MI != ME; MI++) {
        std::string name = MI->getName();
        if ((name.length() < 4 || name.substr(0,4) != ".str")
         && (name.length() < 18 || name.substr(0,18) != "__block_descriptor")) {
            void *addr = EE->getPointerToGlobal(MI);
            Type *Ty = MI->getType()->getElementType();
            memoryRegion.push_back(MEMORY_REGION{addr,
                EE->getDataLayout()->getTypeAllocSize(Ty), MI->getType(), NULL});
            mapType(Mod, (char *)addr, Ty, name);
        }
    }
    if (trace_mapt)
        dumpMemoryRegions(4010);
}
