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

static int trace_malloc;//= 1;
static int trace_fixup;//= 1;
static int trace_hoist;//= 1;
static int trace_lookup;//= 1;
static int trace_pair;//= 1;

typedef  struct {
    void *p;
    size_t size;
    Type *type;
    const StructType *STy;
    uint64_t   vecCount;
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
typedef std::map<std::string, Function *>MethodMapType;

#define GIANT_SIZE 1024
std::map<const Function *, std::string> pushSeen;
std::list<Function *> fixupFuncList;
static struct {
    std::map<const BasicBlock *, Value *> val;
} blockCondition[2];
static std::map<MAPSEEN_TYPE, int, MAPSEENcomp> addressTypeAlreadyProcessed;
static std::list<MEMORY_REGION> memoryRegion;

/*
 * Allocated memory region management
 */
extern "C" void *llvm_translate_malloc(size_t size, Type *type, const StructType *STy, uint64_t vecCount)
{
    size_t newsize = size * 2 + MAX_BASIC_BLOCK_FLAGS * sizeof(int) + GIANT_SIZE;
    void *ptr = malloc(newsize);
    memset(ptr, 0x5a, newsize);
    if (trace_malloc)
        printf("[%s:%d] %ld = %p type %p sty %p vecCount %ld\n", __FUNCTION__, __LINE__, size, ptr, type, STy, vecCount);
    memoryRegion.push_back(MEMORY_REGION{ptr, newsize, type, STy, vecCount});
    return ptr;
}

/*
 * Recursively delete an Instruction and operands (if they become unused)
 */
static void recursiveDelete(Value *V) //nee: RecursivelyDeleteTriviallyDeadInstructions
{
    Instruction *I = dyn_cast<Instruction>(V);
    if (!I)
        return;
    for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i) {
        Value *OpV = I->getOperand(i);
        I->setOperand(i, nullptr);
        if (OpV && OpV->use_empty())
            recursiveDelete(OpV);
    }
    I->eraseFromParent();
}

/*
 * Remove Alloca items inserted by clang as part of dwarf debug support.
 * (the 'this' pointer was copied into a stack temp rather than being
 * referenced directly from the parameter)
 * Context: Must be after processMethodToFunction
 */
static void processAlloca(Function *func)
{
    std::map<const Value *,Value *> remapValue;
    std::list<Instruction *> moveList;
if (func->getName() == "zz_ZN7Connect7respondEv") {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
func->dump();
}
    for (auto BB = func->begin(), BE = func->end(); BB != BE; ++BB) {
        for (auto II = BB->begin(), IE = BB->end(); II != IE;) {
            auto PI = std::next(BasicBlock::iterator(II));
            switch (II->getOpcode()) {
            case Instruction::Store:
                if (Instruction *target = dyn_cast<Instruction>(II->getOperand(1))) {
                if (target->getOpcode() == Instruction::Alloca) {
                    if (!dyn_cast<CallInst>(II->getOperand(0))) { // don't do remapping for calls
                    // remember values stored in Alloca temps
                    remapValue[II->getOperand(1)] = II->getOperand(0);
                    recursiveDelete(II);
                    }
                }
                else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(II->getOperand(0))) {
                    (void)CE;
                    //printf("[%s:%d] func %s val %d\n", __FUNCTION__, __LINE__, func->getName().str().c_str(), II->getOperand(0)->getValueID());
                    //CE->dump();
                    recursiveDelete(II);
                }
                }
                break;
            case Instruction::Load:
                if (Value *val = remapValue[II->getOperand(0)]) {
                    // replace loads from temp areas with stored values
                    II->replaceAllUsesWith(val);
                    recursiveDelete(II);
                }
                break;
            case Instruction::Call:
                if (CallInst *CI = dyn_cast<CallInst>(II))
                if (II->getType() == Type::getVoidTy(II->getParent()->getContext())
                  && !CI->hasStructRetAttr())
                    moveList.push_back(II); // move all Action calls to end of basic block
                break;
            };
            II = PI;
        }
    }
    for (auto item: moveList)
        item->moveBefore(item->getParent()->getTerminator());
}

