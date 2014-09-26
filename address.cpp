/* Copyright (c) 2014 Quanta Research Cambridge, Inc
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
#include "llvm/Linker.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Assembly/Parser.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm;

#include "declarations.h"
#define GIANT_SIZE 1024
static std::map<void *, std::string> mapitem;
static std::list<MAPTYPE_WORK> mapwork, mapwork_non_class;
static int trace_map;// = 1;
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
static void additemtolist(void *p, long size)
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
    printf("[%s:%d] %ld = %p\n", __FUNCTION__, __LINE__, size, ptr);
    additemtolist(ptr, newsize);
    return ptr;
}

static void dump_memory_regions(int arg)
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
int validate_address(int arg, void *p)
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
const char *map_address(void *arg, std::string name)
{
    const GlobalValue *g = EE->getGlobalValueAtAddress(arg);
    if (g)
        mapitem[arg] = g->getName().str();
    std::map<void *, std::string>::iterator MI = mapitem.find(arg);
    if (MI != mapitem.end())
        return MI->second.c_str();
    if (name.length() != 0) {
        mapitem[arg] = name;
        return name.c_str();
    }
    static char temp[MAX_CHAR_BUFFER];
    sprintf(temp, "%p", arg);
    return temp;
}

static void mapType(int derived, const MDNode *aCTy, char *aaddr, std::string aname)
{
    DICompositeType CTy(aCTy);
    int tag = CTy.getTag();
    long offset = (long)CTy.getOffsetInBits()/8;
    char *addr = aaddr + offset;
    char *addr_target = *(char **)addr;
    if (validate_address(5000, aaddr) || validate_address(5001, addr))
        exit(1);
    std::string name = CTy.getName().str();
    if (!name.length())
        name = CTy.getName().str();
    std::string fname = name;
    const GlobalValue *g = EE->getGlobalValueAtAddress(((uint64_t *)addr_target)-2);
    if (tag == dwarf::DW_TAG_class_type && g) {
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
                CTy = DICompositeType(classp->node);
        }
    }
    if (aname.length() > 0)
        fname = aname + ":" + name;
    if (tag == dwarf::DW_TAG_pointer_type) {
        if (addr_target && mapitem.find(addr_target) == mapitem.end()) { // process item, if not seen before
            const MDNode *node = getNode(CTy.getTypeDerivedFrom());
            DICompositeType pderiv(node);
            int ptag = pderiv.getTag();
            int pptag = 0;
            if (ptag == dwarf::DW_TAG_pointer_type)
                pptag = DICompositeType(getNode(pderiv.getTypeDerivedFrom())).getTag();
            if (pptag != dwarf::DW_TAG_subroutine_type) {
                if (validate_address(5010, addr_target))
                    exit(1);
                mapwork.push_back(MAPTYPE_WORK(0, node, addr_target, fname));
            }
        }
        return;
    }
    map_address(addr, fname);
    if (tag != dwarf::DW_TAG_subprogram && tag != dwarf::DW_TAG_subroutine_type
     && tag != dwarf::DW_TAG_class_type && tag != dwarf::DW_TAG_inheritance
     && tag != dwarf::DW_TAG_base_type) {
        if (trace_map)
            printf(" SSSStag %20s name %30s ", dwarf::TagString(tag), fname.c_str());
        if (CTy.isStaticMember()) {
            if (trace_map)
                printf("STATIC\n");
            return;
        }
        if (trace_map)
            printf("addr [%s]=val %s derived %d\n", map_address(addr, fname), map_address(addr_target, ""), derived);
    }
    if (CTy.isDerivedType() || CTy.isCompositeType()) {
        const MDNode *derivedNode = getNode(CTy.getTypeDerivedFrom());
        DICompositeType foo(derivedNode);
        DIArray Elements = CTy.getTypeArray();
        mapwork.push_back(MAPTYPE_WORK(1, derivedNode, addr, fname));
        for (unsigned k = 0, N = Elements.getNumElements(); k < N; ++k)
            mapwork.push_back(MAPTYPE_WORK(0, Elements.getElement(k), addr, fname));
    }
}

void constructAddressMap(NamedMDNode *CU_Nodes)
{
    mapwork.clear();
    mapwork_non_class.clear();
    mapitem.clear();
    if (CU_Nodes) {
        for (unsigned j = 0, e = CU_Nodes->getNumOperands(); j != e; ++j) {
            DIArray GVs = DICompileUnit(CU_Nodes->getOperand(j)).getGlobalVariables();
            for (unsigned i = 0, e = GVs.getNumElements(); i != e; ++i) {
                DIGlobalVariable DIG(GVs.getElement(i));
                const GlobalVariable *gv = DIG.getGlobal();
                std::string cp = DIG.getLinkageName().str();
                if (!cp.length())
                    cp = DIG.getName().str();
                void *addr = EE->getPointerToGlobal(gv);
                mapitem[addr] = cp;
                DICompositeType CTy(DIG.getType());
                int tag = CTy.getTag();
                Value *contextp = DIG.getContext();
                printf("%s: globalvar %s tag %s context %p addr %p\n", __FUNCTION__, cp.c_str(), dwarf::TagString(tag), contextp, addr);
                const MDNode *node = CTy;
                additemtolist(addr, EE->getDataLayout()->getTypeAllocSize(gv->getType()->getElementType()));
                if (!contextp)
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
    //dump_memory_regions(4010);
}

/*
 * Detect and allocate space for shadow variables
 */
