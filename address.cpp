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
    const Metadata *type;
    std::string name;
    ADDRESSMAP_TYPE(std::string aname, const Metadata *atype) {
       name = aname;
       type = atype;
    }
};

#define GIANT_SIZE 1024
static int trace_mapa;// = 1;
static int trace_malloc;// = 1;
static std::map<void *, ADDRESSMAP_TYPE *> mapitem;
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
        printf("[%d] = %p %s %s\n", i, callfunhack[i].p, gname.c_str(), mapAddress(callfunhack[i].p, "", NULL));
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
const char *mapAddress(void *arg, std::string name, const Metadata *type)
{
    const GlobalValue *g = EE->getGlobalValueAtAddress(arg);
    ADDRESSMAP_TYPE *mptr = mapitem[arg];
    if (mptr)
        return mptr->name.c_str();
    if (g)
        name = g->getName().str();
    if (name.length() != 0) {
        mapitem[arg] = new ADDRESSMAP_TYPE(name, type);
        if (name != "") {
            maplookup[name] = arg;
            //printf("%s: setname %s = %p\n", __FUNCTION__, name.c_str(), arg);
        }
        if (trace_mapa) {
            const char *t = "";
            if (type) {
               const DICompositeType *CTy = dyn_cast<DICompositeType>(type);
               t = CTy->getName().str().c_str();
            }
            printf("%s: %p = %s [%s] new\n", __FUNCTION__, arg, name.c_str(), t);
        }
        if (name[0] != '(')
            return name.c_str();
    }
    static char temp[MAX_CHAR_BUFFER];
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
    return maplookup[name];
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
                void *addr = EE->getPointerToGlobal(gv);
                const Metadata *node = fetchType(GVs[i]->getType());
                std::string cp = GVs[i]->getLinkageName().str();
                if (!cp.length())
                    cp = GVs[i]->getName().str();
                mapAddress(addr, cp, node);
                const DIType *DT = dyn_cast<DIType>(node);
                //if (trace_mapt)
                    printf("CAM: %s tag %s:\n", cp.c_str(), DT ? dwarf::TagString(DT->getTag()) : "notag");
node->dump();
gv->getType()->dump();
                addItemToList(addr, EE->getDataLayout()->getTypeAllocSize(gv->getType()->getElementType()));
                mapDwarfType(1, node, (char *)addr, 0, cp);
            }
        }
    }
    //if (trace_mapt)
        dumpMemoryRegions(4010);
}

std::string getVtableName(void *addr_target)
{
    if (const GlobalValue *g = EE->getGlobalValueAtAddress(((uint64_t *)addr_target)-MAGIC_VTABLE_OFFSET)) {
        int status;
        const char *ret = abi::__cxa_demangle(g->getName().str().c_str(), 0, 0, &status);
        if (!strncmp(ret, "vtable for ", 11))
            return CBEMangle("l_class." + std::string(ret+11));
    }
    return "";
}