static void processSelect(Function *thisFunc)
{
    for (auto BB = thisFunc->begin(), BE = thisFunc->end(); BB != BE; ++BB) {
        for (auto II = BB->begin(), IE = BB->end(); II != IE;) {
            auto PI = std::next(BasicBlock::iterator(II));
            switch (II->getOpcode()) {
            case Instruction::Select:
                II->replaceAllUsesWith(II->getOperand(2));
                recursiveDelete(II);
                break;
            };
            II = PI;
        }
    }
}

/*
 * Lookup vtable-referenced functions, changing IR to reference actual bound
 * function directly.  Also inline references to methods from the same class,
 * since these internal references must be inlined in generated verilog.
 */
static void processMethodInlining(Function *thisFunc, Function *parentFunc)
{
    for (auto BB = thisFunc->begin(), BE = thisFunc->end(); BB != BE; ++BB) {
        for (auto II = BB->begin(), IE = BB->end(); II != IE;) {
            auto PI = std::next(BasicBlock::iterator(II));
            if (CallInst *ICL = dyn_cast<CallInst>(II)) {
                Module *Mod = thisFunc->getParent();
                std::string callingName = thisFunc->getName();
                const StructType *callingSTy = findThisArgument(thisFunc);
                Value *callV = ICL->getCalledValue();
                Function *func = dyn_cast<Function>(callV);
                if (Instruction *oldOp = dyn_cast<Instruction>(callV)) {
                    std::string opName = printOperand(callV, false);
                    func = dyn_cast_or_null<Function>(Mod->getNamedValue(opName));
                    if (!func) {
                        printf("%s: %s not an instantiable call!!!! %s\n", __FUNCTION__, parentFunc->getName().str().c_str(), opName.c_str());
                        II->dump();
                        thisFunc->dump();
                        callingSTy->dump();
                        if (parentFunc != thisFunc)
                            parentFunc->dump();
                        exit(-1);
                    }
                    II->setOperand(II->getNumOperands()-1, func);
                    recursiveDelete(oldOp);
                }
                std::string calledName = func->getName();
                const StructType *STy = findThisArgument(func);
                //int ind = calledName.find("EEaSERKS0_");
                //printf("%s: %s CALLS %s cSTy %p STy %p ind %d\n", __FUNCTION__, callingName.c_str(), calledName.c_str(), callingSTy, STy, ind);
                if (callingSTy == STy || endswith(calledName, "C2Ev") || endswith(calledName, "D2Ev")) {
                    //fprintf(stdout,"callProcess: %s cName %s single!!!!\n", callingName.c_str(), calledName.c_str());
                    processAlloca(func);
                    processMethodInlining(func, parentFunc);
                    InlineFunctionInfo IFI;
                    InlineFunction(ICL, IFI, false);
                }
            };
            II = PI;
        }
    }
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
    if (Instruction *blockCond = dyn_cast_or_null<Instruction>(getCondition(argI->getParent(), 1))) // get inverted condition, if any
        newI = BinaryOperator::Create(Instruction::Or, newI, cloneTree(blockCond,TI), "newor", TI);
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
                    printf("[%s:%d] condition %s [%p, %p]\n", __FUNCTION__, __LINE__, cond.c_str(), BI->getSuccessor(0), BI->getSuccessor(1));
                    blockCondition[0].val[BI->getSuccessor(0)] = BI->getCondition(); // 'true' condition
                    blockCondition[1].val[BI->getSuccessor(1)] = BI->getCondition(); // 'inverted' condition
                }
                else if (isa<IndirectBrInst>(II)) {
                    printf("[%s:%d] indirect\n", __FUNCTION__, __LINE__);
                    for (unsigned i = 0, e = II->getNumOperands(); i != e; ++i) {
                        printf("[%d] = %s\n", i, printOperand(II->getOperand(i), false).c_str());
                    }
                }
                else {
                    printf("[%s:%d] BRUNCOND %p\n", __FUNCTION__, __LINE__, BI->getSuccessor(0));
                }
                break;
                }
            case Instruction::GetElementPtr: {
                    int maxop = II->getNumOperands();
                    if (maxop == 2)
                    if (Instruction *switchIndex = dyn_cast<Instruction>(II->getOperand(1))) {
                    int Values_size = 2;
printf("[%s:%d] maxop %d\n", __FUNCTION__, __LINE__, maxop);
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
printf("[%s:%d] get dyn size (static not handled)\n", __FUNCTION__, __LINE__);
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
            }
            II = INEXT;
        }
    }
}

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
        blockCondition[invert].val[bb] = BinaryOperator::Create(Instruction::Xor,
           val, builder.getInt1(1), "invertCond", TI);
        return blockCondition[invert].val[bb];
    }
    return NULL;
}

