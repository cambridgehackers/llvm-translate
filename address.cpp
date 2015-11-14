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
static std::map<void *, ADDRESSMAP_TYPE *> mapitem;
static std::map<MAPSEEN_TYPE, int, MAPSEENcomp> mapseen;
static std::map<std::string, void *> maplookup;
static struct {
    void *p;
    long size;
} callfunhack[100];

/*
 * Allocated memory region management
 */
static void addItemToList(void *p, long size)
{
    int i = 0;

    while(callfunhack[i].p)
        i++;
    callfunhack[i].p = p;
    callfunhack[i].size = size;
}

extern "C" void *llvm_translate_malloc(size_t size)
{
    size_t newsize = size * 2 + MAX_BASIC_BLOCK_FLAGS * sizeof(int) + GIANT_SIZE;
    void *ptr = malloc(newsize);
    memset(ptr, 0x5a, newsize);
    if (trace_malloc)
        printf("[%s:%d] %ld = %p\n", __FUNCTION__, __LINE__, size, ptr);
    addItemToList(ptr, newsize);
    return ptr;
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
        if (ret && !strncmp(ret, "vtable for ", 11))
            return "class." + std::string(ret+11);
    }
    return "";
}

static void mapType(char *addr, Type *Ty, std::string aname)
{
    const DataLayout *TD = EE->getDataLayout();
    if (!addr || mapseen[MAPSEEN_TYPE{addr, Ty}])
        return;
    mapseen[MAPSEEN_TYPE{addr, Ty}] = 1;
printf("[%s:%d] addr %p Ty %p = '%s' name %s\n", __FUNCTION__, __LINE__, addr, Ty, //printType(Ty, false, "", "", "").c_str()
"", aname.c_str());
    std::string nvname = getVtableName(addr);
    if (nvname != "") {
printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, nvname.c_str());
    }
    switch (Ty->getTypeID()) {
    case Type::StructTyID: {
        StructType *STy = cast<StructType>(Ty);
        const StructLayout *SLO = TD->getStructLayout(STy);
        int Idx = 0;
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
STy->dump();
        for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
            MEMBER_INFO *tptr = lookupMember(STy, Idx, dwarf::DW_TAG_member);
            if (!tptr)
                continue;    /* for templated classes, like Fifo1<int>, clang adds an int8[3] element to the end of the struct */
            int off = SLO->getElementOffset(Idx);
            Type *element = *I;
            const DIType *LTy = dyn_cast<DIType>(tptr->meta);
            std::string fname = CBEMangle(LTy->getName().str());
            if (fname.length() <= 6 || fname.substr(0, 6) != "_vptr_") {
                if (LTy->getTag() == dwarf::DW_TAG_inheritance)
                    mapType(addr, element, aname);
                else
                    mapType(addr + off, element, aname + "$$" + fname);
            }
        }
        break;
        }
    case Type::PointerTyID: {
        PointerType *PTy = cast<PointerType>(Ty);
        Type *element = PTy->getElementType();
        char *addr_target = *(char **)addr;
        std::string vname = getVtableName(addr_target);
        if (vname != "") {
printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, vname.c_str());
element->dump();
        }
        mapType(addr_target, element, aname);
        break;
        }
    default:
        break;
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
                addItemToList(addr, EE->getDataLayout()->getTypeAllocSize(Ty));
                mapDwarfType(1, node, (char *)addr, 0, cp);
                mapType((char *)addr, Ty, cp);
            }
        }
    }
    //if (trace_mapt)
        dumpMemoryRegions(4010);
}
