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

class MAPTYPE_WORK {
public:
    int derived;
    const Metadata *CTy;
    char *addr;
    int   offset;
    std::string name;
    MAPTYPE_WORK(int deriv, const Metadata *node, char *aaddr, int off, std::string aname) {
       derived = deriv;
       CTy = node;
       addr = aaddr;
       offset = off;
       name = aname;
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

typedef struct {
    const void     *addr;
    const Metadata *type;
} MAPSEEN_TYPE;

struct seencomp {
    bool operator() (const MAPSEEN_TYPE& lhs, const MAPSEEN_TYPE& rhs) const {
        if (lhs.addr < rhs.addr)
            return true;
        if (lhs.addr > rhs.addr)
            return false;
        return lhs.type < rhs.type;
    }
};

#define GIANT_SIZE 1024
static int trace_mapt;// = 1;
static int trace_mapa;// = 1;
static int trace_malloc;// = 1;
static std::map<void *, ADDRESSMAP_TYPE *> mapitem;
static std::map<MAPSEEN_TYPE, int, seencomp> mapseen;
static std::map<std::string, void *> maplookup;
static std::list<MAPTYPE_WORK> mapwork;
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

void memdumpl(unsigned char *p, int len, const char *title)
{
int i;

    i = 0;
    while (len > 0) {
        if (!(i & 0x3f)) {
            if (i > 0)
                printf("\n");
            printf("%s: ",title);
        }
        uint64_t temp = *(uint64_t *)p;
        if (temp == 0x5a5a5a5a5a5a5a5a)
            printf("_ ");
        else
            //printf("0x%lx ", temp);
            printf("%s ", mapAddress((void *)temp, "", NULL));
        p += sizeof(uint64_t);
        i += sizeof(uint64_t);
        len -= sizeof(uint64_t);
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

static const Metadata *fetchType(const Metadata *arg)
{
    if (auto *S = dyn_cast_or_null<MDString>(arg)) {
      // Don't error on missing types (checked elsewhere).
      const DIType *DT = TypeRefs.lookup(S);
      if (!DT) {
          printf("[%s:%d] lookup named type failed\n", __FUNCTION__, __LINE__);
          exit(-1);
      }
      arg = DT;
      if (trace_mapt)
          printf("        [%s:%d] replacedmeta S %p %s = %p\n", __FUNCTION__, __LINE__, S, S->getString().str().c_str(), arg);
    }
    return arg;
}

static void pushwork(int deriv, const Metadata *node, char *aaddr, int off, std::string aname)
{
    if (trace_mapt)
        printf("PUSH: D %d N %p A %p O %d M %s *******\n", deriv, node, aaddr, off, aname.c_str());
    mapwork.push_back(MAPTYPE_WORK(deriv, node, aaddr, off, aname));
}
static std::string getVtableName(void *addr_target)
{
    if (const GlobalValue *g = EE->getGlobalValueAtAddress(((uint64_t *)addr_target)-MAGIC_VTABLE_OFFSET)) {
        int status;
        const char *ret = abi::__cxa_demangle(g->getName().str().c_str(), 0, 0, &status);
        if (!strncmp(ret, "vtable for ", 11))
            return CBEMangle("l_class." + std::string(ret+11));
    }
    return "";
}
static void mapType(int derived, const Metadata *aMeta, char *addr, int aoffset, std::string aname)
{
    int off = 0;
    std::string name;
    aMeta = fetchType(aMeta);
    if (const DICompositeType *CTy = dyn_cast<DICompositeType>(aMeta)) {
        off = CTy->getOffsetInBits()/8;
        //name = CTy->getName();
    }
    else if (const DIType *Ty = dyn_cast<DIType>(aMeta)) {
        off = Ty->getOffsetInBits()/8;
        name = Ty->getName();
    }
    else {
        printf("[%s:%d] mapType cast failed\n", __FUNCTION__, __LINE__);
        exit(-1);
    }
    int offset = aoffset + off;
    char *addr_target = *(char **)(addr + offset);
    std::string incomingAddr = mapAddress(addr, "", NULL);
    if (trace_mapt)
        printf("%s+%d+%d N %p A %p aname %s name %s D %d\n", incomingAddr.c_str(),
            offset - off, off, aMeta, addr_target, aname.c_str(), name.c_str(), derived);
    if (incomingAddr.length() < 2 || incomingAddr.substr(0,2) != "0x")
        aname = incomingAddr;
    if (validateAddress(5000, addr) || validateAddress(5001, (addr + offset)))
        exit(1);
    std::string vname = getVtableName(addr_target);
    if (vname != "")
        name = vname;
    std::string fname = name;
    if (aname != "") {
        if (name != "")
            fname = aname + "_ZZ_" + name;
        else
            fname = aname;
    }
    std::string mstr = mapAddress(addr + offset, fname, aMeta); // setup mapping!
    if (trace_mapt) {
        printf("%p @[%s]=val %s der %d\n", addr, mstr.c_str(), mapAddress(addr_target, "", NULL), derived);
        memdumpl((unsigned char *)addr + offset, 64, mapAddress(addr+offset, "", NULL));
    }
    if (auto *DT = dyn_cast<DIDerivedTypeBase>(aMeta)) {
        const Metadata *node = fetchType(DT->getRawBaseType());
        if (DT->getTag() == dwarf::DW_TAG_member || DT->getTag() == dwarf::DW_TAG_inheritance) {
            if (const DIDerivedTypeBase *DI = dyn_cast_or_null<DIDerivedTypeBase>(node)) {
                DT = DI;
                node = fetchType(DT->getRawBaseType());
            }
        }
        std::string tname;
        if (auto *DR = dyn_cast_or_null<DIDerivedType>(node))
            tname = DR->getName();
        /* Handle pointer types */
        if (DT->getTag() == dwarf::DW_TAG_pointer_type && tname != "__vtbl_ptr_type"
         && addr_target && !mapseen[MAPSEEN_TYPE{addr_target, node}]) {  // process item, if not seen before
            if (trace_mapt)
                printf("    pointer ;");
            mapseen[MAPSEEN_TYPE{addr_target, node}] = 1;
            pushwork(0, node, addr_target, 0, fname);
            if (validateAddress(5010, addr_target))
                exit(1);
            return;
        }
    }
    if (auto *CTy = dyn_cast<DICompositeType>(aMeta)) {
        mapseen[MAPSEEN_TYPE{addr_target, aMeta}] = 1;
        /* Handle class types */
        if (trace_mapt)
            printf("   CLASSSSS name %s id %s N %p A %p\n", CTy->getName().str().c_str(), CTy->getIdentifier().str().c_str(), aMeta, addr+offset);
        DINodeArray Elements = CTy->getElements();
        for (unsigned k = 0, N = Elements.size(); k < N; ++k)
            if (DIType *Ty = dyn_cast<DIType>(Elements[k])) {
                if (Ty->isStaticMember())  // don't recurse on static member elements
                    continue;
                if (trace_mapt)
                    printf("   %p+%d+%ld %s ", addr, offset, Ty->getOffsetInBits()/8, Ty->getName().str().c_str());
                int tag = Ty->getTag();
                const Metadata *node = fetchType(Ty);
                if (tag == dwarf::DW_TAG_member)
                    mapType(0, node, addr, offset, fname);
                else if (tag == dwarf::DW_TAG_inheritance)
                    mapType(1, node, addr, aoffset, fname);
            }
    }
}

/*
 * Starting from all toplevel global, construct symbolic names for
 * all reachable addresses
 */
void constructAddressMap(NamedMDNode *CU_Nodes)
{
    mapwork.clear();
    mapitem.clear();
    if (CU_Nodes) {
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
                if (trace_mapt)
                    printf("CAM: %s tag %s: ", cp.c_str(), DT ? dwarf::TagString(DT->getTag()) : "notag");
                addItemToList(addr, EE->getDataLayout()->getTypeAllocSize(gv->getType()->getElementType()));
                pushwork(1, node, (char *)addr, 0, cp);
            }
        }
    }
    // process top level classes
    while (mapwork.begin() != mapwork.end()) {
        const MAPTYPE_WORK *p = &*mapwork.begin();
        if (trace_mapt)
            printf("***** POP: D %d N %p A %p O %d M %s\n", p->derived, p->CTy, p->addr, p->offset, p->name.c_str());
        mapType(p->derived, p->CTy, p->addr, p->offset, p->name);
        mapwork.pop_front();
    }
    //if (trace_mapt)
        dumpMemoryRegions(4010);
}