// rename parameter names so that they are prefixed by method name (so that
// all parameter names are unique across module for verilog instantiation)
static void updateParameterNames(std::string mName, Function *func)
{
    auto AI = ++func->arg_begin(), AE = func->arg_end();
    if (func->hasStructRetAttr()) {
        func->arg_begin()->setName(mName);
        AI++;
    }
    for (; AI != AE; AI++)
        AI->setName(mName + "_" + AI->getName());
}

static void setSeen(Function *func, std::string mName)
{
    pushSeen[func] = mName;
    processAlloca(func);
}
/*
 * Add a function to the processing list for generation of cpp and verilog.
 */
static void pushWork(std::string mName, Function *func)
{
    const StructType *STy = findThisArgument(func);
    ClassMethodTable *table = classCreate[STy];
    if (pushSeen[func] != "")
        return;
    setSeen(func, mName);
    //printf("[%s:%d] mname %s funcname %s\n", __FUNCTION__, __LINE__, mName.c_str(), func->getName().str().c_str());
    table->method[mName] = func;
    updateParameterNames(mName, func);
    // inline intra-class method call bodies
    processMethodInlining(func, func);
    fixupFuncList.push_back(func);
}

/*
 * Methods, guarded values, rules all get pushed as pairs so that 'RDY' function
 * is processed after processing for base method (so that guards promoted during
 * the method processing are later handled)
 */
static void pushPair(Function *enaFunc, std::string enaName, Function *rdyFunc, std::string rdyName)
{
    ruleRDYFunction[enaFunc] = rdyFunc; // must be before pushWork() calls
    pushWork(enaName, enaFunc);
    pushWork(rdyName, rdyFunc); // must be after 'ENA', since hoisting copies guards
}

/*
 * Called from user constructors to force processing of interface 'request' methods
 * to classes.
 */
extern "C" void exportRequest(Function *enaFunc)
{
    ClassMethodTable *table = classCreate[findThisArgument(enaFunc)];
    std::string enaName = getMethodName(enaFunc->getName());
    std::string rdyName = enaName + "__RDY";
    if (trace_pair)
        printf("[%s:%d] func %s [%s]\n", __FUNCTION__, __LINE__, enaFunc->getName().str().c_str(), rdyName.c_str());
    for (unsigned int i = 0; i < table->vtableCount; i++)
        if (getMethodName(table->vtable[i]->getName()) == rdyName)
            pushPair(enaFunc, enaName, table->vtable[i], rdyName);
}

/*
 * Process 'blocks' functions that were generated for user rules.
 * The blocks context is removed; the functions are transformed into
 * a method (and its associated RDY method), attached to the containing class.
 */
