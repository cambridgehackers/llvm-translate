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
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.

#define DEBUG_TYPE "llvm-translate"

#include <stdio.h>
#include <list>
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/DebugInfo.h"
#include "llvm/Linker.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Assembly/Parser.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm;

#include "declarations.h"
#define GIANT_SIZE 1024

extern "C" struct {
    void *p;
    long size;
} callfunhack[100];
static int dump_interpret;// = 1;
static int trace_translate;// = 1;
static int trace_meta;// = 1;
static int trace_full;// = 1;
static int trace_map;// = 1;
static int output_stdout;// = 1;

static SLOTARRAY_TYPE slotarray[MAX_SLOTARRAY];
static int slotarray_index = 1;
static ExecutionEngine *EE;
static Module *Mod;
static std::map<const Value *, int> slotmap;
static std::map<const MDNode *, int> metamap;
static CLASS_META class_data[MAX_CLASS_DEFS];
static int class_data_index;

static const char *globalName, *globalGuardName;
static FILE *outputFile;
static int already_printed_header;
static char vout[MAX_CHAR_BUFFER];

static struct {
   int type;
   uint64_t value;
} operand_list[MAX_OPERAND_LIST];
static int operand_list_index;
static std::map<void *, std::string> mapitem;
static std::list<MAPTYPE_WORK> mapwork, mapwork_non_class;
static std::list<VTABLE_WORK> vtablework;
static std::map<const Value *, Value *> cloneVmap;
static int slevel, dtlevel;
static NamedMDNode *CU_Nodes;

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
extern "C" void additemtolist(void *p, long size)
{
    int i = 0;

    while(callfunhack[i].p)
        i++;
    callfunhack[i].p = p;
    callfunhack[i].size = size;
}
static void callfun(int arg)
{
    int i = 0;

return;
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
static int validate_address(int arg, void *p)
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

static bool endswith(const char *str, const char *suffix)
{
    int skipl = strlen(str) - strlen(suffix);
    return skipl >= 0 && !strcmp(str + skipl, suffix);
}
static void makeLocalSlot(const Value *V)
{
  slotmap.insert(std::pair<const Value *, int>(V, slotarray_index++));
}
static int getLocalSlot(const Value *V)
{
  std::map<const Value *, int>::iterator FI = slotmap.find(V);
  if (FI == slotmap.end()) {
     makeLocalSlot(V);
     return getLocalSlot(V);
  }
  return (int)FI->second;
}

void dump_class_data()
{
  CLASS_META *classp = class_data;
  for (int i = 0; i < class_data_index; i++) {
    printf("class %s node %p inherit %p; ", classp->name, classp->node, classp->inherit);
    for (std::list<const MDNode *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
        DIType Ty(*MI);
        DISubprogram CTy(*MI);
        uint64_t off = Ty.getOffsetInBits()/8;
        const char *cp = CTy.getLinkageName().str().c_str();
        if (Ty.getTag() != dwarf::DW_TAG_subprogram || !strlen(cp))
            cp = Ty.getName().str().c_str();
        printf(" %s/%lld", cp, (long long)off);
    }
    printf("\n");
    classp++;
  }
}
static CLASS_META *lookup_class(const char *cp)
{
  CLASS_META *classp = class_data;
  for (int i = 0; i < class_data_index; i++) {
    if (!strcmp(cp, classp->name))
        return classp;
    classp++;
  }
  return NULL;
}
const MDNode *lookup_class_member(const char *cp, uint64_t Total)
{
  CLASS_META *classp = lookup_class(cp);
  if (!classp)
      return NULL;
  for (std::list<const MDNode *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
      DIType Ty(*MI);
      if (Ty.getOffsetInBits()/8 == Total)
          return *MI;
  }
  return NULL;
}
static int lookup_method(const char *classname, std::string methodname)
{
  if (trace_translate)
      printf("[%s:%d] class %s meth %s\n", __FUNCTION__, __LINE__, classname, methodname.c_str());
  CLASS_META *classp = lookup_class(classname);
  if (!classp)
      return -1;
  for (std::list<const MDNode *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
      DISubprogram Ty(*MI);
      if (Ty.getTag() == dwarf::DW_TAG_subprogram && Ty.getName().str() == methodname)
          return Ty.getVirtualIndex();
  }
  return -1;
}
static int lookup_field(const char *classname, std::string methodname)
{
  CLASS_META *classp = lookup_class(classname);
  if (!classp)
      return -1;
  for (std::list<const MDNode *>::iterator MI = classp->memberl.begin(), ME = classp->memberl.end(); MI != ME; MI++) {
      DIType Ty(*MI);
      if (Ty.getTag() == dwarf::DW_TAG_member && Ty.getName().str() == methodname)
          return Ty.getOffsetInBits()/8;
  }
  return -1;
}

static const char *intmap_lookup(INTMAP_TYPE *map, int value)
{
    while (map->name) {
        if (map->value == value)
            return map->name;
        map++;
    }
    return "unknown";
}
static const MDNode *getNode(const Value *val)
{
    if (val)
        return dyn_cast<MDNode>(val);
    return NULL;
}

/*
 * Process dwarf metadata
 */
static std::string getScope(const Value *val)
{
    const MDNode *Node = getNode(val);
    if (!Node)
        return "";
    DIType nextitem(Node);
    std::string name = getScope(nextitem.getContext()) + nextitem.getName().str();
    //int ind = name.find("<");
    //if (ind >= 0)
        //name = name.substr(0, ind);
    return name + "::";
}
static void dumpType(DIType litem, CLASS_META *classp);
static void dumpTref(const MDNode *Node, CLASS_META *classp)
{
    if (!Node)
        return;
    DIType nextitem(Node);
    int tag = nextitem.getTag();
    std::string name = nextitem.getName().str();
    std::map<const MDNode *, int>::iterator FI = metamap.find(Node);
    if (FI == metamap.end()) {
        metamap[Node] = 1;
        if (tag == dwarf::DW_TAG_class_type) {
            classp = &class_data[class_data_index++];
            classp->node = Node;
            classp->name = strdup(("class." + getScope(nextitem.getContext()) + name).c_str());
            int ind = name.find("<");
            if (ind >= 0) { /* also insert the class w/o template parameters */
                classp = &class_data[class_data_index++];
                *classp = *(classp-1);
                name = name.substr(0, ind);
                classp->name = strdup(("class." + getScope(nextitem.getContext()) + name).c_str());
            }
        }
        dumpType(nextitem, classp);
    }
}

static void dumpType(DIType litem, CLASS_META *classp)
{
    int tag = litem.getTag();
    if (!tag)     // Ignore elements with tag of 0
        return;
    if (tag == dwarf::DW_TAG_pointer_type || tag == dwarf::DW_TAG_inheritance) {
        dtlevel++;
        DICompositeType CTy(litem);
        const MDNode *Node = getNode(CTy.getTypeDerivedFrom());
        if (tag == dwarf::DW_TAG_inheritance && Node && classp) {
            if(classp->inherit) {
                printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                exit(1);
            }
            classp->inherit = Node;
        }
        dumpTref(Node, classp);
        dtlevel--;
        return;
    }
    if (trace_meta)
        printf("%d tag %s name %s off %3ld size %3ld", dtlevel,
            dwarf::TagString(tag), litem.getName().str().c_str(),
            (long)litem.getOffsetInBits()/8, (long)litem.getSizeInBits()/8);
    if (litem.getTag() == dwarf::DW_TAG_subprogram) {
        DISubprogram sub(litem);
        if (trace_meta)
            printf(" link %s", sub.getLinkageName().str().c_str());
    }
    if (trace_meta)
        printf("\n");
    if (litem.isCompositeType()) {
        dtlevel++;
        DICompositeType CTy(litem);
        DIArray Elements = CTy.getTypeArray();
        if (tag != dwarf::DW_TAG_subroutine_type) {
            dumpTref(getNode(CTy.getTypeDerivedFrom()), classp);
            dumpTref(getNode(CTy.getContainingType()), classp);
        }
        for (unsigned k = 0, N = Elements.getNumElements(); k < N; ++k) {
            DIType Ty(Elements.getElement(k));
            int tag = Ty.getTag();
            if (tag == dwarf::DW_TAG_member || tag == dwarf::DW_TAG_subprogram) {
                const MDNode *Node = Ty;
                if (classp)
                    classp->memberl.push_back(Node);
            }
            dumpType(Ty, classp);
        }
        dtlevel--;
    }
}

static void process_metadata(NamedMDNode *CU_Nodes)
{
  for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
    DICompileUnit CU(CU_Nodes->getOperand(i));
    printf("\n%s: compileunit %d:%s %s\n", __FUNCTION__, CU.getLanguage(),
         // from DIScope:
         CU.getDirectory().str().c_str(), CU.getFilename().str().c_str());
    DIArray SPs = CU.getSubprograms();
    for (unsigned i = 0, e = SPs.getNumElements(); i != e; ++i) {
      DISubprogram sub(SPs.getElement(i));
          if (trace_meta)
          printf("Subprogram: %s %s %d %d %d %d\n", sub.getName().str().c_str(),
              sub.getLinkageName().str().c_str(), sub.getVirtuality(),
              sub.getVirtualIndex(), sub.getFlags(), sub.getScopeLineNumber());
          dumpTref(getNode(sub.getContainingType()), NULL);
      DIArray tparam(sub.getTemplateParams());
      if (tparam.getNumElements() > 0) {
          if (trace_meta) {
              printf("tparam: ");
              for (unsigned j = 0, je = tparam.getNumElements(); j != je; j++) {
                 DIDescriptor ee(tparam.getElement(j));
                 printf("[%s:%d] %d/%d\n", __FUNCTION__, __LINE__, j, je);
                 ee.dump();
              }
          }
      }
      dumpType(DICompositeType(sub.getType()), NULL);
    }
    DIArray EnumTypes = CU.getEnumTypes();
    for (unsigned i = 0, e = EnumTypes.getNumElements(); i != e; ++i) {
      if (trace_meta)
      printf("[%s:%d]enumtypes\n", __FUNCTION__, __LINE__);
      dumpType(DIType(EnumTypes.getElement(i)), NULL);
    }
    DIArray RetainedTypes = CU.getRetainedTypes();
    for (unsigned i = 0, e = RetainedTypes.getNumElements(); i != e; ++i) {
      if (trace_meta)
      printf("[%s:%d]retainedtypes\n", __FUNCTION__, __LINE__);
      dumpType(DIType(RetainedTypes.getElement(i)), NULL);
    }
    DIArray Imports = CU.getImportedEntities();
    for (unsigned i = 0, e = Imports.getNumElements(); i != e; ++i) {
      DIImportedEntity Import = DIImportedEntity(Imports.getElement(i));
      DIDescriptor Entity = Import.getEntity();
      if (Entity.isType()) {
        if (trace_meta)
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        dumpType(DIType(Entity), NULL);
      }
      else if (Entity.isSubprogram()) {
        if (trace_meta) {
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        DISubprogram(Entity)->dump();
        }
      }
      else if (Entity.isNameSpace()) {
        //DINameSpace(Entity).getContext()->dump();
      }
      else
        printf("[%s:%d] entity not type/subprog/namespace\n", __FUNCTION__, __LINE__);
    }
    DIArray GVs = CU.getGlobalVariables();
    for (unsigned i = 0, e = GVs.getNumElements(); i != e; ++i) {
      DIGlobalVariable DIG(GVs.getElement(i));
      const GlobalVariable *gv = DIG.getGlobal();
      //const Constant *con = DIG.getConstant();
      const Value *val = dyn_cast<Value>(gv);
      std::string cp = DIG.getLinkageName().str();
      if (!cp.length())
          cp = DIG.getName().str();
      if (trace_meta)
          printf("%s: globalvar: %s GlobalVariable %p type %d\n", __FUNCTION__, cp.c_str(), gv, val->getType()->getTypeID());
    }
  }
}

