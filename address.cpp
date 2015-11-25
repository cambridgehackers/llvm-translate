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
//#define DEBUG_TYPE "llvm-translate"
#include <stdio.h>
#include <list>
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"

#define MAGIC_VTABLE_OFFSET 2

using namespace llvm;

#include "declarations.h"

class ADDRESSMAP_TYPE {
public:
    std::string name;
    ADDRESSMAP_TYPE(std::string aname) {
       name = aname;
    }
};

#define GIANT_SIZE 1024
static int trace_mapa;// = 1;
static int trace_malloc;// = 1;
static int trace_fixup;// = 1;
static std::map<void *, ADDRESSMAP_TYPE *> mapitem;
static std::map<MAPSEEN_TYPE, int, MAPSEENcomp> mapseen;
static std::map<std::string, void *> maplookup;
std::list<RULE_INFO *> ruleInfo;
static std::map<std::string, Function *> ruleFunctionTable;
std::map<std::string, std::list<std::string>> ruleFunctionNames;
std::map<EREPLACE_INFO, const Type *, EREPLACEcomp> replaceType;
static struct {
    void *p;
    long size;
    const Type *type;
} callfunhack[100];

/*
 * Allocated memory region management
 */
static void addItemToList(void *p, long size, const Type *type)
{
    int i = 0;

    while(callfunhack[i].p)
        i++;
    callfunhack[i].p = p;
    callfunhack[i].size = size;
    callfunhack[i].type = type;
}

extern "C" void *llvm_translate_malloc(size_t size, const Type *type)
{
    size_t newsize = size * 2 + MAX_BASIC_BLOCK_FLAGS * sizeof(int) + GIANT_SIZE;
    void *ptr = malloc(newsize);
    memset(ptr, 0x5a, newsize);
    if (trace_malloc)
        printf("[%s:%d] %ld = %p type %p\n", __FUNCTION__, __LINE__, size, ptr, type);
    addItemToList(ptr, newsize, type);
    return ptr;
}

#if 0
int refCou(Value *OpV)
{
int ret = 0;
for (auto JJ = OpV->use_begin(); JJ != OpV->use_end(); JJ++)
    ret++;
return ret;
}
#endif
Function *lookup_function(std::string className, std::string methodName)
{
    //printf("[%s:%d] class %s method %s\n", __FUNCTION__, __LINE__, className.c_str(), methodName.c_str());
    return ruleFunctionTable[className + "//" + methodName];
}

static Function *fixupFunction(std::string methodName, Function *func)
{
    std::string className;
    for (Function::iterator BB = func->begin(), BE = func->end(); BB != BE; ++BB) {
        for (BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE; ) {
            BasicBlock::iterator PI = std::next(BasicBlock::iterator(II));
            std::string vname = II->getName();
            Argument *newArg = NULL;
            switch (II->getOpcode()) {
            case Instruction::Load:
                if (vname == "this") {
                    const PointerType *PTy = dyn_cast<PointerType>(II->getType());
                    newArg = new Argument(II->getType(), "this", func);
                    if (StructType *STy = dyn_cast<StructType>(PTy->getElementType()))
                        className = STy->getName().substr(6);
                    II->replaceAllUsesWith(newArg);
                }
                if (II->use_empty())
                    recursiveDelete(II);
                if (newArg)
                    newArg->setName("this");
                break;
            case Instruction::Store: {
                std::string name = II->getOperand(0)->getName();
                //printf("[%s:%d] name %s\n", __FUNCTION__, __LINE__, name.c_str());
                if (name == ".block_descriptor" || name == "block")
                    recursiveDelete(II);
                break;
                }
            }
            II = PI;
        }
    }
    func->getArgumentList().pop_front(); // remove original argument
    func->setName("_ZN" + utostr(className.length()) + className + utostr(methodName.length()) + methodName + "Ev");
    func->setLinkage(GlobalValue::LinkOnceODRLinkage);
    if (trace_fixup)
        printf("[%s:%d] class %s method %s\n", __FUNCTION__, __LINE__, className.c_str(), methodName.c_str());
    if (!endswith(methodName.c_str(), "__RDY"))
        ruleFunctionNames["class." + className].push_back(methodName);
    ruleFunctionTable["class." + className + "//" + methodName] = func;
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
}