static Function *fixupFunction(std::string methodName, Function *argFunc, uint8_t *blockData)
{
    Function *fnew = NULL;
    ValueToValueMapTy VMap;
    SmallVector<ReturnInst*, 8> Returns;  // Ignore returns cloned.
    ValueToValueMapTy VMapfunc;
    SmallVector<ReturnInst*, 8> Returnsfunc;  // Ignore returns cloned.
    if (trace_fixup) {
        printf("[%s:%d] BEFORECLONE method %s func %p\n", __FUNCTION__, __LINE__, methodName.c_str(), argFunc);
        argFunc->dump();
    }
    Function *func = Function::Create(FunctionType::get(argFunc->getReturnType(),
                        argFunc->getFunctionType()->params(), false), GlobalValue::LinkOnceODRLinkage,
                        "TEMPFUNC", argFunc->getParent());
    VMapfunc[argFunc->arg_begin()] = func->arg_begin();
    CloneFunctionInto(func, argFunc, VMapfunc, false, Returnsfunc, "", nullptr);
    processAlloca(func);
    if (trace_fixup) {
        printf("[%s:%d] BEFORE method %s func %p\n", __FUNCTION__, __LINE__, methodName.c_str(), func);
        func->dump();
    }
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
                    Argument *newArg = new Argument(PTy, "temporary_this", func);
                    II->replaceAllUsesWith(newArg);
                    VMap[newArg] = fnew->arg_begin();
                    recursiveDelete(II);
                }
                else if (II->use_empty())
                    recursiveDelete(II);
                else if (Instruction *target = dyn_cast<Instruction>(II->getOperand(0)))
                    if (GetElementPtrInst *IG = dyn_cast<GetElementPtrInst>(target))
                    if (Instruction *ptr = dyn_cast<Instruction>(IG->getPointerOperand()))
                    if (ptr->getOpcode() == Instruction::BitCast)
                    if (dyn_cast<Argument>(ptr->getOperand(0))) {
                        VectorType *LastIndexIsVector = NULL;
                        uint64_t Total = getGEPOffset(&LastIndexIsVector, gep_type_begin(IG), gep_type_end(IG));
                        IRBuilder<> builder(II->getParent());
                        builder.SetInsertPoint(II);
                        Value *param = builder.getInt32(*(uint32_t *)(blockData + Total));
                        II->replaceAllUsesWith(param);
                        recursiveDelete(II);
                    }
                break;
            case Instruction::SExt: {
                if (const ConstantInt *CI = dyn_cast<ConstantInt>(II->getOperand(0))) {
                    IRBuilder<> builder(II->getParent());
                    builder.SetInsertPoint(II);
                    Value *param = builder.getInt64(CI->getZExtValue());
                    II->replaceAllUsesWith(param);
                    recursiveDelete(II);
                }
                break;
                }
            }
            II = PI;
        }
    }
    func->getArgumentList().pop_front(); // remove original argument
    CloneFunctionInto(fnew, func, VMap, false, Returns, "", nullptr);
    if (trace_fixup) {
        printf("[%s:%d] AFTER method %s\n", __FUNCTION__, __LINE__, methodName.c_str());
        fnew->dump();
    }
    func->setName("unused_block_function");
    return fnew;
}

/*
 * Called from user constructors to process Blocks functions generated for a rule
 */
extern "C" void addBaseRule(void *thisp, const char *name, Function **RDY, Function **ENA)
{
    Function *enaFunc = fixupFunction(name, ENA[2], (uint8_t *)ENA);
    Function *rdyFunc = fixupFunction(std::string(name) + "__RDY", RDY[2], (uint8_t *)RDY);
    ClassMethodTable *table = classCreate[findThisArgument(rdyFunc)];
    table->rules[name] = enaFunc;
    if (trace_pair)
        printf("[%s:%d] name %s ena %s rdy %s\n", __FUNCTION__, __LINE__, name, enaFunc->getName().str().c_str(), rdyFunc->getName().str().c_str());
    pushPair(enaFunc, getMethodName(enaFunc->getName()), rdyFunc, getMethodName(rdyFunc->getName()));
}

/*
 * Dump all statically allocated and malloc'ed data areas
 */
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

/*
 * Check if one StructType inherits another one.
 */
static int derivedStruct(const StructType *STyA, const StructType *STyB)
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
    if (const PointerType *PTyA = cast_or_null<PointerType>(A))
    if (const PointerType *PTyB = cast_or_null<PointerType>(B))
        return derivedStruct(dyn_cast<StructType>(PTyA->getElementType()),
                             dyn_cast<StructType>(PTyB->getElementType()));
    return 0;
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

/*
 * Return the name of the 'ind'th field of a StructType.
 * This code depends on a modification to llvm/clang that generates structFieldMap.
 */
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

/*
 * Lookup a function name from the vtable for a class.
 */
std::string lookupMethodName(const ClassMethodTable *table, int ind)
{
    if (trace_lookup)
        printf("[%s:%d] table %p, ind %d max %d\n", __FUNCTION__, __LINE__, table, ind, table ? table->vtableCount:-1);
    if (table && ind >= 0 && ind < (int)table->vtableCount)
        return table->vtable[ind]->getName();
    return "";
}

