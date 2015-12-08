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

using namespace llvm;

#include "declarations.h"

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
static int trace_malloc;// = 1;
static int trace_fixup;// = 1;
static int trace_mapt;// = 1;
static std::map<void *, std::string> addressToName;
std::map<std::string, void *> nameToAddress;
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

void recursiveDelete(Value *V) //nee: RecursivelyDeleteTriviallyDeadInstructions
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
    classCreate[findThisArgumentType(rdyFunc->getType())]->rules.push_back(name);
    pushWork(enaFunc, thisp);
    pushWork(rdyFunc, thisp); // must be after 'ENA', since hoisting copies guards
    ruleRDYFunction[enaFunc] = rdyFunc;
}

static Function *addFunction(void *thisp, std::string name, ClassMethodTable *table)
{
    std::string lname = lookupMethodName(table, vtableFind(table, name));
    if (lname == "") {
        printf("ExportSymbol: could not find method %s\n", name.c_str());
        exit(-1);
    }
    Function *func = EE->FindFunctionNamed(lname.c_str());
    pushWork(func, thisp);
    return func;
}

extern "C" void exportSymbol(void *thisp, const char *name, StructType *STy)
{
    ClassMethodTable *table = classCreate[STy];
    Function *enaFunc = addFunction(thisp, name, table);
    Function *rdyFunc = addFunction(thisp, std::string(name) + "__RDY", table); // must be after 'ENA'
    ruleRDYFunction[enaFunc] = rdyFunc;
}

static void dumpMemoryRegions(int arg)
{
    printf("%s: %d\n", __FUNCTION__, arg);
    for (MEMORY_REGION info : memoryRegion) {
        const GlobalValue *g = EE->getGlobalValueAtAddress(info.p);
        std::string gname;
        if (g)
            gname = g->getName();
        printf("%p %s %s", info.p, gname.c_str(), mapAddress(info.p).c_str());
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

std::string hexAddress(void *arg)
{
    char temp[MAX_CHAR_BUFFER];
    sprintf(temp, "%p", arg);
    return temp;
}

/*
 * Build up reverse address map from all data items after running constructors
 */
std::string mapAddress(void *arg)
{
    std::string val = addressToName[arg];
    if (val != "")
        return val;
    return hexAddress(arg);
}

void *mapLookup(std::string name)
{
    char *endptr = NULL;
    if (!strncmp(name.c_str(), "0x", 2))
        return (void *)strtol(name.c_str()+2, &endptr, 16);
    return nameToAddress[name];
}

static int checkDerived(const Type *A, const Type *B)
{
    if (const PointerType *PTyA = cast<PointerType>(A))
    if (const PointerType *PTyB = cast<PointerType>(B))
    if (const StructType *STyA = dyn_cast<StructType>(PTyA->getElementType()))
    if (const StructType *STyB = dyn_cast<StructType>(PTyB->getElementType())) {
        int Idx = 0;
        for (auto I = STyA->element_begin(), E = STyA->element_end(); I != E; ++I, Idx++) {
            if (fieldName(STyA, Idx) == "" && dyn_cast<StructType>(*I) && *I == STyB) {
printf("[%s:%d] inherit %p A %p B %p\n", __FUNCTION__, __LINE__, *I, STyA, STyB);
                STyA->dump();
                STyB->dump();
                return 1;
            }
        }
    }
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

void inlineReferences(const StructType *STy, uint64_t Idx, Type *newType)
{
    for (auto FB = globalMod->begin(), FE = globalMod->end(); FB != FE; ++FB) {
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

static void mapType(char *addr, Type *Ty, std::string aname)
{
    const DataLayout *TD = EE->getDataLayout();
    if (!addr || addr == BOGUS_POINTER
     || addressTypeAlreadyProcessed[MAPSEEN_TYPE{addr, Ty}])
        return;
    addressTypeAlreadyProcessed[MAPSEEN_TYPE{addr, Ty}] = 1;
    //printf("[%s:%d] addr %p TID %d Ty %p name %s\n", __FUNCTION__, __LINE__, addr, Ty->getTypeID(), Ty, aname.c_str());
    if (validateAddress(3010, addr))
        printf("[%s:%d] baddd\n", __FUNCTION__, __LINE__);
    addressToName[addr] = aname;
    nameToAddress[aname] = addr;
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
                        if (STy == info.STy)
                            classCreate[STy]->allocateLocally[Idx] = true;
                    }
            }
            if (fname != "")
                mapType(eaddr, element, aname + "$$" + fname);
            else if (dyn_cast<StructType>(element))
                mapType(eaddr, element, aname);
        }
        break;
        }
    case Type::PointerTyID:
        mapType(*(char **)addr, cast<PointerType>(Ty)->getElementType(), aname);
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
        if (getMethodName(table->vtable[i]) == name)
            return i;
    return -1;
}

std::string lookupMethodName(const ClassMethodTable *table, int ind)
{
    if (table && ind >= 0 && ind < (int)table->vtableCount)
        return table->vtable[ind];
    return "";
}

void constructVtableMap(Module *Mod)
{
    addressToName.clear();
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
                if (const ConstantExpr *vinit = dyn_cast<ConstantExpr>((*CI)))
                if (vinit->getOpcode() == Instruction::BitCast)
                if (const Function *func = dyn_cast<Function>(vinit->getOperand(0)))
                if (const StructType *STy = findThisArgumentType(func->getType())) {
                    if (!classCreate[STy]->vtable)
                        classCreate[STy]->vtable = new std::string[numElements];
                    classCreate[STy]->vtable[classCreate[STy]->vtableCount++] = func->getName();
                }
            }
        }
    }
}

/*
 * Starting from all toplevel global, construct symbolic names for
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
            mapType((char *)addr, Ty, name);
        }
    }
    if (trace_mapt)
        dumpMemoryRegions(4010);
}