/*
 * Build up reverse address map from all data items after running constructors
 */

static const char *map_address(void *arg, std::string name)
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
//printf("[%s:%d] tag %s addr %p offset %ld addr_target %p name %s\n", __FUNCTION__, __LINE__, dwarf::TagString(tag), addr, offset, addr_target, aname.c_str());
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
        const MDNode *node = getNode(CTy.getTypeDerivedFrom());
        DICompositeType pderiv(node);
        int ptag = pderiv.getTag();
        if (addr_target && mapitem.find(addr_target) == mapitem.end()) // process item, if not seen before
 {
        int pptag = 0;
if (ptag == dwarf::DW_TAG_pointer_type) {
        DICompositeType ppderiv(getNode(pderiv.getTypeDerivedFrom()));
        pptag = ppderiv.getTag();
}
        if (pptag != dwarf::DW_TAG_subroutine_type) {
            if (validate_address(5010, addr_target)) {
                CTy.dump();
        DICompositeType ppderiv(getNode(pderiv.getTypeDerivedFrom()));
                ppderiv.dump();
                exit(1);
            }
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
            printf(" %d SSSStag %20s name %30s ", slevel, dwarf::TagString(tag), fname.c_str());
        if (CTy.isStaticMember()) {
            if (trace_map)
                printf("STATIC\n");
            return;
        }
        if (trace_map)
            printf("addr [%s]=val %s derived %d\n", map_address(addr, fname), map_address(addr_target, ""), derived);
    }
    if (CTy.isDerivedType() || CTy.isCompositeType()) {
        slevel++;
        const MDNode *derivedNode = getNode(CTy.getTypeDerivedFrom());
        DICompositeType foo(derivedNode);
        DIArray Elements = CTy.getTypeArray();
        //if (tag != dwarf::DW_TAG_subroutine_type && tag != dwarf::DW_TAG_member && derivedNode)
        mapwork.push_back(MAPTYPE_WORK(1, derivedNode, addr, fname));
        for (unsigned k = 0, N = Elements.getNumElements(); k < N; ++k)
            mapwork.push_back(MAPTYPE_WORK(0, Elements.getElement(k), addr, fname));
        slevel--;
    }
}

