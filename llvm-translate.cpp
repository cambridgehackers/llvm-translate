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
#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/MutexGuard.h"
#include "llvm/DebugInfo.h"
#include "llvm/Linker.h"
#include "llvm/Assembly/Parser.h"

using namespace llvm;

static int dump_interpret;// = 1;
static int trace_meta;// = 1;
static int trace_full;// = 1;
static int output_stdout;// = 1;
#define SEPARATOR ":"

#define MAX_SLOTARRAY 1000
#define MAX_OPERAND_LIST 200
#define MAX_CHAR_BUFFER 1000
#define MAX_CLASS_DEFS  200
#define MAX_VTAB_EXTRA 100

class SLOTARRAY_TYPE {
public:
    const char *name;
    int ignore_debug_info;
    uint8_t *svalue;
    uint64_t offset;
    SLOTARRAY_TYPE() {
        name = NULL;
        ignore_debug_info = 0;
        svalue = NULL;
        offset = 0;
    }
};
class MAPTYPE_WORK {
public:
    int derived;
    DICompositeType CTy;
    char *addr;
    std::string aname;
    MAPTYPE_WORK(int a, DICompositeType b, char *c, std::string d) {
       derived = a;
       CTy = b;
       addr = c;
       aname = d;
    }
};

class VTABLE_WORK {
public:
    Function ***thisp;
    int method_index;
    SLOTARRAY_TYPE arg;
    VTABLE_WORK(Function ***a, int b, SLOTARRAY_TYPE c) {
       thisp = a;
       method_index = b;
       arg = c;
    }
};

typedef struct {
    const char   *name;
    const MDNode *node;
} CLASS_META_MEMBER;

typedef struct {
    const char   *name;
    const MDNode *node;
    const MDNode *inherit;
    int           member_count;
    CLASS_META_MEMBER *member;
} CLASS_META;

typedef struct {
    int value;
    const char *name;
} INTMAP_TYPE;

enum {OpTypeNone, OpTypeInt, OpTypeLocalRef, OpTypeExternalFunction, OpTypeString};

static INTMAP_TYPE predText[] = {
    {FCmpInst::FCMP_FALSE, "false"}, {FCmpInst::FCMP_OEQ, "oeq"},
    {FCmpInst::FCMP_OGT, "ogt"}, {FCmpInst::FCMP_OGE, "oge"},
    {FCmpInst::FCMP_OLT, "olt"}, {FCmpInst::FCMP_OLE, "ole"},
    {FCmpInst::FCMP_ONE, "one"}, {FCmpInst::FCMP_ORD, "ord"},
    {FCmpInst::FCMP_UNO, "uno"}, {FCmpInst::FCMP_UEQ, "ueq"},
    {FCmpInst::FCMP_UGT, "ugt"}, {FCmpInst::FCMP_UGE, "uge"},
    {FCmpInst::FCMP_ULT, "ult"}, {FCmpInst::FCMP_ULE, "ule"},
    {FCmpInst::FCMP_UNE, "une"}, {FCmpInst::FCMP_TRUE, "true"},
    {ICmpInst::ICMP_EQ, "eq"}, {ICmpInst::ICMP_NE, "ne"},
    {ICmpInst::ICMP_SGT, "sgt"}, {ICmpInst::ICMP_SGE, "sge"},
    {ICmpInst::ICMP_SLT, "slt"}, {ICmpInst::ICMP_SLE, "sle"},
    {ICmpInst::ICMP_UGT, "ugt"}, {ICmpInst::ICMP_UGE, "uge"},
    {ICmpInst::ICMP_ULT, "ult"}, {ICmpInst::ICMP_ULE, "ule"}, {}};
static INTMAP_TYPE opcodeMap[] = {
    {Instruction::Add, "+"}, {Instruction::FAdd, "+"},
    {Instruction::Sub, "-"}, {Instruction::FSub, "-"},
    {Instruction::Mul, "*"}, {Instruction::FMul, "*"},
    {Instruction::UDiv, "/"}, {Instruction::SDiv, "/"}, {Instruction::FDiv, "/"},
    {Instruction::URem, "%"}, {Instruction::SRem, "%"}, {Instruction::FRem, "%"},
    {Instruction::And, "&"}, {Instruction::Or, "|"}, {Instruction::Xor, "^"}, {}};