/*
 * Transform a MemberFunctionPointer into a normal c++ function pointer,
 * looking up as necessary in vtable.  Note that the 'this' pointer offset
 * field is assumed to be 0 and discarded in the process.  The original
 * MemberFunctionPointer is a struct of {i64, i64}.
 * The first i64 is either the address of a C++ function or (if odd)
 * the offset in bytes + 1 in the vtable.
 * The second i64 seems to be an offset for 'this', for derived types.
 */
static void processMethodToFunction(CallInst *II)
{
    Function *callingFunction = II->getParent()->getParent();
    IRBuilder<> builder(II->getParent());
    builder.SetInsertPoint(II);
    const StructType *STy = findThisArgument(callingFunction);
    ClassMethodTable *table = classCreate[STy];
    Value *oldOp = II->getOperand(1);
    II->setOperand(1, ConstantInt::get(Type::getInt64Ty(II->getContext()), (uint64_t)STy));
    recursiveDelete(oldOp);
    Instruction *load = dyn_cast<Instruction>(II->getOperand(0));
    Instruction *gep = dyn_cast<Instruction>(load->getOperand(0));
    Value *gepPtr = gep->getOperand(0);
    Instruction *store = NULL;
    Value *val = NULL;
    for (auto UI = gepPtr->use_begin(), UE = gepPtr->use_end(); UI != UE; UI++) {
        if (Instruction *IR = dyn_cast<Instruction>(UI->getUser()))
        if (IR->getOpcode() == Instruction::Store) {
            store = IR;
            if (const ConstantStruct *CS = cast<ConstantStruct>(store->getOperand(0))) {
                Value *vop = CS->getOperand(0);
                Function *func = NULL;
                int64_t offset = -1;
                if (ConstantInt *CI = dyn_cast<ConstantInt>(vop)) {
                    offset = CI->getZExtValue()/sizeof(uint64_t);
                    if (offset >= table->vtableCount) {
                        printf("[%s:%d] offset %ld too large %d\n", __FUNCTION__, __LINE__, offset, table->vtableCount);
                        exit(-1);
                    }
                    func = table->vtable[offset];
                }
                else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(vop)) {
                    ERRORIF(CE->getOpcode() != Instruction::PtrToInt);
                    func = dyn_cast<Function>(CE->getOperand(0));
                }
                else
                    ERRORIF(1);
                //printf("[%s:%d] STY %s offset %ld. func %p [%s]\n", __FUNCTION__, __LINE__, STy->getName().str().c_str(), offset, func, func->getName().str().c_str());
                val = ConstantInt::get(Type::getInt64Ty(II->getContext()), (uint64_t)func);
            }
        }
    }
    if (!store) {
        printf("%s: store was NULL\n", __FUNCTION__);
        II->getParent()->getParent()->dump();
        exit(-1);
    }
    recursiveDelete(store);
    II->replaceAllUsesWith(val);
    recursiveDelete(II);      // No longer need to call methodToFunction() !
}

/*
 * Map calls to 'new()' and 'malloc()' in constructors to call 'llvm_translate_malloc'.
 * This enables llvm-translate to easily maintain a list of valid memory regions
 * during processing.
 */
