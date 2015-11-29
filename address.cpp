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
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"

using namespace llvm;

#include "declarations.h"

typedef struct {
    unsigned int maxIndex;
    std::string *methods;
} METHOD_INFO;

typedef  struct {
    void *p;
    size_t size;
    const Type *type;
} MEMORY_REGION;

#define GIANT_SIZE 1024
static int trace_mapa;// = 1;
static int trace_malloc;// = 1;
static int trace_fixup;// = 1;
static std::map<void *, std::string> mapItem;
static std::map<MAPSEEN_TYPE, int, MAPSEENcomp> mapSeen;
static std::map<std::string, void *> mapNameLookup;
static std::map<const StructType *, METHOD_INFO *> classMethod;
static std::list<MEMORY_REGION> memoryRegion;

/*
 * Allocated memory region management
 */
extern "C" void *llvm_translate_malloc(size_t size, const Type *type)
{
    size_t newsize = size * 2 + MAX_BASIC_BLOCK_FLAGS * sizeof(int) + GIANT_SIZE;
    void *ptr = malloc(newsize);
    memset(ptr, 0x5a, newsize);
    if (trace_malloc)
        printf("[%s:%d] %ld = %p type %p\n", __FUNCTION__, __LINE__, size, ptr, type);
    memoryRegion.push_back(MEMORY_REGION{ptr, newsize, type});
    return ptr;
}

static void recursiveDelete(Value *V)
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
    const StructType *STy = NULL;
    for (auto BB = func->begin(), BE = func->end(); BB != BE; ++BB) {
        for (auto II = BB->begin(), IE = BB->end(); II != IE; ) {
            BasicBlock::iterator PI = std::next(BasicBlock::iterator(II));
            std::string vname = II->getName();
            switch (II->getOpcode()) {
            case Instruction::Load:
                if (vname == "this") {
                    II->setName("unused");
                    const PointerType *PTy = dyn_cast<PointerType>(II->getType());
                    Argument *newArg = new Argument(II->getType(), "this", func);
                    STy = dyn_cast<StructType>(PTy->getElementType());
                    II->replaceAllUsesWith(newArg);
                }
                if (II->use_empty())
                    recursiveDelete(II);
                break;
            case Instruction::Store: {
                std::string name = II->getOperand(0)->getName();
                if (name == ".block_descriptor" || name == "block")
                    recursiveDelete(II);
                break;
                }
            }
            II = PI;
        }
    }
    func->getArgumentList().pop_front(); // remove original argument
    std::string className = STy->getName().substr(6);
    func->setName("_ZN" + utostr(className.length()) + className + utostr(methodName.length()) + methodName + "Ev");
    func->setLinkage(GlobalValue::LinkOnceODRLinkage);
    if (trace_fixup)
        printf("[%s:%d] class %s method %s\n", __FUNCTION__, __LINE__, className.c_str(), methodName.c_str());
    if (!endswith(methodName, "__RDY")) {
        if (!classCreate[STy])
            classCreate[STy] = new ClassMethodTable;
        classCreate[STy]->rules.push_back(methodName);
    }
    if (trace_fixup) {
        printf("[%s:%d] AFTER\n", __FUNCTION__, __LINE__);
        func->dump();
    }
    return func;
}

extern "C" void addBaseRule(void *thisp, const char *aname, Function **RDY, Function **ENA)
{
    std::string name = aname;
    ruleInfo.push_back(new RULE_INFO{aname, thisp, fixupFunction(name + "__RDY", RDY[2]), fixupFunction(name, ENA[2])});
    ruleRDYFunction[ENA[2]] = RDY[2];
}