static void constructAddressMap(void)
{
  mapwork.clear();
  mapwork_non_class.clear();
  mapitem.clear();
  if (CU_Nodes) {
      for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
        DICompileUnit CU(CU_Nodes->getOperand(i));
        DIArray GVs = CU.getGlobalVariables();
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
          additemtolist(addr, sizeof(long));
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
}

static bool opt_runOnBasicBlock(BasicBlock &BB)
{
    bool changed = false;
    BasicBlock::iterator Start = BB.getFirstInsertionPt();
    BasicBlock::iterator E = BB.end();
    if (Start == E) return false;
    BasicBlock::iterator I = Start++;
    while(1) {
        BasicBlock::iterator PI = llvm::next(BasicBlock::iterator(I));
        int opcode = I->getOpcode();
        Value *retv = (Value *)I;
        //printf("[%s:%d] OP %d %p;", __FUNCTION__, __LINE__, opcode, retv);
        //for (unsigned i = 0;  i < I->getNumOperands(); ++i) {
            //printf(" %p", I->getOperand(i));
        //}
        //printf("\n");
        switch (opcode) {
        case Instruction::Load:
            {
            const char *cp = I->getOperand(0)->getName().str().c_str();
            if (!strcmp(cp, "this")) {
                retv->replaceAllUsesWith(I->getOperand(0));
                I->eraseFromParent(); // delete "Load 'this'" instruction
                changed = true;
            }
            }
            break;
        case Instruction::Alloca:
            if (I->hasName()) {
                Value *newt = NULL;
                BasicBlock::iterator PN = PI;
                while (PN != E) {
                    BasicBlock::iterator PNN = llvm::next(BasicBlock::iterator(PN));
                    if (PN->getOpcode() == Instruction::Store && retv == PN->getOperand(1))
                        newt = PN->getOperand(0);
                    for (User::op_iterator OI = PN->op_begin(), OE = PN->op_end(); OI != OE; ++OI) {
                        if (*OI == retv && newt)
                            *OI = newt;
                    }
                    if (PN->getOpcode() == Instruction::Store && PN->getOperand(0) == PN->getOperand(1)) {
                        if (PI == PN)
                            PI = PNN;
                        PN->eraseFromParent(); // delete Store instruction
                    }
                    PN = PNN;
                }
                I->eraseFromParent(); // delete Alloca instruction
                changed = true;
            }
            break;
        case Instruction::Call:
            if (const CallInst *CI = dyn_cast<CallInst>(I)) {
              const Value *Operand = CI->getCalledValue();
                if (Operand->hasName() && isa<Constant>(Operand)) {
                  const char *cp = Operand->getName().str().c_str();
                  if (!strcmp(cp, "llvm.dbg.declare") /*|| !strcmp(cp, "printf")*/) {
                      I->eraseFromParent(); // delete this instruction
                      changed = true;
                  }
                }
            }
            break;
        };
        if (PI == E)
            break;
        I = PI;
    }
    return changed;
}

static void prepareOperand(const Value *Operand)
{
    if (!Operand)
        return;
    int slotindex = getLocalSlot(Operand);
    operand_list[operand_list_index].value = slotindex;
    if (Operand->hasName()) {
        if (isa<Constant>(Operand)) {
            operand_list[operand_list_index].type = OpTypeExternalFunction;
            slotarray[slotindex].name = Operand->getName().str().c_str();
            goto retlab;
        }
    }
    else {
        const Constant *CV = dyn_cast<Constant>(Operand);
        if (CV && !isa<GlobalValue>(CV)) {
            if (const ConstantInt *CI = dyn_cast<ConstantInt>(CV)) {
                operand_list[operand_list_index].type = OpTypeInt;
                slotarray[slotindex].offset = CI->getZExtValue();
                goto retlab;
            }
            printf("[%s:%d] non integer constant\n", __FUNCTION__, __LINE__);
        }
        if (dyn_cast<MDNode>(Operand) || dyn_cast<MDString>(Operand)
         || Operand->getValueID() == Value::PseudoSourceValueVal ||
            Operand->getValueID() == Value::FixedStackPseudoSourceValueVal) {
            printf("[%s:%d] MDNode/MDString/Pseudo\n", __FUNCTION__, __LINE__);
        }
    }
    operand_list[operand_list_index].type = OpTypeLocalRef;
retlab:
    operand_list_index++;
}

static const char *getparam(int arg)
{
   char temp[MAX_CHAR_BUFFER];
   temp[0] = 0;
   if (operand_list[arg].type == OpTypeLocalRef)
       return slotarray[operand_list[arg].value].name;
   else if (operand_list[arg].type == OpTypeExternalFunction)
       return slotarray[operand_list[arg].value].name;
   else if (operand_list[arg].type == OpTypeInt)
       sprintf(temp, "%lld", (long long)slotarray[operand_list[arg].value].offset);
   else if (operand_list[arg].type == OpTypeString)
       return (const char *)operand_list[arg].value;
   return strdup(temp);
}

static Instruction *cloneTree(const Instruction *I, Instruction *insertPoint)
{
    std::string NameSuffix = "foosuff";
    Instruction *NewInst = I->clone();

    if (I->hasName())
        NewInst->setName(I->getName()+NameSuffix);
    for (unsigned OI = 0, E = I->getNumOperands(); OI != E; ++OI) {
        const Value *oval = I->getOperand(OI);
        if (cloneVmap.find(oval) == cloneVmap.end()) {
            if (const Instruction *IC = dyn_cast<Instruction>(oval))
                cloneVmap[oval] = cloneTree(IC, insertPoint);
            else
                continue;
        }
        NewInst->setOperand(OI, cloneVmap[oval]);
    }
    NewInst->insertBefore(insertPoint);
    return NewInst;
}

Instruction *copyFunction(Instruction *TI, const Instruction *I, int methodIndex, Type *returnType)
{
    cloneVmap.clear();
    Function *TargetF = TI->getParent()->getParent();
    const Function *SourceF = I->getParent()->getParent();
    Function::arg_iterator TargetA = TargetF->arg_begin();
    for (Function::const_arg_iterator AI = SourceF->arg_begin(),
             AE = SourceF->arg_end(); AI != AE; ++AI, ++TargetA)
        cloneVmap[AI] = TargetA;
    if (!returnType)
        return cloneTree(I, TI);
    Instruction *orig_thisp = dyn_cast<Instruction>(I->getOperand(0));
    Instruction *thisp = cloneTree(orig_thisp, TI);
    Type *Params[] = {thisp->getType()};
    Type *castType = PointerType::get(
             PointerType::get(
                 PointerType::get(
                     FunctionType::get(returnType,
                           ArrayRef<Type*>(Params, 1), false),
                     0), 0), 0);
    IRBuilder<> builder(TI->getParent());
    builder.SetInsertPoint(TI);
    Value *vtabbase = builder.CreateLoad(
             builder.CreateBitCast(thisp, castType));
    Value *newCall = builder.CreateCall(
             builder.CreateLoad(
                 builder.CreateConstInBoundsGEP1_32(
                     vtabbase, methodIndex)), thisp);
    return dyn_cast<Instruction>(newCall);
}

static void calculateGuardUpdate(Function ***parent_thisp, Instruction &I)
{
  int opcode = I.getOpcode();
  switch (opcode) {
  // Terminators
  case Instruction::Br:
      {
      if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
        const BranchInst &BI(cast<BranchInst>(I));
        prepareOperand(BI.getCondition());
        int cond_item = getLocalSlot(BI.getCondition());
        char temp[MAX_CHAR_BUFFER];
        sprintf(temp, "%s" SEPARATOR "%s_cond", globalName, I.getParent()->getName().str().c_str());
        if (slotarray[cond_item].name) {
            slotarray[cond_item].name = strdup(temp);
        }
        prepareOperand(BI.getSuccessor(0));
        prepareOperand(BI.getSuccessor(1));
      } else if (isa<IndirectBrInst>(I)) {
        for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i) {
          prepareOperand(I.getOperand(i));
        }
      }
      }
      break;

  // Memory instructions...
  case Instruction::Store:
      printf("%s: STORE %s=%s\n", __FUNCTION__, getparam(2), getparam(1));
      if (operand_list[1].type == OpTypeLocalRef && !slotarray[operand_list[1].value].svalue)
          operand_list[1].type = OpTypeInt;
      if (operand_list[1].type != OpTypeLocalRef || operand_list[2].type != OpTypeLocalRef)
          printf("%s: STORE %s;", __FUNCTION__, getparam(2));
      else
          slotarray[operand_list[2].value] = slotarray[operand_list[1].value];
      break;

  // Convert instructions...
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::BitCast:
      slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
      break;

  // Other instructions...
  case Instruction::PHI:
      {
      char temp[MAX_CHAR_BUFFER];
      const PHINode *PN = dyn_cast<PHINode>(&I);
      I.getType()->dump();
      sprintf(temp, "%s" SEPARATOR "%s_phival", globalName, I.getParent()->getName().str().c_str());
      slotarray[operand_list[0].value].name = strdup(temp);
      for (unsigned op = 0, Eop = PN->getNumIncomingValues(); op < Eop; ++op) {
          int valuein = getLocalSlot(PN->getIncomingValue(op));
          prepareOperand(PN->getIncomingValue(op));
          prepareOperand(PN->getIncomingBlock(op));
          TerminatorInst *TI = PN->getIncomingBlock(op)->getTerminator();
          //printf("[%s:%d] terminator\n", __FUNCTION__, __LINE__);
          //TI->dump();
          const BranchInst *BI = dyn_cast<BranchInst>(TI);
          const char *trailch = "";
          if (isa<BranchInst>(TI) && cast<BranchInst>(TI)->isConditional()) {
            prepareOperand(BI->getCondition());
            int cond_item = getLocalSlot(BI->getCondition());
            sprintf(temp, "%s ?", slotarray[cond_item].name);
            trailch = ":";
            //prepareOperand(BI->getSuccessor(0));
            //prepareOperand(BI->getSuccessor(1));
          }
          if (slotarray[valuein].name)
              sprintf(temp, "%s %s", slotarray[valuein].name, trailch);
          else
              sprintf(temp, "%lld %s", (long long)slotarray[valuein].offset, trailch);
      }
      }
      break;
  case Instruction::Call:
      {
      int tcall = operand_list[operand_list_index-1].value; // Callee is _last_ operand
      Function *f = (Function *)slotarray[tcall].svalue;
      Function ***thisp = (Function ***)slotarray[operand_list[1].value].svalue;
      if (!f) {
          printf("[%s:%d] not an instantiable call!!!!\n", __FUNCTION__, __LINE__);
          break;
      }
      if (trace_translate)
          printf("%s: CALL %d %s %p\n", __FUNCTION__, I.getType()->getTypeID(), f->getName().str().c_str(), thisp);
      const Instruction *IC = dyn_cast<Instruction>(I.getOperand(0));
      const Type *p1 = IC->getOperand(0)->getType()->getPointerElementType();
      const StructType *STy = cast<StructType>(p1->getPointerElementType());
      const char *tname = strdup(STy->getName().str().c_str());
      int guardName = lookup_method(tname, "guard");
      int updateName = lookup_method(tname, "update");
      printf("[%s:%d] guard %d update %d\n", __FUNCTION__, __LINE__, guardName, updateName);
      const GlobalValue *g = EE->getGlobalValueAtAddress(parent_thisp[0] - 2);
      printf("[%s:%d] %p g %p\n", __FUNCTION__, __LINE__, parent_thisp, g);
      int parentGuardName = -1;
      int parentUpdateName = -1;
      if (g) {
          char temp[MAX_CHAR_BUFFER];
          int status;
          const char *ret = abi::__cxa_demangle(g->getName().str().c_str(), 0, 0, &status);
          sprintf(temp, "class.%s", ret+11);
          parentGuardName = lookup_method(temp, "guard");
          parentUpdateName = lookup_method(temp, "update");
      }
      if (guardName >= 0 && parentGuardName >= 0) {
          Function *peer_guard = parent_thisp[0][parentGuardName];
          TerminatorInst *TI = peer_guard->begin()->getTerminator();
          Instruction *newI = copyFunction(TI, &I, guardName, Type::getInt1Ty(TI->getContext()));
          if (CallInst *nc = dyn_cast<CallInst>(newI))
              nc->addAttribute(AttributeSet::ReturnIndex, Attribute::ZExt);
          Value *cond = TI->getOperand(0);
          const ConstantInt *CI = dyn_cast<ConstantInt>(cond);
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
      if (parentUpdateName >= 0) {
          Function *peer_update = parent_thisp[0][parentUpdateName];
          TerminatorInst *TI = peer_update->begin()->getTerminator();
          if (updateName >= 0)
              copyFunction(TI, &I, updateName, Type::getVoidTy(TI->getContext()));
          else if (I.use_empty()) {
              copyFunction(TI, &I, 0, NULL); // Move this call to the 'update()' method
              I.eraseFromParent(); // delete "Call" instruction
          }
      }
if (operand_list_index <= 3)
      vtablework.push_back(VTABLE_WORK(slotarray[tcall].offset/sizeof(uint64_t),
          (Function ***)slotarray[operand_list[1].value].svalue,
          (operand_list_index > 3) ? slotarray[operand_list[2].value] : SLOTARRAY_TYPE()));
      slotarray[operand_list[0].value].name = strdup(f->getName().str().c_str());
      }
      break;
  default:
      printf("Other opcode %d.=%s\n", opcode, I.getOpcodeName());
      exit(1);
  case Instruction::Ret: case Instruction::Unreachable:
  case Instruction::Add: case Instruction::FAdd:
  case Instruction::Sub: case Instruction::FSub:
  case Instruction::Mul: case Instruction::FMul:
  case Instruction::UDiv: case Instruction::SDiv: case Instruction::FDiv:
  case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
  case Instruction::And: case Instruction::Or: case Instruction::Xor:
  case Instruction::ICmp:
      break;
  }
}