static void processMalloc(CallInst *II)
{
    Function *callingFunc = II->getParent()->getParent();
    Module *Mod = II->getParent()->getParent()->getParent();
    Value *called = II->getOperand(II->getNumOperands()-1);
    const Function *CF = dyn_cast<Function>(called);
    const Type *Typ = NULL;
    const StructType *STy = findThisArgument(II->getParent()->getParent());
    uint64_t styparam = (uint64_t)STy;
    Value *vecparam = NULL;
    for(auto PI = II->user_begin(), PE = II->user_end(); PI != PE; PI++) {
        Instruction *ins = dyn_cast<Instruction>(*PI);
        printf("[%s:%d] ins %p opcode %s\n", __FUNCTION__, __LINE__, ins, ins->getOpcodeName());
        ins->dump();
        if (ins->getOpcode() == Instruction::BitCast) {
            if (!Typ)
                Typ = ins->getType();
            else {
                Instruction *PI = ins->user_back();
                PI->dump();
                if (PI->getOpcode() == Instruction::Store)
                    vecparam = PI->getOperand(0);
            }
        }
        if (ins->getOpcode() == Instruction::GetElementPtr) {
            Instruction *PI = ins->user_back();
            PI->dump();
            if (PI->getOpcode() == Instruction::BitCast)
                Typ = PI->getType();
        }
    }
    uint64_t tparam = (uint64_t)Typ;
    printf("%s: %s calling %s styparam %lx tparam %lx vecparam %p\n",
        __FUNCTION__, callingFunc->getName().str().c_str(), CF->getName().str().c_str(), styparam, tparam, vecparam);
    STy->dump();
    if (Typ)
        Typ->dump();
    II->dump();
    Type *Params[] = {Type::getInt64Ty(Mod->getContext()),
        Type::getInt64Ty(Mod->getContext()), Type::getInt64Ty(Mod->getContext()),
        Type::getInt64Ty(Mod->getContext())};
    FunctionType *fty = FunctionType::get(Type::getInt8PtrTy(Mod->getContext()),
        ArrayRef<Type*>(Params, 4), false);
    Function *F = dyn_cast<Function>(Mod->getOrInsertFunction("llvm_translate_malloc", fty));
    F->setCallingConv(CF->getCallingConv());
    F->setDoesNotAlias(0);
    F->setAttributes(CF->getAttributes());
    IRBuilder<> builder(II->getParent());
    builder.SetInsertPoint(II);
    if (!vecparam)
        vecparam = builder.getInt64(-1);
    II->replaceAllUsesWith(builder.CreateCall(F,
       {II->getOperand(0), builder.getInt64(tparam), builder.getInt64(styparam), vecparam},
       "llvm_translate_malloc"));
    II->eraseFromParent();
}

/*
 * replace unsupported calls to llvm.umul.with.overflow.i64, llvm.uadd.with.overflow.i64
 */
static void processOverflow(CallInst *II)
{
    Function *func = dyn_cast<Function>(II->getCalledValue());
    std::string fname = func->getName();
    IRBuilder<> builder(II->getParent());
    builder.SetInsertPoint(II);
    Value *LHS = II->getOperand(0);
    Value *RHS = II->getOperand(1);
    Value *newins = (fname == "llvm.umul.with.overflow.i64") ? builder.CreateMul(LHS, RHS)
         : builder.CreateAdd(LHS, RHS);
printf("[%s:%d] func %s\n", __FUNCTION__, __LINE__, fname.c_str());
    //newins->dump();
    for(auto UI = II->user_begin(), UE = II->user_end(); UI != UE;) {
        auto UIN = std::next(UI);
        UI->replaceAllUsesWith(newins);
        recursiveDelete(*UI);
        UI = UIN;
    }
    //Rem->dropAllReferences();
    //Rem->eraseFromParent();
    //Function *callingFunc = II->getParent()->getParent();
    //callingFunc->dump();
}

/*
 * Preprocess memcpy calls
 */