static void dumpMemoryRegions(int arg)
{
    printf("%s: %d\n", __FUNCTION__, arg);
    for (MEMORY_REGION info : memoryRegion) {
        const GlobalValue *g = EE->getGlobalValueAtAddress(info.p);
        std::string gname;
        if (g)
            gname = g->getName();
        printf("%p %s %s\n", info.p, gname.c_str(), mapAddress(info.p).c_str());
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

/*
 * Build up reverse address map from all data items after running constructors
 */
static void setMapAddress(void *arg, std::string name)
{
    if (mapItem[arg] == "") {
        mapItem[arg] = name;
        mapNameLookup[name] = arg;
    }
}

std::string mapAddress(void *arg)
{
    std::string val = mapItem[arg];
    if (val != "")
        return val;
    char temp[MAX_CHAR_BUFFER];
    sprintf(temp, "%p", arg);
    if (trace_mapa)
        printf("%s: %p\n", __FUNCTION__, arg);
    return temp;
}

void *mapLookup(std::string name)
{
    char *endptr = NULL;
    if (!strncmp(name.c_str(), "0x", 2))
        return (void *)strtol(name.c_str()+2, &endptr, 16);
    return mapNameLookup[name];
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

static const Type *memoryType(void *p)
{
    for (MEMORY_REGION info : memoryRegion) {
        if (p >= info.p && (size_t)p < ((size_t)info.p + info.size))
            return info.type;
    }
    return NULL;
}

static void mapType(char *addr, Type *Ty, std::string aname)
{
    const DataLayout *TD = EE->getDataLayout();
    if (!addr || addr == BOGUS_POINTER || mapSeen[MAPSEEN_TYPE{addr, Ty}])
        return;
    mapSeen[MAPSEEN_TYPE{addr, Ty}] = 1;
printf("[%s:%d] addr %p TID %d Ty %p name %s\n", __FUNCTION__, __LINE__, addr, Ty->getTypeID(), Ty, aname.c_str());
    if (validateAddress(3010, addr))
        printf("[%s:%d] baddd\n", __FUNCTION__, __LINE__);
    setMapAddress(addr, aname);
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
                const Type *Ty = memoryType(*(char **)eaddr);
                if (Ty && checkDerived(Ty, PTy))
                    replaceType[EREPLACE_INFO{STy, Idx}] = Ty;
            }
            if (fname != "")
                mapType(eaddr, element, aname + "$$" + fname);
            else if (dyn_cast<StructType>(element)) {
                //printf("[%s:%d] inherit %p eaddr %p\n", __FUNCTION__, __LINE__, element, eaddr);
                mapType(eaddr, element, aname);
            }
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

int lookupRDY(const Function *func)
{
    std::string methodString;
    if (const StructType *STy = findThisArgument(func))
    if ((methodString = getMethodName(func->getName())) != "") {
        std::string tname = STy->getName();
        METHOD_INFO *mInfo = classMethod[STy];
        for (unsigned int i = 0; mInfo && i < mInfo->maxIndex; i++)
            if (getMethodName(mInfo->methods[i]) == methodString + "__RDY")
                return i;
    }
    return -1;
}
std::string lookupMethod(const StructType *STy, uint64_t ind)
{
    std::string sname = STy->getName();
    METHOD_INFO *mInfo = classMethod[STy];
    if (mInfo && ind < mInfo->maxIndex)
        return mInfo->methods[ind];
    if (generateRegion != ProcessNone) {
        printf("%s: gname %s: could not find %s/%d. mInfo %p max %d\n", __FUNCTION__, globalName.c_str(), sname.c_str(), (int)ind, mInfo, mInfo? mInfo->maxIndex : -1);
        exit(-1);
    }
    return "";
}

/*
 * Starting from all toplevel global, construct symbolic names for
 * all reachable addresses
 */
void constructAddressMap(Module *Mod)
{
    mapItem.clear();
    for (auto MI = Mod->global_begin(), ME = Mod->global_end(); MI != ME; MI++) {
        const ConstantArray *CA;
        std::string name = MI->getName();
        Type *Ty = MI->getType()->getElementType();
        void *addr = EE->getPointerToGlobal(MI);
        int status;
        const char *ret = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
        if (ret && !strncmp(ret, "vtable for ", 11)
         && MI->hasInitializer() && (CA = dyn_cast<ConstantArray>(MI->getInitializer()))) {
            const PointerType *PTy, *Ty = dyn_cast<PointerType>(MI->getType());
            const ArrayType *ATy = dyn_cast<ArrayType>(Ty->getElementType());
            const Function *func;
            const StructType *STy;
            const ConstantExpr *vinit;
            printf("[%s:%d] global %s ret %s ATy %p\n", __FUNCTION__, __LINE__, name.c_str(), ret, ATy);
            METHOD_INFO *mInfo = NULL;
            for (auto CI = CA->op_begin(), CE = CA->op_end(); CI != CE; CI++) {
                if ((vinit = dyn_cast<ConstantExpr>((*CI)))
                 && vinit->getOpcode() == Instruction::BitCast
                 && (func = dyn_cast<Function>(vinit->getOperand(0)))
                 && (PTy = dyn_cast<PointerType>(func->arg_begin()->getType()))
                 && (STy = dyn_cast<StructType>(PTy->getElementType()))) {
                    std::string fname = func->getName();
                    std::string sname = STy->getName();
                    if (!mInfo) {
                        mInfo = new METHOD_INFO;
                        mInfo->maxIndex = 0;
                        mInfo->methods = new std::string[ATy->getNumElements()];
                        classMethod[STy] = mInfo;
                    }
                    mInfo->methods[mInfo->maxIndex++] = fname;
                }
            }
        }
        else if ((name.length() < 4 || name.substr(0,4) != ".str")
         && (name.length() < 18 || name.substr(0,18) != "__block_descriptor")) {
            memoryRegion.push_back(MEMORY_REGION{addr, EE->getDataLayout()->getTypeAllocSize(Ty), MI->getType()});
            mapType((char *)addr, Ty, name);
        }
    }
    //if (trace_mapt)
        dumpMemoryRegions(4010);
}