static INTMAP_TYPE opcodeStrMap[] = {
    {Instruction::Add, "Add"}, {Instruction::FAdd, "Add"},
    {Instruction::Sub, "Sub"}, {Instruction::FSub, "Sub"},
    {Instruction::Mul, "Mul"}, {Instruction::FMul, "Mul"},
    {Instruction::UDiv, "Div"}, {Instruction::SDiv, "Div"}, {Instruction::FDiv, "Div"},
    {Instruction::URem, "Rem"}, {Instruction::SRem, "Rem"}, {Instruction::FRem, "Rem"},
    {Instruction::And, "And"}, {Instruction::Or, "Or"}, {Instruction::Xor, "Xor"}, {}};

static SLOTARRAY_TYPE slotarray[MAX_SLOTARRAY];
static int slotarray_index = 1;
static ExecutionEngine *EE = 0;
static std::map<const Value *, int> slotmap;
static std::map<const MDNode *, int> metamap;
static int metanumber;
static CLASS_META class_data[MAX_CLASS_DEFS];
static int class_data_index;

static const char *globalName;
static FILE *outputFile;
static int already_printed_header;
static const char *globalGuardName;
static std::list<const MDNode *> global_members;
static CLASS_META *global_classp;

static struct {
   int type;
   uint64_t value;
} operand_list[MAX_OPERAND_LIST];
static int operand_list_index;
static std::map<void *, std::string> mapitem;
static std::list<MAPTYPE_WORK> mapwork;
static std::list<MAPTYPE_WORK> mapwork_non_class;
static std::list<VTABLE_WORK> vtablework;
static int slevel;
static int dtlevel;

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
static void clearLocalSlot(void)
{
  slotmap.clear();
  slotarray_index = 1;
  memset(slotarray, 0, sizeof(slotarray));
}