static void processMemcpy(CallInst *II)
{
    Instruction *dest = dyn_cast<Instruction>(II->getOperand(0));
    Argument *destArg = NULL;
    if (dest->getOpcode() == Instruction::BitCast)
        destArg = dyn_cast<Argument>(dest->getOperand(0));
    if (Instruction *source = dyn_cast<Instruction>(II->getOperand(1)))
    if (source->getOpcode() == Instruction::BitCast)
    if (dest->getOpcode() == Instruction::BitCast)
    if (Instruction *destTmp = dyn_cast<Instruction>(dest->getOperand(0))) {
    if (destTmp->getOpcode() == Instruction::Alloca) {
        if (Argument *sourceArg = dyn_cast<Argument>(source->getOperand(0))) {
            dest->getOperand(0);
            destTmp->replaceAllUsesWith(sourceArg);
            recursiveDelete(II);
            recursiveDelete(destTmp);
        }
        else if (Instruction *sourceTmp = dyn_cast<Instruction>(source->getOperand(0))) {
            if (sourceTmp->getOpcode() == Instruction::Alloca) {
//printf("[%s:%d] A->A\n", __FUNCTION__, __LINE__);
//destTmp->dump();
//sourceTmp->dump();
//Function *func = II->getParent()->getParent();
//func->dump();
                if (destTmp->getType() == sourceTmp->getType()) {
                destTmp->replaceAllUsesWith(sourceTmp);
                dest->setOperand(0, NULL);
                recursiveDelete(destTmp);
//destTmp->dump();
                recursiveDelete(II);
                }
                else {
printf("[%s:%d] memcpy/alloca, types not match %s\n", __FUNCTION__, __LINE__, II->getParent()->getParent()->getName().str().c_str());
II->dump();
destTmp->getType()->dump();
sourceTmp->getType()->dump();
                }
//printf("[%s:%d] aft\n", __FUNCTION__, __LINE__);
//func->dump();
            }
        }
    }
    else if (const PointerType *PTy = dyn_cast<PointerType>(destTmp->getType())) {
        if (const StructType *STy = dyn_cast<StructType>(PTy->getElementType()))
        if (STy->getName() == "class.BitsClass") {
            printf("[%s:%d] remove memcpy(BitsClass)\n", __FUNCTION__, __LINE__);
            recursiveDelete(II);
        }
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
                printf("[%s:%d] addr %p callMap[%s] = %p [%p = %s]\n", __FUNCTION__, __LINE__, addr, fname.c_str(), eaddr, func, func->getName().str().c_str());
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
                    Function *calledFunc = callMap[sname];
                    if (trace_pair)
                        printf("[%s:%d] set methodMap [%s] = %p [%s]\n", __FUNCTION__, __LINE__, (name + std::string("_") + mName).c_str(), calledFunc, sname.c_str());
                    if (calledFunc)
                        methodMap[name + std::string("_") + mName] = calledFunc;
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

static void addMethodTable(std::string className, Function *func)
{
    const StructType *STy = findThisArgument(func);
    if (!STy || !STy->hasName())
        return;
    std::string sname = STy->getName();
    if (!strncmp(sname.c_str(), "class.std::", 11)
     || !strncmp(sname.c_str(), "struct.std::", 12))
        return;   // don't generate anything for std classes
    ClassMethodTable *table = classCreate[STy];
    std::string fname = func->getName();
    int status;
    const char *ret = abi::__cxa_demangle(fname.c_str(), 0, 0, &status);
    std::string fdname = ret;
    std::string cname = sname.substr(6);
    //if (className != "")
        //printf("[%s:%d] className %s cname %s\n", __FUNCTION__, __LINE__, className.c_str(), cname.c_str());
    if (sname.substr(0, 6) == "class.") {
        int ind = cname.find(".");
        if (ind > 0)
            cname = cname.substr(0,ind);
    }
    int dind = fdname.find("::");
    if (dind > 0)
        fdname = fdname.substr(0,dind);
    if (fdname.substr(0, cname.length()) == cname) {
        fdname = fdname.substr(cname.length());
        if (fdname[0] == '<' && fdname[fdname.length()-1] == '>')
            fdname = fdname.substr(1, fdname.length() - 2);
        if (table->instance == "")
            table->instance = fdname;
        if (table->instance != fdname) {
            printf("[%s:%d] instance name does not match '%s' '%s'\n", __FUNCTION__, __LINE__, table->instance.c_str(), fdname.c_str());
            exit(-1);
        }
    }
    if (trace_lookup)
        printf("%s: %s[%d] = %s [%s]\n", __FUNCTION__, sname.c_str(), table->vtableCount, fname.c_str(), fdname.c_str());
    table->vtable[table->vtableCount++] = func;
}

/*
 * Perform any processing needed on the IR before execution.
 * This includes:
 *    * Removal of calls to dwarf debug functions
 *    * change all malloc/new calls to point to our runtime, so we can track them
 *    * Capture all vtable information for types (so we can lookup functions)
 *    * Process/remove all 'methodToFunction' calls (which allow the generation
 *    *     of function pointers for class methods)
 * Context: Run after IR file load, before executing constructors.
 */
void preprocessModule(Module *Mod)
{
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

    // remove Select statements
    for (auto FI = Mod->begin(), FE = Mod->end(); FI != FE; FI++)
        processSelect(FI);
    // replace unsupported calls to llvm.umul.with.overflow.i64, llvm.uadd.with.overflow.i64
    const char *overflow_names[] = { "llvm.umul.with.overflow.i64", "llvm.uadd.with.overflow.i64", NULL};
    p = overflow_names;
    while(*p)
        if (Function *Declare = Mod->getFunction(*p++))
            for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; I++)
                processOverflow(cast<CallInst>(*I));

    // remap all calls to 'malloc' and 'new' to our runtime.
    const char *malloc_names[] = { "_Znwm", "_Znam", "malloc", NULL};
    p = malloc_names;
    while(*p)
        if (Function *Declare = Mod->getFunction(*p++))
            for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; I++)
                processMalloc(cast<CallInst>(*I));

    // extract vtables for classes.
    for (auto MI = Mod->global_begin(), ME = Mod->global_end(); MI != ME; MI++) {
        std::string name = MI->getName();
        const ConstantArray *CA;
        int status;
        const char *ret = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
        if (ret && !strncmp(ret, "vtable for ", 11)
         && MI->hasInitializer() && (CA = dyn_cast<ConstantArray>(MI->getInitializer()))) {
bool traceme = name == "_ZTV20EchoIndicationOutput";
            if (trace_lookup)
                printf("[%s:%d] global %s ret %s\n", __FUNCTION__, __LINE__, name.c_str(), ret);
            std::string className = ret+11;
            for (auto CI = CA->op_begin(), CE = CA->op_end(); CI != CE; CI++) {
                if (ConstantExpr *vinit = dyn_cast<ConstantExpr>((*CI)))
                if (vinit->getOpcode() == Instruction::BitCast)
                if (Function *func = dyn_cast<Function>(vinit->getOperand(0)))
{
if (traceme)
printf("[%s:%d] ZZZZZ func %p name %s\n", __FUNCTION__, __LINE__, func, func->getName().str().c_str());
                    addMethodTable(className, func);
}
            }
        }
    }
    // now add all non-virtual functions to end of vtable
    // Context: must be run after vtable extraction (so that functions are appended to end)
    for (auto FB = Mod->begin(), FE = Mod->end(); FB != FE; ++FB)
        addMethodTable("", FB);  // append to end of vtable (must run after vtable processing)

    // replace calls to methodToFunction with "Function *" values.
    // Context: Must be after all vtable processing.
    if (Function *Declare = Mod->getFunction("methodToFunction"))
        for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; ) {
            auto NI = std::next(I);
            processMethodToFunction(cast<CallInst>(*I));
            I = NI;
        }

    // process calls to llvm.memcpy
    if (Function *Declare = Mod->getFunction("llvm.memcpy.p0i8.p0i8.i64"))
        for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; ) {
            auto NI = std::next(I);
            processMemcpy(cast<CallInst>(*I));
            I = NI;
        }

