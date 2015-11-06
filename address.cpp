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

using namespace llvm;

#include "declarations.h"

class MAPTYPE_WORK {
public:
    int derived;
    const Metadata *CTy;
    char *addr;
    std::string aname;
    MAPTYPE_WORK(int a, const Metadata *b, char *c, std::string d) {
       derived = a;
       CTy = b;
       addr = c;
       aname = d;
    }
};
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
static int trace_map;// = 1;
static int trace_mapa;// = 1;
static int trace_malloc;// = 1;
static std::map<void *, ADDRESSMAP_TYPE *> mapitem;
static std::map<std::string, void *> maplookup;
static std::list<MAPTYPE_WORK> mapwork, mapwork_non_class;
static struct {
    void *p;
    long size;
} callfunhack[100];

void memdump(unsigned char *p, int len, const char *title)
{
int i;

    i = 0;
    while (len > 0) {
        if (!(i & 0xf)) {
            if (i > 0)
                printf("\n");
            printf("%s: ",title);
        }
        printf("%02x ", *p++);
        i++;
        len--;
    }
    printf("\n");
}

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

//return;
    printf("%s: %d\n", __FUNCTION__, arg);
    while(callfunhack[i].p) {
        printf("[%d] = %p\n", i, callfunhack[i].p);
        long size = callfunhack[i].size;
        if (size > GIANT_SIZE) {
           size -= GIANT_SIZE;
           size -= 10 * sizeof(int);
           size = size/2;
        }
        size += 16;
        memdump((unsigned char *)callfunhack[i].p, size, "data");
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
        printf("%p size 0x%lx\n", callfunhack[i].p, callfunhack[i].size);
        i++;
    }
    //exit(1);
    return 1;
}

/*
 * Build up reverse address map from all data items after running constructors
 */