void dump_class_data()
{
  CLASS_META *classp = class_data;
  for (int i = 0; i < class_data_index; i++) {
    CLASS_META_MEMBER *classm = classp->member;
    printf("class %s node %p inherit %p; ", classp->name, classp->node, classp->inherit);
    for (int j = 0; j < classp->member_count; j++) {
        DIType Ty(classm->node);
        uint64_t off = Ty.getOffsetInBits()/8; //(long)litem.getSizeInBits()/8);
        printf(" %s/%lld", classm->name, (long long)off);
        classm++;
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
CLASS_META_MEMBER *lookup_class_member(const char *cp, uint64_t Total)
{
  CLASS_META *classp = lookup_class(cp);
  if (!classp)
      return NULL;
  CLASS_META_MEMBER *classm = classp->member;
  for (int ind = 0; ind < classp->member_count; ind++) {
      DIType Ty(classm->node);
      uint64_t off = Ty.getOffsetInBits()/8; //(long)litem.getSizeInBits()/8);
      if (off == Total)
          return classm;
      classm++;
  }
  return NULL;
}
static int lookup_method(const char *classname, const char *methodname)
{
  CLASS_META *classp = lookup_class(classname);
  if (!classp)
      return -1;
  CLASS_META_MEMBER *classm = classp->member;
  for (int ind = 0; ind < classp->member_count; ind++) {
      DISubprogram Ty(classm->node);
      if (Ty.getTag() == dwarf::DW_TAG_subprogram
       && !strcmp(Ty.getName().str().c_str(), methodname))
          return Ty.getVirtualIndex();
      classm++;
  }
  return -1;
}
static int lookup_field(const char *classname, const char *methodname)
{
  CLASS_META *classp = lookup_class(classname);
  if (!classp)
      return -1;
  CLASS_META_MEMBER *classm = classp->member;
  for (int ind = 0; ind < classp->member_count; ind++) {
      DIType Ty(classm->node);
      if (Ty.getTag() == dwarf::DW_TAG_member
       && !strcmp(Ty.getName().str().c_str(), methodname))
          return Ty.getOffsetInBits()/8;
      classm++;
  }
  return -1;
}

static const char *map_address(void *arg, std::string name)
{
    static char temp[MAX_CHAR_BUFFER];
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
    sprintf(temp, "%p", arg);
    return temp;
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

void writeOperand(const Value *Operand)
{
  const Constant *CV;

  if (!Operand)
    return;
  int slotindex = getLocalSlot(Operand);
  operand_list[operand_list_index].value = slotindex;
  if (Operand->hasName()) {
    if (isa<Constant>(Operand)) {
        operand_list[operand_list_index].type = OpTypeExternalFunction;
        slotarray[slotindex].name = Operand->getName().str().c_str();
    }
    else
        goto locallab;
    goto retlab;
  }
  CV = dyn_cast<Constant>(Operand);
  if (CV && !isa<GlobalValue>(CV)) {
    if (const ConstantInt *CI = dyn_cast<ConstantInt>(CV)) {
      operand_list[operand_list_index].type = OpTypeInt;
      slotarray[slotindex].offset = CI->getZExtValue();
      goto retlab;
    }
    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    exit(1);
  }
  if (dyn_cast<MDNode>(Operand) || dyn_cast<MDString>(Operand)
   || Operand->getValueID() == Value::PseudoSourceValueVal ||
      Operand->getValueID() == Value::FixedStackPseudoSourceValueVal) {
      printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      exit(1);
  }
locallab:
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

static void print_header()
{
    if (!already_printed_header) {
        fprintf(outputFile, "\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n; %s\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n", globalName);
        if (globalGuardName)
            fprintf(outputFile, "if (%sguardEv && %senableEv) then begin\n", globalGuardName, globalGuardName);
    }
    already_printed_header = 1;
}

uint64_t getOperandValue(const Value *Operand)
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

static const char *getPredicateText(unsigned predicate)
{
  return intmap_lookup(predText, predicate);
}
static const char *opstr(unsigned opcode)
{
  return intmap_lookup(opcodeMap, opcode);
}
static const char *opstr_print(unsigned opcode)
{
  return intmap_lookup(opcodeStrMap, opcode);
}

void translateVerilog(int return_type, const Instruction &I)
{
  int opcode = I.getOpcode();
  char instruction_label[MAX_CHAR_BUFFER];
  char vout[MAX_CHAR_BUFFER];
  int dump_operands = trace_full;

  vout[0] = 0;
  operand_list_index = 0;
  memset(operand_list, 0, sizeof(operand_list));
  if (I.hasName() || !I.getType()->isVoidTy()) {
    int t = getLocalSlot(&I);
    operand_list[operand_list_index].type = OpTypeLocalRef;
    operand_list[operand_list_index++].value = t;
    if (I.hasName())
        slotarray[t].name = strdup(I.getName().str().c_str());
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
  for (unsigned i = 0, E = I.getNumOperands(); i != E; ++i)
      writeOperand(I.getOperand(i));
  printf("%s    ", instruction_label);
  switch (opcode) {
  // Terminators
  case Instruction::Ret:
      {
      printf("XLAT:           Ret");
      int parent_block = getLocalSlot(I.getParent());
      printf("parent %d ret=%d/%d;", parent_block, return_type, operand_list_index);
      if (return_type == Type::IntegerTyID && operand_list_index > 1) {
          operand_list[0].type = OpTypeString;
          operand_list[0].value = (uint64_t)getparam(1);
          sprintf(vout, "%s = %s;", globalName, getparam(1));
      }
      }
      break;
  case Instruction::Br:
      {
      char temp[MAX_CHAR_BUFFER];
      printf("XLAT:            Br");
      if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
        const BranchInst &BI(cast<BranchInst>(I));
        writeOperand(BI.getCondition());
        int cond_item = getLocalSlot(BI.getCondition());
        sprintf(temp, "%s" SEPARATOR "%s_cond", globalName, I.getParent()->getName().str().c_str());
        if (slotarray[cond_item].name) {
            sprintf(vout, "%s = %s\n", temp, slotarray[cond_item].name);
            slotarray[cond_item].name = strdup(temp);
        }
        writeOperand(BI.getSuccessor(0));
        writeOperand(BI.getSuccessor(1));
      } else if (isa<IndirectBrInst>(I)) {
        for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i) {
          writeOperand(I.getOperand(i));
        }
      }
      dump_operands = 1;
      }
      break;
  //case Instruction::Switch:
      //const SwitchInst& SI(cast<SwitchInst>(I));
      //writeOperand(SI.getCondition());
      //writeOperand(SI.getDefaultDest());
      //for (SwitchInst::ConstCaseIt i = SI.case_begin(), e = SI.case_end(); i != e; ++i) {
        //writeOperand(i.getCaseValue());
        //writeOperand(i.getCaseSuccessor());
      //}
  //case Instruction::IndirectBr:
  //case Instruction::Invoke:
  //case Instruction::Resume:
  case Instruction::Unreachable:
      printf("XLAT:   Unreachable");
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
      temp[0] = 0;
      printf("XLAT:%14s", opstr_print(opcode));
      sprintf(temp, "((%s) %s (%s))", op1, opstr(opcode), op2);
      if (operand_list[0].type != OpTypeLocalRef) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      slotarray[operand_list[0].value].name = strdup(temp);
      }
      break;

  // Memory instructions...
  //case Instruction::Alloca: // ignore
  case Instruction::Load:
      {
      printf("XLAT:          Load");
      slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
      PointerTy Ptr = (PointerTy)slotarray[operand_list[1].value].svalue;
      if(!Ptr) {
          printf("[%s:%d] arg not LocalRef;", __FUNCTION__, __LINE__);
          if (!slotarray[operand_list[0].value].svalue)
              operand_list[0].type = OpTypeInt;
          dump_operands = 1;
          break;
      }
      Type *element = NULL;
      int eltype = -1;
      Type *topty = I.getOperand(0)->getType();
      int ptrlevel = 0;
      while (topty->getTypeID() == Type::PointerTyID) {
          PointerType *PTy = cast<PointerType>(topty);
          element = PTy->getElementType();
          eltype = element->getTypeID();
          ptrlevel++;
          if (eltype != Type::PointerTyID)
              break;
          topty = element;
      }
      if (eltype != Type::FunctionTyID) {
          Value *value = (Value *)LoadValueFromMemory(Ptr, I.getType());
          slotarray[operand_list[0].value].svalue = (uint8_t *)value;
          slotarray[operand_list[0].value].name = strdup(map_address(Ptr, ""));
      }
      }
      break;
  case Instruction::Store:
      printf("XLAT:         Store");
      if (operand_list[1].type == OpTypeLocalRef && !slotarray[operand_list[1].value].svalue)
          operand_list[1].type = OpTypeInt;
      if (operand_list[1].type != OpTypeLocalRef || operand_list[2].type != OpTypeLocalRef
        || !slotarray[operand_list[2].value].ignore_debug_info)
          sprintf(vout, "%s = %s;", getparam(2), getparam(1));
      else
          slotarray[operand_list[2].value] = slotarray[operand_list[1].value];
      break;
  //case Instruction::AtomicCmpXchg:
  //case Instruction::AtomicRMW:
  //case Instruction::Fence:
  case Instruction::GetElementPtr:
      {
      Type *element = NULL;
      int eltype = -1;

      printf("XLAT: GetElementPtr");
      uint64_t Total = executeGEPOperation(gep_type_begin(I), gep_type_end(I));
      if (!slotarray[operand_list[1].value].svalue) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      Type *topty = I.getOperand(0)->getType();
      while (topty->getTypeID() == Type::PointerTyID) {
          PointerType *PTy = cast<PointerType>(topty);
          element = PTy->getElementType();
          eltype = element->getTypeID();
          if (eltype != Type::PointerTyID)
              break;
          topty = element;
      }
      uint8_t *ptr = slotarray[operand_list[1].value].svalue + Total;
      if (eltype == Type::FunctionTyID)
          ptr = slotarray[operand_list[1].value].svalue;
      else
          slotarray[operand_list[0].value].name = strdup(map_address(ptr, ""));
      slotarray[operand_list[0].value].svalue = ptr;
      slotarray[operand_list[0].value].offset = Total;
      }
      break;

  // Convert instructions...
  case Instruction::Trunc:
      printf("XLAT:         Trunc");
      if(operand_list[0].type != OpTypeLocalRef || operand_list[1].type != OpTypeLocalRef) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
      break;
  case Instruction::ZExt:
      printf("XLAT:          Zext");
      if(operand_list[0].type != OpTypeLocalRef || operand_list[1].type != OpTypeLocalRef) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
      break;
  //case Instruction::SExt:
  //case Instruction::FPTrunc: //case Instruction::FPExt:
  //case Instruction::FPToUI: //case Instruction::FPToSI:
  //case Instruction::UIToFP: //case Instruction::SIToFP:
  //case Instruction::IntToPtr: //case Instruction::PtrToInt:
  case Instruction::BitCast:
      printf("XLAT:       BitCast");
      if(operand_list[0].type != OpTypeLocalRef || operand_list[1].type != OpTypeLocalRef) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
      break;
  //case Instruction::AddrSpaceCast:

  // Other instructions...
  case Instruction::ICmp:
      {
      const char *op1 = getparam(1), *op2 = getparam(2), *opstr = NULL;
      char temp[MAX_CHAR_BUFFER];
      temp[0] = 0;
      printf("XLAT:          ICmp");
      if (const CmpInst *CI = dyn_cast<CmpInst>(&I))
          opstr = getPredicateText(CI->getPredicate());
      sprintf(temp, "((%s) %s (%s))", op1, opstr, op2);
      if (operand_list[0].type == OpTypeLocalRef)
          slotarray[operand_list[0].value].name = strdup(temp);
      else {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      }
      break;
  //case Instruction::FCmp:
  case Instruction::PHI:
      {
      printf("XLAT:           PHI");
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
          writeOperand(PN->getIncomingValue(op));
          writeOperand(PN->getIncomingBlock(op));
          TerminatorInst *TI = PN->getIncomingBlock(op)->getTerminator();
          printf("[%s:%d] terminator\n", __FUNCTION__, __LINE__);
          TI->dump();
          const BranchInst *BI = dyn_cast<BranchInst>(TI);
          const char *trailch = "";
          if (isa<BranchInst>(TI) && cast<BranchInst>(TI)->isConditional()) {
            writeOperand(BI->getCondition());
            int cond_item = getLocalSlot(BI->getCondition());
            sprintf(temp, "%s ?", slotarray[cond_item].name);
            trailch = ":";
            //writeOperand(BI->getSuccessor(0));
            //writeOperand(BI->getSuccessor(1));
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
      printf("XLAT:          Call");
      if (!slotarray[operand_list[operand_list_index-1].value].svalue) {
          printf("[%s:%d] not an instantiable call!!!!\n", __FUNCTION__, __LINE__);
          break;
      }
      int tcall = operand_list[operand_list_index-1].value; // Callee is _last_ operand
      Function ***thisp = (Function ***)slotarray[tcall].svalue;
      int method_index = slotarray[tcall].offset/8;
      Function *f = thisp[0][method_index];
      vtablework.push_back(VTABLE_WORK(thisp, method_index,
          (operand_list_index > 3) ? slotarray[operand_list[2].value] : SLOTARRAY_TYPE()));
      const char *cp = f->getName().str().c_str();
      slotarray[operand_list[0].value].name = strdup(cp);
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
      printf("XLAT: Other opcode %d.=%s", opcode, I.getOpcodeName());
      exit(1);
      break;
  }
  if (dump_operands)
  for (int i = 0; i < operand_list_index; i++) {
      int t = operand_list[i].value;
      if (operand_list[i].type == OpTypeLocalRef)
          printf(" op[%d]L=%d:%p:%lld:[%p=%s];", i, t, slotarray[t].svalue, (long long)slotarray[t].offset, slotarray[t].name, slotarray[t].name);
      else if (operand_list[i].type != OpTypeNone)
          printf(" op[%d]=%s;", i, getparam(i));
  }
  printf("\n");
  if (strlen(vout)) {
     print_header();
     fprintf(outputFile, "        %s\n", vout);
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
                  if (!strcmp(cp, "llvm.dbg.declare") || !strcmp(cp, "printf")) {
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

static void processFunction(Function *F, void *thisp, SLOTARRAY_TYPE &arg)
{
    //FunctionType *FT = F->getFunctionType();
    char temp[MAX_CHAR_BUFFER];

    /* Do generic optimization of instruction list (remove debug calls, remove automatic variables */
    for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I)
        opt_runOnBasicBlock(*I);
    printf("FULL_AFTER_OPT:\n");
    F->dump();
    printf("TRANSLATE:\n");

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
            slotarray[slotindex].svalue = (uint8_t *)thisp;
        else if (!strcmp(slotarray[slotindex].name, "v")) {
            slotarray[slotindex] = arg;
        }
        else
            printf("%s: unknown parameter!! [%d] '%s'\n", __FUNCTION__, slotindex, slotarray[slotindex].name);
    }

    /* If this is an 'update' method, generate 'if guard' around instruction stream */
    already_printed_header = 0;
    strcpy(temp, globalName);
    globalGuardName = NULL;
    if (endswith(globalName, "updateEv")) {
        //strcat(temp + strlen(globalName) - 8, "guardEv");
        temp[strlen(globalName) - 8] = 0;  // truncate "updateEv"
        globalGuardName = strdup(temp);
    }
    /* Generate Verilog for all instructions.  Record function calls for post processing */
    for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I) {
        if (I->hasName())         // Print out the label if it exists...
            printf("LLLLL: %s\n", I->getName().str().c_str());
        for (BasicBlock::const_iterator ins = I->begin(), ins_end = I->end(); ins != ins_end; ++ins)
            translateVerilog(F->getReturnType()->getTypeID(), *ins);
    }
    clearLocalSlot();
    if (globalGuardName && already_printed_header)
        fprintf(outputFile, "end;\n");
}

static void loop_through_all_rules(Function ***modfirst)
{
    int ModuleRfirst = lookup_field("class.Module", "rfirst")/sizeof(uint64_t);
    int ModuleNext   = lookup_field("class.Module", "next")/sizeof(uint64_t);
    int RuleNext     = lookup_field("class.Rule", "next")/sizeof(uint64_t);
    while (modfirst) {                   // loop through all modules
        printf("Module %p: rfirst %p next %p\n", modfirst, modfirst[ModuleRfirst], modfirst[ModuleNext]);
        Function **t = modfirst[ModuleRfirst];        // Module.rfirst
        while (t) {                      // loop through all rules for module
            printf("Rule %p: next %p\n", t, t[RuleNext]);
            static const char *method[] = { "guard", "body", "update", NULL};
            const char **p = method;
            while (*p) {
                vtablework.push_back(VTABLE_WORK((Function ***)t, 
                    lookup_method("class.Rule", *p), SLOTARRAY_TYPE()));
                p++;
            }
            t = (Function **)t[RuleNext];           // Rule.next
        }
        modfirst = (Function ***)modfirst[ModuleNext]; // Module.next
    }
}

static std::string getScope(const Value *val)
{
    const MDNode *Node;
    if (!val || !(Node = dyn_cast<MDNode>(val)))
        return "";
    DIType nextitem(Node);
    std::string name = getScope(nextitem.getContext()) + nextitem.getName().str();
    //int ind = name.find("<");
    //if (ind >= 0)
        //name = name.substr(0, ind);
    return name + "::";
}
static void dumpType(DIType litem);
static void dumpTref(const Value *val)
{
    const MDNode *Node;
    if (!val || !(Node = dyn_cast<MDNode>(val)))
        return;
    DIType nextitem(Node);
    int tag = nextitem.getTag();
    std::string name = nextitem.getName().str();
    std::map<const MDNode *, int>::iterator FI = metamap.find(Node);
    if (FI != metamap.end()) {
        if (trace_meta)
        printf(" magic %s [%p] = ref %d\n", nextitem.getName().str().c_str(), val, FI->second);
    }
    else {
        if (trace_meta)
        printf(" magic %s [%p] =**** %d\n", nextitem.getName().str().c_str(), val, metanumber);
        metamap[Node] = metanumber++;
        if (tag != dwarf::DW_TAG_class_type)
            dumpType(nextitem);
        else {
            std::list<const MDNode *> saved_members = global_members;
            global_members.clear();
            dumpType(nextitem);
            int mcount = global_members.size();
            if (trace_full)
            printf("class %s members %d:", name.c_str(), mcount);
            CLASS_META *saved_classp = global_classp;
            CLASS_META *classp = &class_data[class_data_index++];
            global_classp = classp;
            classp->node = Node;
            classp->member_count = 0;
            classp->member = (CLASS_META_MEMBER *)malloc(sizeof(CLASS_META_MEMBER) * mcount);
            for (std::list<const MDNode *>::iterator MI = global_members.begin(),
                ME = global_members.end(); MI != ME; MI++) {
                DISubprogram Ty(*MI);
                const Value *v = Ty;
                int etag = Ty.getTag();
                const MDNode *Node;
                if (!v || !(Node = dyn_cast<MDNode>(v))) {
                    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                    exit(1);
                }
                const char *cp = Ty.getLinkageName().str().c_str();
                if (etag != dwarf::DW_TAG_subprogram || !strlen(cp))
                    cp = Ty.getName().str().c_str();
                if (trace_full)
                printf(" %s", cp);
                int j = classp->member_count++;
                classp->member[j].node = Node;
                classp->member[j].name = strdup(cp);
            }
            if (trace_full)
                printf("\n");
            classp->name = strdup(("class." + getScope(nextitem.getContext()) + name).c_str());
            int ind = name.find("<");
            if (ind >= 0) { /* also insert the class w/o template parameters */
                classp = &class_data[class_data_index++];
                *classp = *(classp-1);
                name = name.substr(0, ind);
                classp->name = strdup(("class." + getScope(nextitem.getContext()) + name).c_str());
            }
            global_classp = saved_classp;
            global_members = saved_members;
        }
    }
}

static void dumpType(DIType litem)
{
    int tag = litem.getTag();
    if (!tag)     // Ignore elements with tag of 0
        return;
    if (tag == dwarf::DW_TAG_pointer_type) {
        dtlevel++;
        DICompositeType CTy(litem);
        dumpTref(CTy.getTypeDerivedFrom());
        dtlevel--;
        return;
    }
    if (tag == dwarf::DW_TAG_inheritance) {
        dtlevel++;
        DICompositeType CTy(litem);
        //DIArray Elements = CTy.getTypeArray();
        const Value *v = CTy.getTypeDerivedFrom();
        const MDNode *Node;
        if (v && (Node = dyn_cast<MDNode>(v)) && global_classp) {
            if(global_classp->inherit) {
                printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                exit(1);
            }
            global_classp->inherit = Node;
        }
        dtlevel--;
        dumpTref(v);
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
            dumpTref(CTy.getTypeDerivedFrom());
            dumpTref(CTy.getContainingType());
        }
        for (unsigned k = 0, N = Elements.getNumElements(); k < N; ++k) {
            DIType Ty(Elements.getElement(k));
            int tag = Ty.getTag();
            if (tag == dwarf::DW_TAG_member || tag == dwarf::DW_TAG_subprogram) {
                const MDNode *Node = Ty;
                global_members.push_back(Node);
            }
            dumpType(Ty);
        }
        dtlevel--;
    }
}

static void mapType(int derived, DICompositeType CTy, char *addr, std::string aname)
{
    int tag = CTy.getTag();
    long offset = (long)CTy.getOffsetInBits()/8;
    addr += offset;
    char *addr_target = *(char **)addr;
    std::string name = CTy.getName().str();
    if (!name.length())
        name = CTy.getName().str();
    std::string fname = name;
    const GlobalValue *g = EE->getGlobalValueAtAddress(((uint64_t *)addr_target)-2);
    if (tag == dwarf::DW_TAG_class_type && g) {
        int status;
        const char *ret = abi::__cxa_demangle(g->getName().str().c_str(), 0, 0, &status);
        if (!strncmp(ret, "vtable for ", 11)) {
            char temp[MAX_CHAR_BUFFER];
            sprintf(temp, "class.%s", ret+11);
            CLASS_META *classp = lookup_class(temp);
            if (!classp) {
                printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                exit(1);
            }
            name = ret+11;
            if (!derived)
                CTy = DICompositeType(classp->node);
        }
    }
    if (aname.length() > 0)
        fname = aname + ":" + name;
    const char *cp = fname.c_str();
    if (tag == dwarf::DW_TAG_pointer_type) {
        const Value *val = CTy.getTypeDerivedFrom();
        const MDNode *derivedNode = NULL;
        if (!addr_target)
            return; // don't follow NULL pointers
        if (val)
            derivedNode = dyn_cast<MDNode>(val);
        if (!derivedNode) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        DICompositeType derivType(derivedNode);
        if (mapitem.find(addr_target) == mapitem.end()) // process item, if not seen before
            mapwork.push_back(MAPTYPE_WORK(0, derivType, addr_target, fname));
        return;
    }
    map_address(addr, fname);
    if (tag != dwarf::DW_TAG_subprogram && tag != dwarf::DW_TAG_subroutine_type
     && tag != dwarf::DW_TAG_class_type && tag != dwarf::DW_TAG_inheritance
     && tag != dwarf::DW_TAG_base_type) {
        printf(" %d SSSStag %20s name %30s ", slevel, dwarf::TagString(tag), cp);
        if (CTy.isStaticMember()) {
            printf("STATIC\n");
            return;
        }
        printf("addr [%s]=val %s derived %d\n", map_address(addr, fname), map_address(addr_target, ""), derived);
    }
    slevel++;
    if (tag == dwarf::DW_TAG_inheritance || tag == dwarf::DW_TAG_member
     || CTy.isCompositeType()) {
        const Value *val = CTy.getTypeDerivedFrom();
        const MDNode *derivedNode = NULL;
        if (val)
            derivedNode = dyn_cast<MDNode>(val);
        DIArray Elements = CTy.getTypeArray();
        if (tag != dwarf::DW_TAG_subroutine_type && derivedNode)
            mapwork.push_back(MAPTYPE_WORK(1, DICompositeType(derivedNode), addr, fname));
        for (unsigned k = 0, N = Elements.getNumElements(); k < N; ++k)
            mapwork.push_back(MAPTYPE_WORK(0, DICompositeType(Elements.getElement(k)), addr, fname));
    }
    slevel--;
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
          dumpTref(sub.getContainingType());
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
      dumpType(DICompositeType(sub.getType()));
    }
    DIArray EnumTypes = CU.getEnumTypes();
    for (unsigned i = 0, e = EnumTypes.getNumElements(); i != e; ++i) {
      if (trace_meta)
      printf("[%s:%d]enumtypes\n", __FUNCTION__, __LINE__);
      dumpType(DIType(EnumTypes.getElement(i)));
    }
    DIArray RetainedTypes = CU.getRetainedTypes();
    for (unsigned i = 0, e = RetainedTypes.getNumElements(); i != e; ++i) {
      if (trace_meta)
      printf("[%s:%d]retainedtypes\n", __FUNCTION__, __LINE__);
      dumpType(DIType(RetainedTypes.getElement(i)));
    }
    DIArray Imports = CU.getImportedEntities();
    for (unsigned i = 0, e = Imports.getNumElements(); i != e; ++i) {
      DIImportedEntity Import = DIImportedEntity(Imports.getElement(i));
      DIDescriptor Entity = Import.getEntity();
      if (Entity.isType()) {
        if (trace_meta)
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        dumpType(DIType(Entity));
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
      void *addr = EE->getPointerToGlobal(gv);
      if (trace_meta)
      printf("%s: globalvar: %s GlobalVariable %p type %d address %p\n", __FUNCTION__, cp.c_str(), gv, val->getType()->getTypeID(), addr);
    }
  }
}

static void construct_address_map(NamedMDNode *CU_Nodes)
{
  if (CU_Nodes) {
      process_metadata(CU_Nodes);
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
          printf("%s: globalvar %s tag %s context %p\n", __FUNCTION__, cp.c_str(), dwarf::TagString(tag), contextp);
          if (!contextp)
              mapwork.push_back(MAPTYPE_WORK(1, CTy, (char *)addr, cp));
          else
              mapwork_non_class.push_back(MAPTYPE_WORK(1, CTy, (char *)addr, cp));
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

static Module *llvm_ParseIRFile(const std::string &Filename, SMDiagnostic &Err, LLVMContext &Context) {
  OwningPtr<MemoryBuffer> File;
  if (error_code ec = MemoryBuffer::getFileOrSTDIN(Filename, File)) {
    Err = SMDiagnostic(Filename, SourceMgr::DK_Error, "Could not open input file: " + ec.message());
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
  outputFile = fopen("output.tmp", "w");
  if (output_stdout)
      outputFile = stdout;
  DebugFlag = dump_interpret != 0;

  LLVMContext &Context = getGlobalContext();

  cl::ParseCommandLineOptions(argc, argv, "llvm interpreter & dynamic compiler\n");

  // Load/link the input bitcode
  Module *Mod = LoadFile(argv[0], InputFile[0], Context);
  if (!Mod) {
    errs() << argv[0] << ": error loading file '" << InputFile[0] << "'\n";
    return 1;
  }
  Linker L(Mod);
  for (unsigned i = 1; i < InputFile.size(); ++i) {
    std::string ErrorMessage;
    Module *M = LoadFile(argv[0], InputFile[i], Context);
    if (!M || L.linkInModule(M, &ErrorMessage)) {
      errs() << argv[0] << ": link error in '" << InputFile[i] << "': " << ErrorMessage << "\n";
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

  // Create the execution environment for running the constructors
  EE = builder.create();
  if (!EE) {
    printf("%s: unknown error creating EE!\n", argv[0]);
    exit(1);
  }

  EE->DisableLazyCompilation(true);

  std::vector<std::string> InputArgv;
  InputArgv.insert(InputArgv.begin(), InputFile[0]);

  Function *EntryFn = Mod->getFunction("main");
  if (!EntryFn) {
    printf("'main' function not found in module.\n");
    return 1;
  }

  // Run the static constructors
  EE->runStaticConstructorsDestructors(false);

  // Run main
  int Result = EE->runFunctionAsMain(EntryFn, InputArgv, envp);

  // Construct the address -> symbolic name map using dwarf debug info
  construct_address_map(Mod->getNamedMetadata("llvm.dbg.cu"));
  
  // Walk the rule lists for all modules, generating work items
  loop_through_all_rules((Function ***)*(PointerTy*)
      EE->getPointerToGlobal(Mod->getNamedValue("_ZN6Module5firstE")));

  // Walk list of work items, generating code
  while (vtablework.begin() != vtablework.end()) {
      Function ***thisp = vtablework.begin()->thisp;
      Function *f = thisp[0][vtablework.begin()->method_index];
      globalName = strdup(f->getName().str().c_str());
      processFunction(f, thisp, vtablework.begin()->arg);
      vtablework.pop_front();
  }

  //dump_class_data();

printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
  return Result;
}
