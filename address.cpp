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
static int trace_fixup;// = 1;
static int trace_hoist;//= 1;
static int trace_lookup;//= 1;

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
typedef struct {
    StructType *STy;
    int         Idx;
} InterfaceTypeInfo;
typedef std::map<std::string, Function *>MethodMapType;

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
static std::map<const StructType *, int> STyList;
static std::map<void *, InterfaceTypeInfo> interfaceMap;

static void pushMethodMap(MethodMapType &methodMap, std::string prefixName, const StructType *STy);

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

extern "C" void registerInstance(char *addr, StructType *STy, const char *name)
{
    MethodMapType methodMap;
    const DataLayout *TD = EE->getDataLayout();
    const StructLayout *SLO = TD->getStructLayout(STy);
    std::string sname = STy->getName();
    //printf("[%s:%d] addr %p STy %p name %s\n", __FUNCTION__, __LINE__, addr, STy, name);
    //STy->dump();
    int Idx = 0;
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        char *eaddr = addr + SLO->getElementOffset(Idx);
        if (PointerType *PTy = dyn_cast<PointerType>(*I))
        if (PTy->getElementType()->getTypeID() == Type::FunctionTyID) {
            Function *func = *(Function **)eaddr;
            ERRORIF(!func);
            methodMap[getMethodName(func->getName())] = func;
        }
    }
    pushMethodMap(methodMap, name, NULL);
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