static void generateVerilog(Function ***thisp, Instruction &I)
{
  int dump_operands = trace_full;
  int opcode = I.getOpcode();
  switch (opcode) {
  // Terminators
  case Instruction::Ret:
      if (I.getParent()->getParent()->getReturnType()->getTypeID()
             == Type::IntegerTyID && operand_list_index > 1) {
          operand_list[0].type = OpTypeString;
          operand_list[0].value = (uint64_t)getparam(1);
          sprintf(vout, "%s = %s;", globalName, getparam(1));
      }
      break;
  case Instruction::Br:
      {
      if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
        char temp[MAX_CHAR_BUFFER];
        const BranchInst &BI(cast<BranchInst>(I));
        prepareOperand(BI.getCondition());
        int cond_item = getLocalSlot(BI.getCondition());
        sprintf(temp, "%s" SEPARATOR "%s_cond", globalName, I.getParent()->getName().str().c_str());
        if (slotarray[cond_item].name) {
            sprintf(vout, "%s = %s\n", temp, slotarray[cond_item].name);
            slotarray[cond_item].name = strdup(temp);
        }
        prepareOperand(BI.getSuccessor(0));
        prepareOperand(BI.getSuccessor(1));
      } else if (isa<IndirectBrInst>(I)) {
        for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i) {
          prepareOperand(I.getOperand(i));
        }
      }
      dump_operands = 1;
      }
      break;
  //case Instruction::Switch:
      //const SwitchInst& SI(cast<SwitchInst>(I));
      //prepareOperand(SI.getCondition());
      //prepareOperand(SI.getDefaultDest());
      //for (SwitchInst::ConstCaseIt i = SI.case_begin(), e = SI.case_end(); i != e; ++i) {
        //prepareOperand(i.getCaseValue());
        //prepareOperand(i.getCaseSuccessor());
      //}
  //case Instruction::IndirectBr:
  //case Instruction::Invoke:
  //case Instruction::Resume:
  case Instruction::Unreachable:
      break;

  // Standard binary operators...
  case Instruction::Add: case Instruction::FAdd:
  case Instruction::Sub: case Instruction::FSub:
  case Instruction::Mul: case Instruction::FMul:
  case Instruction::UDiv: case Instruction::SDiv: case Instruction::FDiv:
  case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
  // Logical operators...
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
      {
      const char *op1 = getparam(1), *op2 = getparam(2);
      char temp[MAX_CHAR_BUFFER];
      sprintf(temp, "((%s) %s (%s))", op1, intmap_lookup(opcodeMap, opcode), op2);
      if (operand_list[0].type != OpTypeLocalRef) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      slotarray[operand_list[0].value].name = strdup(temp);
      }
      break;

  // Memory instructions...
  case Instruction::Store:
      if (operand_list[1].type == OpTypeLocalRef && !slotarray[operand_list[1].value].svalue)
          operand_list[1].type = OpTypeInt;
      if (operand_list[1].type != OpTypeLocalRef || operand_list[2].type != OpTypeLocalRef)
          sprintf(vout, "%s = %s;", getparam(2), getparam(1));
      else
          slotarray[operand_list[2].value] = slotarray[operand_list[1].value];
      break;
  //case Instruction::AtomicCmpXchg:
  //case Instruction::AtomicRMW:
  //case Instruction::Fence:

  // Convert instructions...
  //case Instruction::SExt:
  //case Instruction::FPTrunc: //case Instruction::FPExt:
  //case Instruction::FPToUI: //case Instruction::FPToSI:
  //case Instruction::UIToFP: //case Instruction::SIToFP:
  //case Instruction::IntToPtr: //case Instruction::PtrToInt:
  case Instruction::Trunc: case Instruction::ZExt: case Instruction::BitCast:
      if(operand_list[0].type != OpTypeLocalRef || operand_list[1].type != OpTypeLocalRef) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
      break;
  //case Instruction::AddrSpaceCast:

  // Other instructions...
  case Instruction::ICmp: case Instruction::FCmp:
      {
      const char *op1 = getparam(1), *op2 = getparam(2);
      const CmpInst *CI;
      if (!(CI = dyn_cast<CmpInst>(&I)) || operand_list[0].type != OpTypeLocalRef) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      char temp[MAX_CHAR_BUFFER];
      sprintf(temp, "((%s) %s (%s))", op1, intmap_lookup(predText, CI->getPredicate()), op2);
      slotarray[operand_list[0].value].name = strdup(temp);
      }
      break;
  case Instruction::PHI:
      {
      char temp[MAX_CHAR_BUFFER];
      const PHINode *PN = dyn_cast<PHINode>(&I);
      if (!PN) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      I.getType()->dump();
      sprintf(temp, "%s" SEPARATOR "%s_phival", globalName, I.getParent()->getName().str().c_str());
      sprintf(vout, "%s = ", temp);
      slotarray[operand_list[0].value].name = strdup(temp);
      for (unsigned op = 0, Eop = PN->getNumIncomingValues(); op < Eop; ++op) {
          int valuein = getLocalSlot(PN->getIncomingValue(op));
          prepareOperand(PN->getIncomingValue(op));
          prepareOperand(PN->getIncomingBlock(op));
          TerminatorInst *TI = PN->getIncomingBlock(op)->getTerminator();
          //printf("[%s:%d] terminator\n", __FUNCTION__, __LINE__);
          //TI->dump();
          const BranchInst *BI = dyn_cast<BranchInst>(TI);
          const char *trailch = "";
          if (isa<BranchInst>(TI) && cast<BranchInst>(TI)->isConditional()) {
            prepareOperand(BI->getCondition());
            int cond_item = getLocalSlot(BI->getCondition());
            sprintf(temp, "%s ?", slotarray[cond_item].name);
            trailch = ":";
            //prepareOperand(BI->getSuccessor(0));
            //prepareOperand(BI->getSuccessor(1));
            strcat(vout, temp);
          }
          if (slotarray[valuein].name)
              sprintf(temp, "%s %s", slotarray[valuein].name, trailch);
          else
              sprintf(temp, "%lld %s", (long long)slotarray[valuein].offset, trailch);
          strcat(vout, temp);
      }
      dump_operands = 1;
      }
      break;
  //case Instruction::Select:
  case Instruction::Call:
      {
      //const CallInst *CI = dyn_cast<CallInst>(&I);
      //const Value *val = CI->getCalledValue();
      int tcall = operand_list[operand_list_index-1].value; // Callee is _last_ operand
      Function *f = (Function *)slotarray[tcall].svalue;
      if (!f) {
          printf("[%s:%d] not an instantiable call!!!!\n", __FUNCTION__, __LINE__);
          break;
      }
      SLOTARRAY_TYPE arg;
      if (operand_list_index > 3)
          arg = slotarray[operand_list[2].value];
else
      vtablework.push_back(VTABLE_WORK(slotarray[tcall].offset/sizeof(uint64_t),
          (Function ***)slotarray[operand_list[1].value].svalue, arg));
      slotarray[operand_list[0].value].name = strdup(f->getName().str().c_str());
      }
      break;
  //case Instruction::Shl:
  //case Instruction::LShr:
  //case Instruction::AShr:
  //case Instruction::VAArg:
  //case Instruction::ExtractElement:
  //case Instruction::InsertElement:
  //case Instruction::ShuffleVector:
  //case Instruction::ExtractValue:
  //case Instruction::InsertValue:
  //case Instruction::LandingPad:
  default:
      printf("Other opcode %d.=%s\n", opcode, I.getOpcodeName());
      exit(1);
      break;
  }
  if (dump_operands && trace_translate)
  for (int i = 0; i < operand_list_index; i++) {
      int t = operand_list[i].value;
      if (operand_list[i].type == OpTypeLocalRef)
          printf(" op[%d]L=%d:%p:%lld:[%p=%s];", i, t, slotarray[t].svalue, (long long)slotarray[t].offset, slotarray[t].name, slotarray[t].name);
      else if (operand_list[i].type != OpTypeNone)
          printf(" op[%d]=%s;", i, getparam(i));
  }
}