static void dumpMemoryRegions(int arg)
{
    int i = 0;

    printf("%s: %d\n", __FUNCTION__, arg);
    while(callfunhack[i].p) {
        const GlobalValue *g = EE->getGlobalValueAtAddress(callfunhack[i].p);
        std::string gname;
        if (g)
            gname = g->getName();
        printf("[%d] = %p %s %s\n", i, callfunhack[i].p, gname.c_str(), mapAddress(callfunhack[i].p).c_str());
        if (callfunhack[i].type)
            callfunhack[i].type->dump();
        long size = callfunhack[i].size;
        if (size > GIANT_SIZE) {
           size -= GIANT_SIZE;
           size -= 10 * sizeof(int);
           size = size/2;
        }
        size += 16;
        memdumpl((unsigned char *)callfunhack[i].p, size, "data");
        i++;
    }
}
const Type *memoryType(void *p)
{
    int i = 0;

    while(callfunhack[i].p) {
        if (p >= callfunhack[i].p && (long)p < ((long)callfunhack[i].p + callfunhack[i].size))
            return callfunhack[i].type;
        i++;
    }
    return NULL;
}
int validateAddress(int arg, void *p)
{
    int i = 0;

    while(callfunhack[i].p) {
        if (p >= callfunhack[i].p && (long)p < ((long)callfunhack[i].p + callfunhack[i].size))
            return 0;
        i++;
    }
    printf("%s: %d address validation failed %p\n", __FUNCTION__, arg, p);
    i = 0;
    while(callfunhack[i].p) {
        const GlobalValue *g = EE->getGlobalValueAtAddress(callfunhack[i].p);
        const char *cp = "";
        if (g)
            cp = g->getName().str().c_str();
        printf("%p %s size 0x%lx\n", callfunhack[i].p, cp, callfunhack[i].size);
        i++;
    }
    return 1;
}

/*
 * Build up reverse address map from all data items after running constructors
 */
std::string setMapAddress(void *arg, std::string name)
{
    const GlobalValue *g = EE->getGlobalValueAtAddress(arg);
    ADDRESSMAP_TYPE *mptr = mapitem[arg];
    if (mptr)
        return mptr->name.c_str();
    if (g)
        name = g->getName().str();
    if (name.length() != 0) {
        mapitem[arg] = new ADDRESSMAP_TYPE(name);
        if (name != "") {
            maplookup[name] = arg;
            //printf("%s: setname %s = %p\n", __FUNCTION__, name.c_str(), arg);
        }
        if (name[0] != '(')
            return name;
    }
    static char temp[MAX_CHAR_BUFFER];
    sprintf(temp, "%p", arg);
    if (trace_mapa)
        printf("%s: %p\n", __FUNCTION__, arg);
    return temp;
}
std::string mapAddress(void *arg)
{
    return setMapAddress(arg, "");
}
void *mapLookup(std::string name)
{
    char *endptr = NULL;
    if (!strncmp(name.c_str(), "0x", 2))
        return (void *)strtol(name.c_str()+2, &endptr, 16);
    return maplookup[name];
}

std::string getVtableName(void *addr_target)
{
    if (const GlobalValue *g = EE->getGlobalValueAtAddress(((uint64_t *)addr_target)-MAGIC_VTABLE_OFFSET)) {
        int status;
        std::string name = g->getName();
        const char *ret = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
        if (ret && !strncmp(ret, "vtable for ", 11)) {
Type *Ty = g->getType();
printf("[%s:%d] %d \n", __FUNCTION__, __LINE__, Ty->getTypeID());
Ty->dump();
            if (PointerType *PTy = dyn_cast<PointerType>(Ty)) {
                Type *newel = PTy->getElementType();
printf("[%s:%d] newel %d\n", __FUNCTION__, __LINE__, newel->getTypeID());
newel->dump();
            }
            return "class." + std::string(ret+11);
        }
    }
    return "";
}

