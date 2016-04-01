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

static int trace_map;//= 1;
static int trace_hoist;//= 1;
static int trace_dump_malloc;//= 1;

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
typedef std::map<std::string, Function *>MethodMapType;

std::list<Function *> fixupFuncList;
static std::map<MAPSEEN_TYPE, int, MAPSEENcomp> addressTypeAlreadyProcessed;

static struct {
    std::map<const BasicBlock *, Value *> val;
} blockCondition[2];
void setCondition(BasicBlock *bb, int invert, Value *val)
{
    blockCondition[invert].val[bb] = val;
}

Value *getCondition(BasicBlock *bb, int invert)
{
    if (Value *val = blockCondition[invert].val[bb])
        return val;
    if (Instruction *val = dyn_cast_or_null<Instruction>(blockCondition[1-invert].val[bb])) {
        BasicBlock *prevBB = val->getParent();
        Instruction *TI = bb->getTerminator();
        if (prevBB != bb)
            val = cloneTree(val, TI);
        IRBuilder<> builder(bb);
        builder.SetInsertPoint(TI);
        setCondition(bb, invert, BinaryOperator::Create(Instruction::Xor,
           val, builder.getInt1(1), "invertCond", TI));
        return blockCondition[invert].val[bb];
    }
    return NULL;
}

/*
 * Dump all statically allocated and malloc'ed data areas
 */