static uint64_t getOperandValue(const Value *Operand)
{
  const Constant *CV = dyn_cast<Constant>(Operand);
  const ConstantInt *CI;

  if (CV && !isa<GlobalValue>(CV) && (CI = dyn_cast<ConstantInt>(CV)))
      return CI->getZExtValue();
  printf("[%s:%d]\n", __FUNCTION__, __LINE__);
  exit(1);
}

static uint64_t executeGEPOperation(gep_type_iterator I, gep_type_iterator E)
{
  const DataLayout *TD = EE->getDataLayout();
  uint64_t Total = 0;
  for (; I != E; ++I) {
      if (StructType *STy = dyn_cast<StructType>(*I)) {
          const StructLayout *SLO = TD->getStructLayout(STy);
          const ConstantInt *CPU = cast<ConstantInt>(I.getOperand());
          Total += SLO->getElementOffset(CPU->getZExtValue());
      } else {
          SequentialType *ST = cast<SequentialType>(*I);
          // Get the index number for the array... which must be long type...
          Total += TD->getTypeAllocSize(ST->getElementType())
               * getOperandValue(I.getOperand());
      }
  }
  return Total;
}
static void LoadIntFromMemory(uint64_t *Dst, uint8_t *Src, unsigned LoadBytes)
{
  if (LoadBytes > sizeof(*Dst)) {
      printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      exit(1);
  }
  memcpy(Dst, Src, LoadBytes);
}
static uint64_t LoadValueFromMemory(PointerTy Ptr, Type *Ty)
{
  const DataLayout *TD = EE->getDataLayout();
  const unsigned LoadBytes = TD->getTypeStoreSize(Ty);
  uint64_t rv = 0;

  if (trace_full)
      printf("[%s:%d] bytes %d type %x\n", __FUNCTION__, __LINE__, LoadBytes, Ty->getTypeID());
  switch (Ty->getTypeID()) {
  case Type::IntegerTyID:
      LoadIntFromMemory(&rv, (uint8_t*)Ptr, LoadBytes);
      break;
  case Type::PointerTyID:
      if (!Ptr) {
          printf("[%s:%d] %p\n", __FUNCTION__, __LINE__, Ptr);
          exit(1);
      }
      else
          rv = (uint64_t) *((PointerTy*)Ptr);
      break;
  default:
      errs() << "Cannot load value of type " << *Ty << "!";
      exit(1);
  }
  if (trace_full)
      printf("[%s:%d] rv %llx\n", __FUNCTION__, __LINE__, (long long)rv);
  return rv;
}
static void processFunction(VTABLE_WORK *work, void (*proc)(Function ***thisp, Instruction &I))
{
    Function *F = work->thisp[0][work->f];
    slotmap.clear();
    slotarray_index = 1;
    memset(slotarray, 0, sizeof(slotarray));
    int generate = proc == generateVerilog;
    /* Do generic optimization of instruction list (remove debug calls, remove automatic variables */
    for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I)
        opt_runOnBasicBlock(*I);
    if (trace_translate) {
        printf("FULL_AFTER_OPT: %s\n", F->getName().str().c_str());
        F->dump();
        printf("TRANSLATE:\n");
    }
    /* connect up argument formal param names with actual values */
    for (Function::const_arg_iterator AI = F->arg_begin(), AE = F->arg_end(); AI != AE; ++AI) {
        int slotindex = getLocalSlot(AI);
        if (AI->hasByValAttr()) {
            printf("[%s] hasByVal param not supported\n", __FUNCTION__);
            exit(1);
        }
        slotarray[slotindex].name = strdup(AI->getName().str().c_str());
        if (trace_full)
            printf("%s: [%d] '%s'\n", __FUNCTION__, slotindex, slotarray[slotindex].name);
        if (!strcmp(slotarray[slotindex].name, "this"))
            slotarray[slotindex].svalue = (uint8_t *)work->thisp;
        else if (!strcmp(slotarray[slotindex].name, "v")) {
            slotarray[slotindex] = work->arg;
        }
        else
            printf("%s: unknown parameter!! [%d] '%s'\n", __FUNCTION__, slotindex, slotarray[slotindex].name);
    }
    /* If this is an 'update' method, generate 'if guard' around instruction stream */
    already_printed_header = 0;
    globalGuardName = NULL;
    if (generate && endswith(globalName, "updateEv")) {
        char temp[MAX_CHAR_BUFFER];
        strcpy(temp, globalName);
        //strcat(temp + strlen(globalName) - 8, "guardEv");
        temp[strlen(globalName) - 9] = 0;  // truncate "updateEv"
        globalGuardName = strdup(temp);
    }
    /* Generate Verilog for all instructions.  Record function calls for post processing */
    for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I) {
        if (trace_translate)
        if (generate && I->hasName())         // Print out the label if it exists...
            printf("LLLLL: %s\n", I->getName().str().c_str());
        for (BasicBlock::iterator ins = I->begin(), ins_end = I->end(); ins != ins_end;) {
            char instruction_label[MAX_CHAR_BUFFER];

            BasicBlock::iterator next_ins = llvm::next(BasicBlock::iterator(ins));
            operand_list_index = 0;
            memset(operand_list, 0, sizeof(operand_list));
            if (ins->hasName() || !ins->getType()->isVoidTy()) {
              int t = getLocalSlot(ins);
              operand_list[operand_list_index].type = OpTypeLocalRef;
              operand_list[operand_list_index++].value = t;
              if (ins->hasName())
                  slotarray[t].name = strdup(ins->getName().str().c_str());
              else {
                  char temp[MAX_CHAR_BUFFER];
                  sprintf(temp, "%%%d", t);
                  slotarray[t].name = strdup(temp);
              }
              sprintf(instruction_label, "%10s/%d: ", slotarray[t].name, t);
            }
            else {
              operand_list_index++;
              sprintf(instruction_label, "            : ");
            }
            if (trace_translate)
            printf("%s    XLAT:%14s", instruction_label, ins->getOpcodeName());
            for (unsigned i = 0, E = ins->getNumOperands(); i != E; ++i)
                prepareOperand(ins->getOperand(i));
            switch (ins->getOpcode()) {
            case Instruction::GetElementPtr:
                {
                uint64_t Total = executeGEPOperation(gep_type_begin(ins), gep_type_end(ins));
                if (!slotarray[operand_list[1].value].svalue) {
                    printf("[%s:%d] GEP pointer not valid\n", __FUNCTION__, __LINE__);
                    break;
                    exit(1);
                }
                uint8_t *ptr = slotarray[operand_list[1].value].svalue + Total;
                slotarray[operand_list[0].value].name = strdup(map_address(ptr, ""));
                slotarray[operand_list[0].value].svalue = ptr;
                slotarray[operand_list[0].value].offset = Total;
                }
                break;
            case Instruction::Load:
                {
                slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
                PointerTy Ptr = (PointerTy)slotarray[operand_list[1].value].svalue;
                if(!Ptr) {
                    printf("[%s:%d] arg not LocalRef;", __FUNCTION__, __LINE__);
                    if (!slotarray[operand_list[0].value].svalue)
                        operand_list[0].type = OpTypeInt;
                    break;
                }
                slotarray[operand_list[0].value].svalue = (uint8_t *)LoadValueFromMemory(Ptr, ins->getType());
                slotarray[operand_list[0].value].name = strdup(map_address(Ptr, ""));
                }
                break;
            default:
                vout[0] = 0;
                proc(work->thisp, *ins);
                if (strlen(vout)) {
                    if (!already_printed_header) {
                        fprintf(outputFile, "\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n; %s\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n", globalName);
                        if (globalGuardName)
                            fprintf(outputFile, "if (%s5guardEv && %s6enableEv) then begin\n", globalGuardName, globalGuardName);
                    }
                    already_printed_header = 1;
                   fprintf(outputFile, "        %s\n", vout);
                }
                break;
            case Instruction::Alloca: // ignore
                memset(&slotarray[operand_list[0].value], 0, sizeof(slotarray[0]));
                break;
            }
            if (trace_translate)
                printf("\n");
            ins = next_ins;
        }
    }
    if (globalGuardName && already_printed_header)
        fprintf(outputFile, "end;\n");
}