class TypeHack: public Type {
    friend class Type;
public:
    unsigned hgetSubclassData() const { return getSubclassData(); }
    void hsetSubclassData(unsigned val) { setSubclassData(val); }
};

static std::map<StructType *, StructType *> structMap;
static StructType *classModule;
static int remapStruct(Type *intype, int inherit)
{
    int rc = 0;
    int id = intype->getTypeID();
    switch(id) {
    case Type::PointerTyID:
        return rc | remapStruct(dyn_cast<PointerType>(intype)->getPointerElementType(), inherit);
    case Type::StructTyID:
        break;
    default:
        printf("%s: unused type %d\n", __FUNCTION__, id);
    case Type::IntegerTyID:
    case Type::FunctionTyID:
    case Type::ArrayTyID:
    //case Type::HalfTyID: //case Type::FloatTyID: //case Type::DoubleTyID:
    //case Type::X86_FP80TyID: //case Type::FP128TyID: //case Type::PPC_FP128TyID:
        return rc;
    }
    StructType *arg = cast<StructType>(intype);
    std::map<StructType *, StructType *>::iterator FI = structMap.find(arg);
    if (FI == structMap.end()) {
        structMap[arg] = arg;
        if (arg->getNumElements() > 0 && arg->getElementType(0)->getTypeID() == Type::StructTyID) {
            StructType *temp = dyn_cast<StructType>(arg->getElementType(0));
            if (temp->isLayoutIdentical(classModule)) {
                printf("[%s] MATCHED %p\n", __FUNCTION__, arg);
                rc = 1;
            }
        }
        int length = arg->getNumElements() * 2 + MAX_BASIC_BLOCK_FLAGS;
        Type **data = (Type **)malloc(length * sizeof(data[0]));
        int i = 0, j = 0;
        for (StructType::element_iterator SI = arg->element_begin(), SE = arg->element_end(); SI != SE; SI++) {
            Type *Ty = *SI;
            rc |= remapStruct(Ty, 1);
            data[i++] = Ty;
        }
        for (StructType::element_iterator SI = arg->element_begin(), SE = arg->element_end(); SI != SE; SI++) {
            Type *Ty = data[j++];
            if (Ty->getTypeID() == Type::StructTyID)
                Ty = Type::getInt1Ty(arg->getContext());
            data[i++] = Ty;
        }
        for (j = 0; j < MAX_BASIC_BLOCK_FLAGS; j++)
            data[i++] = Type::getInt1Ty(arg->getContext());
        if (!inherit && rc) {
            printf("[%s:%d] REMAPME\n", __FUNCTION__, __LINE__);
            TypeHack *bozo = (TypeHack *)arg;
            int val = bozo->hgetSubclassData();
            bozo->hsetSubclassData(0);
            arg->setBody(ArrayRef<Type *>(data, length));
            bozo->hsetSubclassData(val);
        }
    }
    return rc;
}

void adjustModuleSizes(Module *Mod)
{
    classModule = Mod->getTypeByName("class.Module");
    printf("[%s:%d] classModule %p\n", __FUNCTION__, __LINE__, classModule);
    /* iterate through all global variables, adjusting size of types */
    for (Module::global_iterator MI = Mod->global_begin(), ME = Mod->global_end(); MI != ME; ++MI) {
        structMap.clear();
        if (!MI->isDeclaration() && !MI->isConstant())
            remapStruct(MI->getType(), 0);
    }
    printf("\n");
    /* iterate through all functions, adjusting size of 'new' operands */
    for (Module::iterator FI = Mod->begin(), FE = Mod->end(); FI != FE; ++FI) {
        for (Function::iterator BI = FI->begin(), BE = FI->end(); BI != BE; ++BI) {
            for (BasicBlock::iterator II = BI->begin(), IE = BI->end(); II != IE; ++II) {
                if (II->getOpcode() != Instruction::Call)
                    continue;
                Value *called = II->getOperand(II->getNumOperands()-1);
                const char *cp = called->getName().str().c_str();
                const Function *CF = dyn_cast<Function>(called);
                if (CF && CF->isDeclaration()
                 && (!strcmp(cp, "_Znwm") || !strcmp(cp, "malloc"))) {
                    printf("[%s:%d]CALL %d\n", __FUNCTION__, __LINE__, called->getValueID());
                    Type *Params[] = {Type::getInt64Ty(Mod->getContext())};
                    FunctionType *fty = FunctionType::get(
                        Type::getInt8PtrTy(Mod->getContext()),
                        ArrayRef<Type*>(Params, 1), false);
                    Value *newmalloc = Mod->getOrInsertFunction("llvm_translate_malloc", fty);
                    Function *F = dyn_cast<Function>(newmalloc);
                    Function *cc = dyn_cast<Function>(called);
                    F->setCallingConv(cc->getCallingConv());
                    F->setDoesNotAlias(0);
                    F->setAttributes(cc->getAttributes());
                    called->replaceAllUsesWith(newmalloc);
                }
            }
        }
    }
}