const char *mapAddress(void *arg, std::string name, const Metadata *type)
{
    const GlobalValue *g = EE->getGlobalValueAtAddress(arg);
    std::map<void *, ADDRESSMAP_TYPE *>::iterator MI = mapitem.find(arg);
    if (MI != mapitem.end()) {
        //if (trace_mapa)
        //    printf("%s: %p = %s found\n", __FUNCTION__, arg, MI->second->name.c_str());
        return MI->second->name.c_str();
    }
    if (g) {
        name = g->getName().str();
printf("[%s:%d] OK %p = %s\n", __FUNCTION__, __LINE__, arg, name.c_str());
    }
    else
printf("[%s:%d] FAIL %p = %s\n", __FUNCTION__, __LINE__, arg, name.c_str());
    if (name.length() != 0) {
        mapitem[arg] = new ADDRESSMAP_TYPE(name, type);
        if (name != "") {
            maplookup[name] = arg;
            //printf("%s: setname %s = %p\n", __FUNCTION__, name.c_str(), arg);
        }
        if (trace_mapa) {
            const char *t = "";
            if (type) {
//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
//type->dump();
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
    if (!strncmp(name.c_str(), "0x", 2)) {
        char *endptr = NULL;
        return (void *)strtol(name.c_str()+2, &endptr, 16);
    }
    std::map<std::string, void *>::iterator MI = maplookup.find(name);
    if (MI != maplookup.end())
        return MI->second;
    return NULL;
}

static void mapType(int derived, const Metadata *aCTy, char *aaddr, std::string aname)
{
    char *addr = aaddr;
    std::string name = "";
    if (const DICompositeType *CTy = dyn_cast<DICompositeType>(aCTy)) {
        name = CTy->getName();
    }
    else if (const DIType *Ty = dyn_cast<DIType>(aCTy)) {
        //int tag = CTy->getTag();
        addr += Ty->getOffsetInBits()/8;
        //name = Ty->getName();
        //printf(" M[%s/%lld]", cp, (long long)off);
    }
    char *addr_target = *(char **)addr;
    if (validateAddress(5000, aaddr) || validateAddress(5001, addr))
        exit(1);
    std::string fname = name;
printf("[%s:%d] AEEEEEEEEEE %s\n", __FUNCTION__, __LINE__, name.c_str());
    const GlobalValue *g = EE->getGlobalValueAtAddress(((uint64_t *)addr_target)-2);
    if (g) {
        int status;
        const char *ret = abi::__cxa_demangle(g->getName().str().c_str(), 0, 0, &status);
        if (!strncmp(ret, "vtable for ", 11)) {
            name = ret+11;
            char temp[MAX_CHAR_BUFFER];
            sprintf(temp, "class.%s", name.c_str());
            CLASS_META *classp = lookup_class(temp);
            if (!classp) {
                printf("[%s:%d] ret %s\n", __FUNCTION__, __LINE__, ret);
                exit(1);
            }
            if (!derived)
                //CTy = dyn_cast<DICompositeType>(classp->node);
                aCTy = classp->node;
            std::replace(name.begin(), name.end(), ':', '_');
            std::replace(name.begin(), name.end(), '<', '_');
            std::replace(name.begin(), name.end(), '>', '_');
        }
    }
    if (aname.length() > 0 && name.length() > 0)
        fname = aname + "_ZZ_" + name;
    if (auto *DT = dyn_cast<DIDerivedTypeBase>(aCTy)) {
        if (addr_target && mapitem.find(addr_target) == mapitem.end()) { // process item, if not seen before
            
            const Metadata *node = /*getNode*/(DT->getBaseType());
            //const DICompositeType *pderiv = dyn_cast<DICompositeType>(node);
            //int ptag = pderiv->getTag();
            int pptag = 0;
            //if (ptag == dwarf::DW_TAG_pointer_type) {
            //if (auto *DT = dyn_cast<DIDerivedTypeBase>(node)) {
                //const DIType *T = dyn_cast<DIType>(/*getNode*/(DT->getBaseType()));
                //pptag = T->getTag();
            //}
            //if (pptag != dwarf::DW_TAG_subroutine_type) {
                if (validateAddress(5010, addr_target))
                    exit(1);
                mapwork.push_back(MAPTYPE_WORK(0, node, addr_target, fname));
            //}
        }
        return;
    }
printf("[%s:%d] AAAAAAAAAAAAAAAAAAAAAAAAAAaddr %p fname %s aname %s name %s\n", __FUNCTION__, __LINE__, addr, fname.c_str(), aname.c_str(), name.c_str());
    mapAddress(addr, fname, aCTy);
#if 0
if (CTy) {
    int tag = CTy->getTag();
    if (tag != dwarf::DW_TAG_subprogram && tag != dwarf::DW_TAG_subroutine_type
     && tag != dwarf::DW_TAG_class_type && tag != dwarf::DW_TAG_inheritance
     && tag != dwarf::DW_TAG_base_type) {
        if (trace_map)
            printf(" SSSStag %20s name %30s ", dwarf::TagString(tag), fname.c_str());
        if (CTy->isStaticMember()) {
            if (trace_map)
                printf("STATIC\n");
            return;
        }
        if (trace_map)
            printf("addr [%s]=val %s derived %d\n", mapAddress(addr, fname, aCTy), mapAddress(addr_target, "", NULL), derived);
    }
}
#endif
    if (auto *DT = dyn_cast<DIDerivedTypeBase>(aCTy)) {
        const Metadata *derivedNode = /*getNode*/(DT->getBaseType());
        //const DICompositeType *foo = dyn_cast<DICompositeType>(derivedNode);
        auto *SP = dyn_cast<DISubroutineType>(aCTy);
        DITypeRefArray Elements = SP->getTypeArray();
        mapwork.push_back(MAPTYPE_WORK(1, derivedNode, addr, fname));
        for (unsigned k = 0, N = Elements.size(); k < N; ++k)
            mapwork.push_back(MAPTYPE_WORK(0, /*getNode*/(Elements[k]), addr, fname));
    }
}

/*
 * Starting from all toplevel global, construct symbolic names for
 * all reachable addresses
 */
void constructAddressMap(NamedMDNode *CU_Nodes)
{
    mapwork.clear();
    mapwork_non_class.clear();
    mapitem.clear();
    if (CU_Nodes) {
        for (unsigned j = 0, e = CU_Nodes->getNumOperands(); j != e; ++j) {
            const DICompileUnit *CUP = dyn_cast<DICompileUnit>(CU_Nodes->getOperand(j));
            DIGlobalVariableArray GVs = CUP->getGlobalVariables();
            for (unsigned i = 0, e = GVs.size(); i != e; ++i) {
                const GlobalValue *gv = dyn_cast<GlobalValue>(GVs[i]->getVariable());
                std::string cp = GVs[i]->getLinkageName().str();
                if (!cp.length())
                    cp = GVs[i]->getName().str();
                void *addr = EE->getPointerToGlobal(gv);
                //mapitem[addr] = new ADDRESSMAP_TYPE(cp, GVs[i].getType());
                const Metadata *node = GVs[i]->getType();
                mapAddress(addr, cp, node);
                const DICompositeType *CTy = dyn_cast<DICompositeType>(node);
                //if (trace_map)
                    printf("%s: globalvar %s CTy %p addr %p\n", __FUNCTION__, cp.c_str(), CTy, addr);
                addItemToList(addr, EE->getDataLayout()->getTypeAllocSize(gv->getType()->getElementType()));
                if (CTy)
                    mapwork.push_back(MAPTYPE_WORK(1, node, (char *)addr, cp));
                else
                    mapwork_non_class.push_back(MAPTYPE_WORK(1, node, (char *)addr, cp));
            }
        }
    }
    // process top level classes
    while (mapwork.begin() != mapwork.end()) {
        mapType(mapwork.begin()->derived, mapwork.begin()->CTy, mapwork.begin()->addr, mapwork.begin()->aname);
        mapwork.pop_front();
    }

    // process top level non-classes
    mapwork = mapwork_non_class;
    while (mapwork.begin() != mapwork.end()) {
        mapType(mapwork.begin()->derived, mapwork.begin()->CTy, mapwork.begin()->addr, mapwork.begin()->aname);
        mapwork.pop_front();
    }
    //dumpMemoryRegions(4010);
}