static void processConstructorAndRules(Module *Mod, Function ****modfirst,
       void (*proc)(Function ***thisp, Instruction &I))
{
  int generate = proc == generateVerilog;
  *modfirst = NULL;       // init the Module list before calling constructors
  // run Constructors
callfun(4000);
  EE->runStaticConstructorsDestructors(false);
callfun(4010);
  // Construct the address -> symbolic name map using dwarf debug info
  constructAddressMap();
  int ModuleRfirst= lookup_field("class.Module", "rfirst")/sizeof(uint64_t);
  int ModuleNext  = lookup_field("class.Module", "next")/sizeof(uint64_t);
  int RuleNext    = lookup_field("class.Rule", "next")/sizeof(uint64_t);
  Function ***modp = *modfirst;

  // Walk the rule lists for all modules, generating work items
  while (modp) {                   // loop through all modules
      printf("Module %p: rfirst %p next %p\n", modp, modp[ModuleRfirst], modp[ModuleNext]);
      Function ***rulep = (Function ***)modp[ModuleRfirst];        // Module.rfirst
      while (rulep) {                      // loop through all rules for module
          printf("Rule %p: next %p\n", rulep, rulep[RuleNext]);
          static std::string method[] = { "body", "guard", "update", ""};
          std::string *p = method;
          while (*p != "") {
              vtablework.push_back(VTABLE_WORK(lookup_method("class.Rule", *p),
                  rulep, SLOTARRAY_TYPE()));
              p++;
              if (!generate) // only preprocess 'body'
                  break;
          }
          rulep = (Function ***)rulep[RuleNext];           // Rule.next
      }
      modp = (Function ***)modp[ModuleNext]; // Module.next
  }

  // Walk list of work items, generating code
  while (vtablework.begin() != vtablework.end()) {
      Function *f = vtablework.begin()->thisp[0][vtablework.begin()->f];
      globalName = strdup(f->getName().str().c_str());
      VTABLE_WORK work = *vtablework.begin();
      processFunction(&work, proc);
      vtablework.pop_front();
  }
}

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
        {
        PointerType *PTy = dyn_cast<PointerType>(intype);
        rc |= remapStruct(PTy->getPointerElementType(), inherit);
        }
        return rc;
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
//length = arg->getNumElements();
        arg->setBody(ArrayRef<Type *>(data, length));
        bozo->hsetSubclassData(val);
        }
    }
    return rc;
}