//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    //Mod->getFunction("_ZN7IVectorC2EP17IVectorIndicationi")->dump();
}

static void mapType(Module *Mod, char *addr, Type *Ty, std::string aname)
{
    const DataLayout *TD = EE->getDataLayout();
    if (!addr || addr == BOGUS_POINTER
     || addressTypeAlreadyProcessed[MAPSEEN_TYPE{addr, Ty}])
        return;
    addressTypeAlreadyProcessed[MAPSEEN_TYPE{addr, Ty}] = 1;
    //printf("%s: addr %p TID %d Ty %p name %s\n", __FUNCTION__, addr, Ty->getTypeID(), Ty, aname.c_str());
    if (validateAddress(3010, addr))
        printf("%s: bad addr %p TID %d Ty %p name %s\n", __FUNCTION__, addr, Ty->getTypeID(), Ty, aname.c_str());
    switch (Ty->getTypeID()) {
    case Type::StructTyID: {
        StructType *STy = cast<StructType>(Ty);
        getStructName(STy); // allocate classCreate
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
                        classCreate[STy]->STy = STy;
                        classCreate[STy]->replaceType[Idx] = info.type;
                        classCreate[STy]->replaceCount[Idx] = info.vecCount;
                        if (STy == info.STy) {
                            classCreate[STy]->allocateLocally[Idx] = true;
                            inlineReferences(Mod, STy, Idx, info.type);
                            classCreate[STy]->replaceType[Idx] = cast<PointerType>(info.type)->getElementType();
                        }
                    }
            }
            if (fname != "") {
                if (StructType *iSTy = dyn_cast<StructType>(element))
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

    if (trace_malloc)
        dumpMemoryRegions(4010);
}