void dumpMemoryRegions(int arg)
{
    printf("%s: %d\n", __FUNCTION__, arg);
    for (MEMORY_REGION info : memoryRegion) {
        const GlobalValue *g = EE->getGlobalValueAtAddress(info.p);
        std::string gname;
        if (g)
            gname = g->getName();
        printf("Region addr %p name '%s'", info.p, gname.c_str());
        if (info.STy)
            printf(" infoSTy %s", info.STy->getName().str().c_str());
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

/*
 * Verify that an address lies within a valid user data area.
 */
int validateAddress(int arg, void *p)
{
    static int once;
    for (MEMORY_REGION info : memoryRegion) {
        if (p >= info.p && (size_t)p < ((size_t)info.p + info.size))
            return 0;
    }
    printf("%s: %d address validation failed %p\n", __FUNCTION__, arg, p);
    if (!once)
    for (MEMORY_REGION info : memoryRegion) {
        const GlobalValue *g = EE->getGlobalValueAtAddress(info.p);
        const char *cp = "";
        if (g)
            cp = g->getName().str().c_str();
        printf("%p %s size 0x%lx\n", info.p, cp, info.size);
    }
    once = 1;
    return 1;
}

static std::list<Instruction *> preCopy;
static Instruction *defactorTree(Instruction *insertPoint, Instruction *top, Instruction *arg)
{
    if (const PHINode *PN = dyn_cast<PHINode>(arg)) {
        for (unsigned opIndex = 1, Eop = PN->getNumIncomingValues(); opIndex < Eop; opIndex++) {
            prepareReplace(arg, PN->getIncomingValue(opIndex));
            preCopy.push_back(cloneTree(top, insertPoint));
        }
        return dyn_cast<Instruction>(PN->getIncomingValue(0));
    }
    else
        for (unsigned int i = 0; i < arg->getNumOperands(); i++) {
            if (Instruction *param = dyn_cast<Instruction>(arg->getOperand(i))) {
                Instruction *ret = defactorTree(insertPoint, top, param);
                if (ret) {
                    arg->setOperand(i, ret);
                    recursiveDelete(param);
                }
            }
        }
    return NULL;
}
static Instruction *copyFunction(Instruction *insertPoint, const Instruction *I, Function *func)
{
    std::list<Instruction *> postCopy;
    preCopy.clear();
    Instruction *retItem = NULL;
    prepareClone(insertPoint, I);
    int ind = 0;
    if (const CallInst *CI = dyn_cast<CallInst>(I))
        ind = CI->hasStructRetAttr();
    Value *new_thisp = I->getOperand(ind);
    if (Instruction *orig_thisp = dyn_cast<Instruction>(new_thisp))
        new_thisp = cloneTree(orig_thisp, insertPoint);
    preCopy.push_back(dyn_cast<Instruction>(new_thisp));
    for (auto item: preCopy) {
        defactorTree(insertPoint, item, item);
        postCopy.push_back(item);
    }
    for (auto item: postCopy) {
        Value *Params[] = {item};
        IRBuilder<> builder(insertPoint->getParent());
        builder.SetInsertPoint(insertPoint);
        CallInst *newCall = builder.CreateCall(func, ArrayRef<Value*>(Params, 1));
        newCall->addAttribute(AttributeSet::ReturnIndex, Attribute::ZExt);
        if (retItem)
            retItem = BinaryOperator::Create(Instruction::And, retItem, newCall, "newand", insertPoint);
        else
            retItem = newCall;
    }
    return retItem;
}

/*
 * Add another condition to a guard function.
 */
static void addGuard(Instruction *argI, Function *func, Function *currentFunction)
{
    /* get my function's guard function */
    Function *parentRDYName = ruleRDYFunction[currentFunction];
    if (!parentRDYName || !func)
        return;
    TerminatorInst *TI = parentRDYName->begin()->getTerminator();
    /* make a call to the guard being promoted */
    Instruction *newI = copyFunction(TI, argI, func);
    /* if the promoted guard is in an 'if' block, 'or' with inverted condition of block */
    if (Instruction *bcond = dyn_cast_or_null<Instruction>(getCondition(argI->getParent(), 1))) // get inverted condition, if any
        newI = BinaryOperator::Create(Instruction::Or, newI, cloneTree(bcond,TI), "newor", TI);
    /* get existing return value from my function's guard */
    Value *cond = TI->getOperand(0);
    const ConstantInt *CI = dyn_cast<ConstantInt>(cond);
    if (CI && CI->getType()->isIntegerTy(1) && CI->getZExtValue())
        TI->setOperand(0, newI); /* if it was 'true', just replace it with new expr */
    else { /* else 'and' new expression into guard expression */
        // 'And' return value into condition
        Instruction *newBool = BinaryOperator::Create(Instruction::And, newI, newI, "newand", TI);
        cond->replaceAllUsesWith(newBool);
        // we must set this after the 'replaceAllUsesWith'
        newBool->setOperand(0, cond);
    }
}

// Preprocess the body rules, creating shadow variables and moving items to RDY() and ENA()
static void processPromote(Function *currentFunction)
{
    Module *Mod = currentFunction->getParent();
restart:
    for (auto BI = currentFunction->begin(), BE = currentFunction->end(); BI != BE; BI++) {
        for (auto II = BI->begin(), IE = BI->end(); II != IE;) {
            auto INEXT = std::next(BasicBlock::iterator(II));
            switch (II->getOpcode()) {
            case Instruction::Call: {
                CallInst *ICL = dyn_cast<CallInst>(II);
                Value *callV = ICL->getCalledValue();
                Function *func = dyn_cast<Function>(callV);
                std::string mName = getMethodName(func->getName());
                ClassMethodTable *table = classCreate[findThisArgument(func)];
                if (trace_hoist)
                    printf("HOIST: CALLER %s calling '%s'[%s] table %p\n", currentFunction->getName().str().c_str(), func->getName().str().c_str(), mName.c_str(), table);
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
                std::string rdyName = mName + "__RDY";
                if (table)
                    table->method[mName] = func;  // keep track of all functions that were called, not just ones that were defined
                for (unsigned int i = 0; table && i < table->vtableCount; i++) {
                    Function *calledFunctionGuard = table->vtable[i];
                    if (trace_hoist)
                        printf("HOIST: act %s req %s\n", getMethodName(calledFunctionGuard->getName()).c_str(), rdyName.c_str());
                    if (getMethodName(calledFunctionGuard->getName()) == rdyName) {
                        addGuard(II, calledFunctionGuard, currentFunction);
                        break;
                    }
                }
                break;
                }
            case Instruction::Br: {
                // BUG BUG BUG -> combine the condition for the current block with the getConditions for this instruction
                const BranchInst *BI = dyn_cast<BranchInst>(II);
                if (BI && BI->isConditional()) {
                    std::string cond = printOperand(BI->getCondition(), false);
                    //printf("[%s:%d] condition %s [%p, %p]\n", __FUNCTION__, __LINE__, cond.c_str(), BI->getSuccessor(0), BI->getSuccessor(1));
                    setCondition(BI->getSuccessor(0), 0, BI->getCondition()); // 'true' condition
                    setCondition(BI->getSuccessor(1), 1, BI->getCondition()); // 'inverted' condition
                }
                else if (isa<IndirectBrInst>(II)) {
                    printf("[%s:%d] indirect\n", __FUNCTION__, __LINE__);
                    for (unsigned i = 0, e = II->getNumOperands(); i != e; ++i) {
                        printf("[%d] = %s\n", i, printOperand(II->getOperand(i), false).c_str());
                    }
                }
                else {
                    //printf("[%s:%d] BRUNCOND %p\n", __FUNCTION__, __LINE__, BI->getSuccessor(0));
                }
                break;
                }
            case Instruction::GetElementPtr:
                // Expand out index expression references
                if (II->getNumOperands() == 2)
                if (Instruction *switchIndex = dyn_cast<Instruction>(II->getOperand(1))) {
                    int Values_size = 2;
                    if (Instruction *ins = dyn_cast<Instruction>(II->getOperand(0))) {
                        if (PointerType *PTy = dyn_cast<PointerType>(ins->getOperand(0)->getType()))
                        if (StructType *STy = dyn_cast<StructType>(PTy->getElementType())) {
                            int Idx = 0, eleIndex = -1;
                            if (const ConstantInt *CI = dyn_cast<ConstantInt>(ins->getOperand(2)))
                                eleIndex = CI->getZExtValue();
                            for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++)
                                if (Idx == eleIndex)
                                if (ClassMethodTable *table = classCreate[STy])
                                    if (table->replaceType[Idx]) {
                                        Values_size = table->replaceCount[Idx];
printf("[%s:%d] get dyn size (static not handled) %d\n", __FUNCTION__, __LINE__, Values_size);
if (Values_size < 0 || Values_size > 100) Values_size = 2;
                                    }
                        }
                        ins->getOperand(0)->dump();
                    }
                    II->getOperand(0)->dump();
                    BasicBlock *afterswitchBB = BI->splitBasicBlock(II, "afterswitch");
                    IRBuilder<> afterBuilder(afterswitchBB);
                    // Build Switch instruction in starting block
                    IRBuilder<> startBuilder(BI);
                    startBuilder.SetInsertPoint(BI->getTerminator());
                    BasicBlock *lastCaseBB = BasicBlock::Create(BI->getContext(), "lastcase", currentFunction, afterswitchBB);
                    SwitchInst *switchInst = startBuilder.CreateSwitch(switchIndex, lastCaseBB, Values_size - 1);
                    BI->getTerminator()->eraseFromParent();
                    // Build PHI in end block
                    PHINode *phi = afterBuilder.CreatePHI(II->getType(), Values_size, "phi");
                    // Add all of the 'cases' to the switch instruction.
                    for (int caseIndex = 0; caseIndex < Values_size; ++caseIndex) {
                        ConstantInt *caseInt = startBuilder.getInt64(caseIndex);
                        BasicBlock *caseBB = lastCaseBB;
                        if (caseIndex != Values_size - 1) { // already created a block for 'default'
                            caseBB = BasicBlock::Create(BI->getContext(), "switchcase", currentFunction, afterswitchBB);
                            switchInst->addCase(caseInt, caseBB);
                        }
                        IRBuilder<> cbuilder(caseBB);
                        cbuilder.CreateBr(afterswitchBB);
                        Instruction *val = cloneTree(II, caseBB->getTerminator());
                        val->setOperand(1, caseInt);
                        phi->addIncoming(val, caseBB);
                    }
                    II->replaceAllUsesWith(phi);
                    recursiveDelete(II);
                    goto restart;  // the instruction INEXT is no longer in the block BI
                }
                break;
            }
            II = INEXT;
        }
    }
}

/*
 * Local version of 'ReplaceAllUsesWith'(RAUW) that allows changing the
 * datatype as part of the replcement.
 */
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

/*
 * For pointers in a class that were allocated by methods in the class,
 * allocate them statically as part of the class object itself.
 */
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

static void registerInterface(char *addr, StructType *STy, const char *name)
{
    const DataLayout *TD = EE->getDataLayout();
    const StructLayout *SLO = TD->getStructLayout(STy);
    ClassMethodTable *table = classCreate[STy];
    std::map<std::string, Function *> callMap;
    MethodMapType methodMap;
    int Idx = 0;
    if (trace_pair)
        printf("[%s:%d] addr %p struct %s name %s vtableCount %d\n", __FUNCTION__, __LINE__, addr, STy->getName().str().c_str(), name, table->vtableCount);
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++)
        if (PointerType *PTy = dyn_cast<PointerType>(*I))
        if (PTy->getElementType()->getTypeID() == Type::FunctionTyID) {
            char *eaddr = addr + SLO->getElementOffset(Idx);
            std::string fname = fieldName(STy, Idx);
            Function *func = *(Function **)eaddr;
            callMap[fname] = func;
            if (trace_pair)
                printf("[%s:%d] addr %p callMap[%s] = %p [%p = %s]\n", __FUNCTION__, __LINE__, addr, fname.c_str(), eaddr, func, func ? func->getName().str().c_str() : "none");
        }
    for (unsigned i = 0; i < table->vtableCount; i++) {
        Function *func = table->vtable[i];
        std::string mName = getMethodName(func->getName());
        setSeen(func, mName);
        for (auto BB = func->begin(), BE = func->end(); BB != BE; ++BB)
            for (auto II = BB->begin(), IE = BB->end(); II != IE; II++)
                if (CallInst *ICL = dyn_cast<CallInst>(II)) {
                    std::string sname = printOperand(ICL->getCalledValue(), false);
                    if (sname.substr(0,6) == "this->")
                        sname = sname.substr(6);
                    else if (sname.substr(0,7) == "thisp->")
                        sname = sname.substr(7);
                    Function *calledFunc = callMap[sname];
                    if (trace_pair)
                        printf("[%s:%d] set methodMap [%s] = %p [%s]\n", __FUNCTION__, __LINE__, (name + std::string("$") + mName).c_str(), calledFunc, sname.c_str());
                    if (calledFunc)
                        methodMap[mName] = calledFunc;
                }
    }
    for (auto item: methodMap) {
        if (trace_pair)
            printf("[%s:%d] methodMap %s\n", __FUNCTION__, __LINE__, item.first.c_str());
        if (endswith(item.first, "__RDY")) {
            std::string enaName = item.first.substr(0, item.first.length() - 5);
            Function *enaFunc = methodMap[enaName];
            if (!enaFunc) {
                printf("%s: guarded function not found %s\n", __FUNCTION__, item.first.c_str());
                for (auto item: methodMap)
                    if (item.second)
                        printf("    %s\n", item.first.c_str());
                exit(-1);
            }
            if (trace_pair)
                printf("%s: seen %s pair rdy %s ena %s[%s]\n", __FUNCTION__,
                    pushSeen[enaFunc].c_str(), item.first.c_str(), enaName.c_str(), enaFunc->getName().str().c_str());
            pushPair(enaFunc, enaName, item.second, item.first);
        }
    }
}