static int checkDerived(const Type *A, const Type *B)
{
    if (const PointerType *PTyA = cast<PointerType>(A))
    if (const PointerType *PTyB = cast<PointerType>(B))
    if (const StructType *STyA = dyn_cast<StructType>(PTyA->getElementType()))
    if (const StructType *STyB = dyn_cast<StructType>(PTyB->getElementType())) {
        int Idx = 0;
        for (StructType::element_iterator I = STyA->element_begin(), E = STyA->element_end(); I != E; ++I, Idx++) {
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

static void mapType(char *addr, Type *Ty, std::string aname)
{
    const DataLayout *TD = EE->getDataLayout();
    if (!addr || addr == BOGUS_POINTER || mapseen[MAPSEEN_TYPE{addr, Ty}])
        return;
    mapseen[MAPSEEN_TYPE{addr, Ty}] = 1;
printf("[%s:%d] addr %p TID %d Ty %p name %s\n", __FUNCTION__, __LINE__, addr, Ty->getTypeID(), Ty, aname.c_str());
    if (validateAddress(3010, addr))
        printf("[%s:%d] baddd\n", __FUNCTION__, __LINE__);
    switch (Ty->getTypeID()) {
    case Type::StructTyID: {
        StructType *STy = cast<StructType>(Ty);
        const StructLayout *SLO = TD->getStructLayout(STy);
        int Idx = 0;
        for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
            std::string fname = fieldName(STy, Idx);
            char *eaddr = addr + SLO->getElementOffset(Idx);
            Type *element = *I;
            if (fname != "") {
                if (PointerType *PTy = dyn_cast<PointerType>(element)) {
                    const Type *Ty = memoryType(*(char **)eaddr);
                    if (Ty && checkDerived(Ty, PTy))
                        replaceType[EREPLACE_INFO{STy, Idx}] = Ty;
                }
                mapType(eaddr, element, aname + "$$" + fname);
            }
            else if (dyn_cast<StructType>(element)) {
                //printf("[%s:%d] inherit %p eaddr %p\n", __FUNCTION__, __LINE__, element, eaddr);
                mapType(eaddr, element, aname);
            }
        }
        break;
        }
    case Type::PointerTyID: {
        PointerType *PTy = cast<PointerType>(Ty);
        Type *element = PTy->getElementType();
        char *addr_target = *(char **)addr;
        mapType(addr_target, element, aname);
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

void addressrunOnFunction(Function &F)
{
    const char *className, *methodName;
    const StructType *STy = NULL;
    std::string fname = F.getName();
//printf("addressrunOnFunction: %s\n", fname.c_str());
    if (getClassName(fname.c_str(), &className, &methodName)
     && (STy = findThisArgument(&F))) {
        std::string sname = getStructName(STy);
        std::string tname = STy->getName();
        if (!classCreate[sname])
            classCreate[sname] = new ClassMethodTable(sname, className);
        classCreate[sname]->type = STy;
    }
}

/*
 * Starting from all toplevel global, construct symbolic names for
 * all reachable addresses
 */
void constructAddressMap(Module *Mod)
{
    process_metadata(Mod);
    mapitem.clear();
    if (NamedMDNode *CU_Nodes = Mod->getNamedMetadata("llvm.dbg.cu")) {
        for (unsigned j = 0, e = CU_Nodes->getNumOperands(); j != e; ++j) {
            const DICompileUnit *CUP = dyn_cast<DICompileUnit>(CU_Nodes->getOperand(j));
            DIGlobalVariableArray GVs = CUP->getGlobalVariables();
            for (unsigned i = 0, e = GVs.size(); i != e; ++i) {
                const GlobalValue *gv = dyn_cast<GlobalValue>(GVs[i]->getVariable());
                Type *Ty = gv->getType()->getElementType();
                void *addr = EE->getPointerToGlobal(gv);
                const Metadata *node = fetchType(GVs[i]->getType());
                std::string cp = GVs[i]->getLinkageName().str();
                if (!cp.length())
                    cp = GVs[i]->getName().str();
                setMapAddress(addr, cp);
                const DIType *DT = dyn_cast<DIType>(node);
                //if (trace_mapt)
                    printf("CAM: %s tag %s:\n", cp.c_str(), DT ? dwarf::TagString(DT->getTag()) : "notag");
node->dump();
                addItemToList(addr, EE->getDataLayout()->getTypeAllocSize(Ty), gv->getType());
                mapDwarfType(1, node, (char *)addr, 0, cp);
                mapType((char *)addr, Ty, cp);
            }
        }
    }
    //if (trace_mapt)
        dumpMemoryRegions(4010);
}