static void adjustModuleSizes(Module *Mod)
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
      const char *fname = FI->getName().str().c_str();
      for (Function::iterator BI = FI->begin(), BE = FI->end(); BI != BE; ++BI) {
          for (BasicBlock::iterator II = BI->begin(), IE = BI->end(); II != IE; ++II) {
          switch (II->getOpcode()) {
          case Instruction::Call:
              Value *called = II->getOperand(II->getNumOperands()-1);
              const char *cp = called->getName().str().c_str();
              const ConstantInt *CI = dyn_cast<ConstantInt>(II->getOperand(0));
              if (!strcmp(cp, "_Znwm") && CI && CI->getType()->isIntegerTy(64)) {
                  BasicBlock::iterator PI = llvm::next(BasicBlock::iterator(II));
                  if (PointerType *PTy = dyn_cast<PointerType>(PI->getType())) {
                      const StructType *STy = cast<StructType>(PTy->getPointerElementType());
                      const char *ctype = STy->getName().str().c_str();
                      uint64_t isize = CI->getZExtValue();
                      printf("%s: %s CALL %s CI %p bbsize %ld isize 0x%llx name %s\n",
                           __FUNCTION__, fname, cp, CI, FI->size(), (long long)isize, ctype);
                      IRBuilder<> builder(II->getParent());
                      II->setOperand(0, builder.getInt64(isize * 2 + MAX_BASIC_BLOCK_FLAGS * sizeof(int) + GIANT_SIZE));
//II->getParent()->dump();
        //BasicBlock::iterator PI = llvm::next(BasicBlock::iterator(II));
//PI->dump();
                      StructType *tgv = Mod->getTypeByName(ctype);
printf("[%s:%d] %p %p\n", __FUNCTION__, __LINE__, STy, tgv);
structMap.clear();
                      remapStruct(tgv, 0);
tgv->dump();
                  }
              }
              break;
          }
          }
      }
  }
}