static void processMethodInlining(Function *currentFunction, Function &F)
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
                    std::string fname = printOperand(callV, false);
                    func = dyn_cast_or_null<Function>(Mod->getNamedValue(fname));
                    if (!func) {
                        printf("%s: %s not an instantiable call!!!! %s\n", __FUNCTION__, currentFunction->getName().str().c_str(), fname.c_str());
                        callingSTy->dump();
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
                    processMethodInlining(currentFunction, *func);
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

static void addGuard(Instruction *argI, Function *func, Function *currentFunction)
{
    Function *parentRDYName = ruleRDYFunction[currentFunction];
    if (!parentRDYName || !func)
        return;
    TerminatorInst *TI = parentRDYName->begin()->getTerminator();
    Value *cond = TI->getOperand(0);
    const ConstantInt *CI = dyn_cast<ConstantInt>(cond);
    Instruction *newI = copyFunction(TI, argI, func, Type::getInt1Ty(TI->getContext()));
    if (CallInst *nc = dyn_cast<CallInst>(newI))
        nc->addAttribute(AttributeSet::ReturnIndex, Attribute::ZExt);
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

static void updateParameterNames(std::string prefixName, Function *currentFunction)
{
    if (prefixName != "")
        prefixName += "_";
    std::string mName = getMethodName(currentFunction->getName());
    int skip = 1;
    for (auto AI = currentFunction->arg_begin(), AE = currentFunction->arg_end(); AI != AE; ++AI) {
        // update parameter names to be prefixed by method name (so that
        // all parameter names are unique across module for verilog instantiation)
        if (!skip)
            AI->setName(mName + "_" + AI->getName());
        skip = 0;
    }
}

// Preprocess the body rules, creating shadow variables and moving items to RDY() and ENA()
// Walk list of work items, cleaning up function references and adding to vtableWork
static void processPromote(Function *currentFunction)
{
    Module *Mod = currentFunction->getParent();
    updateParameterNames("", currentFunction);
    for (auto BI = currentFunction->begin(), BE = currentFunction->end(); BI != BE; ++BI) {
        for (auto II = BI->begin(), IE = BI->end(); II != IE;) {
            auto INEXT = std::next(BasicBlock::iterator(II));
            if (CallInst *ICL = dyn_cast<CallInst>(II)) {
                Value *callV = ICL->getCalledValue();
                Function *func = dyn_cast<Function>(callV);
                ClassMethodTable *table = classCreate[findThisArgumentType(func->getType())];
                if (trace_hoist)
                    printf("HOIST: CALLER %s calling '%s' table %p\n", currentFunction->getName().str().c_str(), func->getName().str().c_str(), table);
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
                std::string rdyName = getMethodName(func->getName()) + "__RDY";
                for (unsigned int i = 0; table && i < table->vtableCount; i++) {
                    Function *mfunc = table->vtable[i];
                    if (trace_hoist)
                        printf("HOIST: act %s req %s\n", getMethodName(mfunc->getName()).c_str(), rdyName.c_str());
                    if (getMethodName(mfunc->getName()) == rdyName) {
                        addGuard(II, mfunc, currentFunction);
                        break;
                    }
                }
            }
            II = INEXT;
        }
    }
}

std::map<Function *, int> pushSeen;
static void pushWork(std::string mname, Function *func)
{
    if (pushSeen[func])
        return;
    pushSeen[func] = 1;
    ClassMethodTable *table = classCreate[findThisArgumentType(func->getType())];
    table->method[mname] = func;
    vtableWork.push_back(func);
    // inline intra-class method call bodies
    processMethodInlining(func, *func);
    // promote guards from contained calls to be guards for this function
    processPromote(func);
}

static void prefixFunction(Function *func, std::string prefixName)
{
    std::string fname = func->getName();
    if (prefixName != "" && fname.substr(0,3) == "_ZN") {
        const char *start = fname.c_str();
        const char *ptr = start + 3;
        char *endptr;
        long len = strtol(ptr, &endptr, 10);
        ptr = endptr + len;
        while (*ptr && *ptr != 'E')
            ptr++;
        if (*ptr)
            ptr++;
        len = strtol(ptr, &endptr, 10);
        fname = fname.substr(0, ptr-start) + utostr(len + prefixName.length() + 1) + prefixName + "_" + endptr;
        func->setName(fname);
    }
}
static void pushPair(Function *enaFunc, Function *rdyFunc, std::string prefixName)
{
    ruleRDYFunction[enaFunc] = rdyFunc; // must be before pushWork() calls
    if (prefixName != "")
        prefixName += "_";
    pushWork(getMethodName(enaFunc->getName()), enaFunc);
    pushWork(getMethodName(rdyFunc->getName()), rdyFunc); // must be after 'ENA', since hoisting copies guards
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
    if (trace_lookup)
        printf("[%s:%d] name %s ena %s rdy %s\n", __FUNCTION__, __LINE__, name, enaFunc->getName().str().c_str(), rdyFunc->getName().str().c_str());
    pushPair(enaFunc, rdyFunc, "");
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
    //printf("%s: addr %p TID %d Ty %p name %s\n", __FUNCTION__, addr, Ty->getTypeID(), Ty, aname.c_str());
    if (validateAddress(3010, addr))
        printf("%s: bad addr %p TID %d Ty %p name %s\n", __FUNCTION__, addr, Ty->getTypeID(), Ty, aname.c_str());
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
            if (fname != "") {
                if (dyn_cast<StructType>(element))
                    interfaceMap[eaddr] = InterfaceTypeInfo{STy, Idx};
                mapType(Mod, eaddr, element, aname + "$$" + fname);
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
    const StructType *STy = findThisArgumentType(callingFunction->getType());
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
                if (ConstantInt *CI = dyn_cast<ConstantInt>(vop))
                    func = table->vtable[CI->getZExtValue()/sizeof(uint64_t)];
                else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(vop)) {
                    ERRORIF(CE->getOpcode() != Instruction::PtrToInt);
                    func = dyn_cast<Function>(CE->getOperand(0));
                }
                else
                    ERRORIF(1);
                val = ConstantInt::get(Type::getInt64Ty(II->getContext()), (uint64_t)func);
            }
        }
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
    Module *Mod = II->getParent()->getParent()->getParent();
    Value *called = II->getOperand(II->getNumOperands()-1);
    const Function *CF = dyn_cast<Function>(called);
    Instruction *PI = II->user_back();
    uint64_t tparam = 0;
    uint64_t styparam = (uint64_t)findThisArgumentType(II->getParent()->getParent()->getType());
    //printf("[%s:%d] %s calling %s styparam %lx\n", __FUNCTION__, __LINE__, II->getParent()->getParent()->getName().str().c_str(), CF->getName().str().c_str(), styparam);
    if (PI->getOpcode() == Instruction::BitCast && &*II == PI->getOperand(0))
        tparam = (uint64_t)PI->getType();
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

static void pushMethodMap(MethodMapType &methodMap, std::string prefixName, const StructType *STy)
{
    for (auto item: methodMap)
        if (endswith(item.first, "__RDY")) {
            Function *enaFunc = methodMap[item.first.substr(0, item.first.length() - 5)];
            if (!enaFunc) {
                printf("%s: guarded function not found %s\n", __FUNCTION__, item.first.c_str());
                exit(-1);
            }
            if (inheritsModule(STy, "class.InterfaceClass")) {
                updateParameterNames(prefixName, enaFunc);
                updateParameterNames(prefixName, item.second);
            }
            else {
            prefixFunction(enaFunc, prefixName);
            prefixFunction(item.second, prefixName);
            if (trace_lookup)
                if (!pushSeen[enaFunc])
                printf("%s: prefix %s pair %s[%s] ena %p[%s]\n", __FUNCTION__, prefixName.c_str(), item.first.c_str(), item.second->getName().str().c_str(), enaFunc, enaFunc->getName().str().c_str());
            pushPair(enaFunc, item.second, prefixName);
            }
        }
}
static void processStruct(const StructType *STy, std::string prefixName)
{
    if (!STyList[STy])
        return;
    STyList[STy] = 0;
    ClassMethodTable *table = classCreate[STy];
    MethodMapType methodMap;

    int Idx = 0;
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        if (PointerType *PTy = dyn_cast<PointerType>(*I))
        if (const StructType *iSTy = dyn_cast<StructType>(PTy->getElementType())) {
            std::string fname;
            processStruct(iSTy, fname);
        }
    }
    for (auto info : classCreate)
        if (const StructType *iSTy = info.first)
        if (derivedStruct(iSTy, STy))
            processStruct(iSTy, prefixName);
    if (trace_lookup)
        printf("%s: start %s\n", __FUNCTION__, STy->getName().str().c_str());
    for (unsigned int i = 0; i < table->vtableCount; i++) {
         Function *func = table->vtable[i];
         //printf("%s: method %s\n", __FUNCTION__, getMethodName(func->getName()).c_str());
         methodMap[getMethodName(func->getName())] = func;
    }
    pushMethodMap(methodMap, prefixName, STy);
}