/*
 * Check if one StructType inherits another one.
 */
static int derivedStruct(const StructType *STyA, const StructType *STyB)
{
    int Idx = 0;
    if (STyA && STyB) {
        if (STyA == STyB)
            return 1;     // all types are derived from themselves
        //BUG: this only checks 1 level of derived???
        for (auto I = STyA->element_begin(), E = STyA->element_end(); I != E; ++I, Idx++)
            if (fieldName(STyA, Idx) == "" && dyn_cast<StructType>(*I) && *I == STyB)
                return 1;
    }
    return 0;
}

static int checkDerived(const Type *A, const Type *B)
{
    if (const PointerType *PTyA = cast_or_null<PointerType>(A))
    if (const PointerType *PTyB = cast_or_null<PointerType>(B))
        return derivedStruct(dyn_cast<StructType>(PTyA->getElementType()),
                             dyn_cast<StructType>(PTyB->getElementType()));
    return 0;
}

static void mapType(Module *Mod, char *addr, Type *Ty, std::string aname)
{
    const DataLayout *TD = EE->getDataLayout();
    if (!addr || addr == BOGUS_POINTER
     || addressTypeAlreadyProcessed[MAPSEEN_TYPE{addr, Ty}])
        return;
    addressTypeAlreadyProcessed[MAPSEEN_TYPE{addr, Ty}] = 1;
    if (trace_map)
        printf("%s: addr %p TID %d Ty %p name %s\n", __FUNCTION__, addr, Ty->getTypeID(), Ty, aname.c_str());
    if (validateAddress(3010, addr))
        printf("%s: bad addr %p TID %d Ty %p name %s\n", __FUNCTION__, addr, Ty->getTypeID(), Ty, aname.c_str());
    switch (Ty->getTypeID()) {
    case Type::StructTyID: {
        StructType *STy = cast<StructType>(Ty);
        getStructName(STy); // allocate classCreate
        const StructLayout *SLO = TD->getStructLayout(STy);
        ClassMethodTable *table = classCreate[STy];
        int Idx = 0;
        for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
            std::string fname = fieldName(STy, Idx);
            char *eaddr = addr + SLO->getElementOffset(Idx);
            Type *element = *I;
            if (PointerType *PTy = dyn_cast<PointerType>(element)) {
                void *p = *(char **)eaddr;
                int setInterface = fname != "";
                /* Look up destination address in allocated regions, to see
                 * what if its datatype is a derived type from the pointer
                 * target type.  If so, replace pointer base type.
                 */
                if (trace_map)
                    printf("%s: Pointer lookup %p STy %s fname %s\n", __FUNCTION__, p, STy->hasName() ? STy->getName().str().c_str() : "none", fname.c_str());
                static int once;
                for (MEMORY_REGION info : memoryRegion) {
                    if (trace_map && !once)
                        printf("%s: info.p %p info.p+info.size %lx\n", __FUNCTION__, info.p, ((size_t)info.p + info.size));
                    if (p >= info.p && (size_t)p < ((size_t)info.p + info.size)) {
                    if (checkDerived(info.type, PTy)) {
                        if (!classCreate[STy])
                            classCreate[STy] = new ClassMethodTable;
                        classCreate[STy]->STy = STy;
                        classCreate[STy]->replaceType[Idx] = info.type;
                        classCreate[STy]->replaceCount[Idx] = info.vecCount;
                        //if (trace_map)
                            printf("%s: pointerFound %p info.STy %s count %d\n", __FUNCTION__, p, info.STy->getName().str().c_str(), (int)info.vecCount);
                        if (STy == info.STy) {
                            classCreate[STy]->allocateLocally[Idx] = true;
                            inlineReferences(Mod, STy, Idx, info.type);
                            classCreate[STy]->replaceType[Idx] = cast<PointerType>(info.type)->getElementType();
                            setInterface = 0;
                        }
                    }
                    else {
                        if (trace_map)
                            printf("%s: notderived %p info.STy %p %s\n", __FUNCTION__, p, info.STy, info.STy?info.STy->getName().str().c_str():"null");
                    }
                    }
                }
                once = 1;
                if (setInterface)
                    table->interfaces[fname] = element;  // add called interfaces from this module
            }
            if (fname != "") {
                if (StructType *iSTy = dyn_cast<StructType>(element)) {
                    if (iSTy->hasName() && iSTy->getName() == "class.FixedPointV") {
                        int iIdx = 0;
                        const StructLayout *iSLO = TD->getStructLayout(iSTy);
                        uint64_t size = -1;
                        Type *bitc = NULL;
                        for (auto I = iSTy->element_begin(), E = iSTy->element_end(); I != E; ++I, iIdx++) {
                            std::string fname = fieldName(iSTy, iIdx);
                            if (fname == "") {
                                if (bitc)
                                    printf("[%s:%d] already had bitc\n", __FUNCTION__, __LINE__);
                                bitc = *I;
                            }
                            else if (fname == "size" && (*I)->getTypeID() == Type::IntegerTyID)
                                size = *(uint64_t *)(eaddr + iSLO->getElementOffset(iIdx));
                        }
                        printf("[%s:%d] make new FixedPoint isize %lx bitc %p\n", __FUNCTION__, __LINE__, size, bitc);
                        Type *EltTys[] = {bitc, Type::getInt64Ty(Mod->getContext())};
                        StructType *nSTy = StructType::create(EltTys, "newStructName");
                        classCreate[nSTy] = new ClassMethodTable;
                        classCreate[nSTy]->STy = nSTy;
                        classCreate[nSTy]->instance = utostr(size);
                        if (!classCreate[STy]) {
                            classCreate[STy] = new ClassMethodTable;
                            classCreate[STy]->STy = STy;
                        }
                        classCreate[STy]->replaceType[Idx] = nSTy;
                        classCreate[STy]->replaceCount[Idx] = -1;
                    }
                }
                mapType(Mod, eaddr, element, aname + "$$" + fname);
                if (StructType *iSTy = dyn_cast<StructType>(element))
                    registerInterface(eaddr, iSTy, fname.c_str());
            }
            else if (dyn_cast<StructType>(element))
                mapType(Mod, eaddr, element, aname);
        }
        break;
        }
    case Type::PointerTyID: {
        Type *element = cast<PointerType>(Ty)->getElementType();
        if (element->getTypeID() != Type::FunctionTyID)
            mapType(Mod, *(char **)addr, element, aname);
        break;
    }
    default:
        break;
    }
}

/*
 * Starting from all toplevel global variables, construct symbolic names for
 * all reachable addresses
 */
void constructAddressMap(Module *Mod)
{
printf("[%s:%d] start\n", __FUNCTION__, __LINE__);
    for (auto MI = Mod->global_begin(), ME = Mod->global_end(); MI != ME; MI++) {
        void *addr = EE->getPointerToGlobal(MI);
        Type *Ty = MI->getType()->getElementType();
        memoryRegion.push_back(MEMORY_REGION{addr,
            EE->getDataLayout()->getTypeAllocSize(Ty), MI->getType(), NULL});
        mapType(Mod, (char *)addr, Ty, MI->getName());
    }
    // promote guards from contained calls to be guards for this function
    for (auto item: fixupFuncList)
        processPromote(item);

    if (trace_dump_malloc)
        dumpMemoryRegions(4010);
}