static Module *llvm_ParseIRFile(const std::string &Filename, SMDiagnostic &Err, LLVMContext &Context)
{
  OwningPtr<MemoryBuffer> File;
  if (MemoryBuffer::getFileOrSTDIN(Filename, File)) {
    printf("llvm_ParseIRFile: could not open inpuf file %s\n", Filename.c_str());
    return 0;
  }
  Module *M = new Module(Filename, Context);
  M->addModuleFlag(llvm::Module::Error, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);
  return ParseAssembly(File.take(), M, Err, Context);
}
static inline Module *LoadFile(const char *argv0, const std::string &FN, LLVMContext& Context)
{
  SMDiagnostic Err;
  printf("[%s:%d] loading '%s'\n", __FUNCTION__, __LINE__, FN.c_str());
  Module* Result = llvm_ParseIRFile(FN, Err, Context);
  if (!Result)
      Err.print(argv0, errs());
  return Result;
}

namespace {
  cl::list<std::string> InputFile(cl::Positional, cl::OneOrMore, cl::desc("<input bitcode>"));
  cl::opt<std::string> MArch("march", cl::desc("Architecture to generate assembly for (see --version)"));
  cl::list<std::string> MAttrs("mattr", cl::CommaSeparated, cl::desc("Target specific attributes (-mattr=help for details)"), cl::value_desc("a1,+a2,-a3,..."));
}

int main(int argc, char **argv, char * const *envp)
{
  std::string ErrorMsg;

printf("[%s:%d] start\n", __FUNCTION__, __LINE__);
  DebugFlag = dump_interpret != 0;
  outputFile = fopen("output.tmp", "w");
  if (output_stdout)
      outputFile = stdout;

  LLVMContext &Context = getGlobalContext();

  cl::ParseCommandLineOptions(argc, argv, "llvm interpreter & dynamic compiler\n");

  // Load/link the input bitcode
  Mod = LoadFile(argv[0], InputFile[0], Context);
  if (!Mod) {
      errs() << argv[0] << ": error loading file '" << InputFile[0] << "'\n";
      return 1;
  }
  Linker L(Mod);
  for (unsigned i = 1; i < InputFile.size(); ++i) {
      Module *M = LoadFile(argv[0], InputFile[i], Context);
      if (!M || L.linkInModule(M, &ErrorMsg)) {
          errs() << argv[0] << ": link error in '" << InputFile[i] << "': " << ErrorMsg << "\n";
          return 1;
      }
  }

  // If not jitting lazily, load the whole bitcode file eagerly too.
  if (Mod->MaterializeAllPermanently(&ErrorMsg)) {
      printf("%s: bitcode didn't read correctly.\n", argv[0]);
      printf("Reason: %s\n", ErrorMsg.c_str());
      return 1;
  }

  //ModulePass *DebugIRPass = createDebugIRPass();
  //DebugIRPass->runOnModule(*Mod);

  EngineBuilder builder(Mod);
  builder.setMArch(MArch);
  builder.setMCPU("");
  builder.setMAttrs(MAttrs);
  builder.setRelocationModel(Reloc::Default);
  builder.setCodeModel(CodeModel::JITDefault);
  builder.setErrorStr(&ErrorMsg);
  builder.setEngineKind(EngineKind::Interpreter);
  builder.setJITMemoryManager(0);
  builder.setOptLevel(CodeGenOpt::None);

  TargetOptions Options;
  Options.UseSoftFloat = false;
  Options.JITEmitDebugInfo = true;
  Options.JITEmitDebugInfoToDisk = false;

  builder.setTargetOptions(Options);

  // preprocessing before running anything
  CU_Nodes = Mod->getNamedMetadata("llvm.dbg.cu");
  if (CU_Nodes)
      process_metadata(CU_Nodes);
  adjustModuleSizes(Mod);

  // Create the execution environment for running the constructors
  EE = builder.create();
  if (!EE) {
      printf("%s: unknown error creating EE!\n", argv[0]);
      exit(1);
  }

  EE->DisableLazyCompilation(true);

  std::vector<std::string> InputArgv;
  InputArgv.push_back("param1");
  InputArgv.push_back("param2");

  Function **** modfirst = (Function ****)EE->getPointerToGlobal(Mod->getNamedValue("_ZN6Module5firstE"));
  Function *EntryFn = Mod->getFunction("main");
  if (!EntryFn || !modfirst) {
      printf("'main' function not found in module.\n");
      return 1;
  }

  // Preprocess the body rules, creating shadow variables and moving items to guard() and update()
  processConstructorAndRules(Mod, modfirst, calculateGuardUpdate);

  // Process the static constructors, generating code for all rules
  processConstructorAndRules(Mod, modfirst, generateVerilog);

printf("[%s:%d] now run main program\n", __FUNCTION__, __LINE__);
  // Run main
  int Result = EE->runFunctionAsMain(EntryFn, InputArgv, envp);

  //dump_class_data();

printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
  return Result;
}