static void addMethodTable(Function *func)
{
    const StructType *STy = findThisArgumentType(func->getType());
    if (!STy || !STy->hasName())
        return;
    std::string sname = STy->getName();
    if (!strncmp(sname.c_str(), "class.std::", 11)
     || !strncmp(sname.c_str(), "struct.std::", 12))
        return;   // don't generate anything for std classes
    STyList[STy] = 1;
    ClassMethodTable *table = classCreate[STy];
    if (trace_lookup)
        printf("%s: %s[%d] = %s\n", __FUNCTION__, sname.c_str(), table->vtableCount, func->getName().str().c_str());
    table->vtable[table->vtableCount++] = func;
}

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

    // before running constructors, remap all calls to 'malloc' and 'new' to our runtime.
    const char *malloc_names[] = { "_Znwm", "malloc", NULL};
    p = malloc_names;
    while(*p)
        if (Function *Declare = Mod->getFunction(*p++))
            for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; I++)
                processMalloc(cast<CallInst>(*I));

    STyList.clear();
    for (auto MI = Mod->global_begin(), ME = Mod->global_end(); MI != ME; MI++) {
        std::string name = MI->getName();
        const ConstantArray *CA;
        int status;
        const char *ret = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
        if (ret && !strncmp(ret, "vtable for ", 11)
         && MI->hasInitializer() && (CA = dyn_cast<ConstantArray>(MI->getInitializer()))) {
            if (trace_lookup)
                printf("[%s:%d] global %s ret %s\n", __FUNCTION__, __LINE__, name.c_str(), ret);
            for (auto CI = CA->op_begin(), CE = CA->op_end(); CI != CE; CI++) {
                if (ConstantExpr *vinit = dyn_cast<ConstantExpr>((*CI)))
                if (vinit->getOpcode() == Instruction::BitCast)
                if (Function *func = dyn_cast<Function>(vinit->getOperand(0)))
                    addMethodTable(func);
            }
        }
    }
    // now add all non-virtual functions to end of vtable
    for (auto FB = Mod->begin(), FE = Mod->end(); FB != FE; ++FB)
        addMethodTable(FB);  // append to end of vtable (must run after vtable processing)

    // replace calls to methodToFunction with "Function *" values.  Must be after vtable processing
    if (Function *Declare = Mod->getFunction("methodToFunction"))
        for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; ) {
            auto NI = std::next(I);
            processMethodToFunction(cast<CallInst>(*I));
            I = NI;
        }

    // Add StructType parameter to registerInstance calls (must have been 'unsigned long' in original source!)
    if (Function *Declare = Mod->getFunction("registerInstance"))
        for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; I++) {
            CallInst *II = cast<CallInst>(*I);
            II->setOperand(1, ConstantInt::get(Type::getInt64Ty(II->getContext()),
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

    for (auto item: STyList)
        processStruct(item.first, "");

    if (trace_malloc)
        dumpMemoryRegions(4010);
}
