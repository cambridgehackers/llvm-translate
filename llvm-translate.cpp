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
#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Triple.h"
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
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/DebugInfo.h"
#include "llvm/Linker.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Assembly/Parser.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"

using namespace llvm;

static int dump_interpret;// = 1;
static int trace_meta;// = 1;
static int output_stdout = 1;
static const char *trace_break_name;// = "_ZN4Echo7respond5guardEv";
#define SEPARATOR ":"

namespace {
  cl::list<std::string> InputFile(cl::Positional, cl::OneOrMore, cl::desc("<input bitcode>")); 
  cl::opt<std::string> MArch("march", cl::desc("Architecture to generate assembly for (see --version)")); 
  cl::list<std::string> MAttrs("mattr", cl::CommaSeparated, cl::desc("Target specific attributes (-mattr=help for details)"), cl::value_desc("a1,+a2,-a3,...")); 
  cl::list<std::string> ExtraModules("extra-module", cl::desc("Extra modules to be loaded"), cl::value_desc("input bitcode"));
}

#define MAX_SLOTARRAY 1000
#define MAX_OPERAND_LIST 200
#define MAX_CHAR_BUFFER 1000

static ExecutionEngine *EE = 0;
static std::map<const Value *, int> slotmap;
static std::map<const MDNode *, int> metamap;
static std::map<std::string, const MDNode *> classmap;
static int metanumber;
static struct {
    const char *name;
    int ignore_debug_info;
    uint8_t *svalue;
    uint64_t offset;
} slotarray[MAX_SLOTARRAY];
static int slotarray_index = 1;
#define MAX_CLASS_ARRAY 20
#define MAX_CLASS_DEFS  200
static struct {
    //std::string name;
    const MDNode * inherit;
    std::list<const MDNode *> members;
} classinfo_array[MAX_CLASS_ARRAY];
static int classinfo_array_index;
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
static CLASS_META class_data[MAX_CLASS_DEFS];
static int class_data_index;

static const char *globalName;
static const char *globalClassName;
static FILE *outputFile;
static const Function *globalFunction;
static int already_printed_header;

enum {OpTypeNone, OpTypeInt, OpTypeLocalRef, OpTypeExternalFunction, OpTypeString};
static struct {
   int type;
   uint64_t value;
} operand_list[MAX_OPERAND_LIST];
static int operand_list_index;
#define MAX_VTAB_EXTRA 100
static struct {
    int thisp;
    int vtab;
    int arg;
} extra_vtab[MAX_VTAB_EXTRA];
static int extra_vtab_index;

static void memdump(unsigned char *p, int len, const char *title)
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

void makeLocalSlot(const Value *V)
{
  slotmap.insert(std::pair<const Value *, int>(V, slotarray_index++));
}
int getLocalSlot(const Value *V)
{
  std::map<const Value *, int>::iterator FI = slotmap.find(V);
  if (FI == slotmap.end()) {
     makeLocalSlot(V);
     return getLocalSlot(V);
  }
  return (int)FI->second;
}
void clearLocalSlot(void)
{
  slotmap.clear();
  slotarray_index = 1;
  memset(slotarray, 0, sizeof(slotarray));
}

static void dump_class_data()
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
static CLASS_META_MEMBER *lookup_class(const char *cp, uint64_t Total)
{
  CLASS_META *classp = class_data;
  CLASS_META_MEMBER *classm;
  for (int i = 0; i < class_data_index; i++) {
    if (!strcmp(cp, classp->name))
        goto check_offset;
    classp++;
  }
  return NULL;
check_offset:
  classm = classp->member;
  for (int ind = 0; ind < classp->member_count; ind++) {
      DIType Ty(classm->node);
      uint64_t off = Ty.getOffsetInBits()/8; //(long)litem.getSizeInBits()/8);
      if (off == Total)
          return classm;
      classm++;
  }
  return NULL;
}

static const char *getPredicateText(unsigned predicate)
{
  const char * pred = "unknown";
  switch (predicate) {
  case FCmpInst::FCMP_FALSE: pred = "false"; break;
  case FCmpInst::FCMP_OEQ:   pred = "oeq"; break;
  case FCmpInst::FCMP_OGT:   pred = "ogt"; break;
  case FCmpInst::FCMP_OGE:   pred = "oge"; break;
  case FCmpInst::FCMP_OLT:   pred = "olt"; break;
  case FCmpInst::FCMP_OLE:   pred = "ole"; break;
  case FCmpInst::FCMP_ONE:   pred = "one"; break;
  case FCmpInst::FCMP_ORD:   pred = "ord"; break;
  case FCmpInst::FCMP_UNO:   pred = "uno"; break;
  case FCmpInst::FCMP_UEQ:   pred = "ueq"; break;
  case FCmpInst::FCMP_UGT:   pred = "ugt"; break;
  case FCmpInst::FCMP_UGE:   pred = "uge"; break;
  case FCmpInst::FCMP_ULT:   pred = "ult"; break;
  case FCmpInst::FCMP_ULE:   pred = "ule"; break;
  case FCmpInst::FCMP_UNE:   pred = "une"; break;
  case FCmpInst::FCMP_TRUE:  pred = "true"; break;
  case ICmpInst::ICMP_EQ:    pred = "eq"; break;
  case ICmpInst::ICMP_NE:    pred = "ne"; break;
  case ICmpInst::ICMP_SGT:   pred = "sgt"; break;
  case ICmpInst::ICMP_SGE:   pred = "sge"; break;
  case ICmpInst::ICMP_SLT:   pred = "slt"; break;
  case ICmpInst::ICMP_SLE:   pred = "sle"; break;
  case ICmpInst::ICMP_UGT:   pred = "ugt"; break;
  case ICmpInst::ICMP_UGE:   pred = "uge"; break;
  case ICmpInst::ICMP_ULT:   pred = "ult"; break;
  case ICmpInst::ICMP_ULE:   pred = "ule"; break;
  }
  return pred;
}
static void WriteConstantInternal(const Constant *CV)
{
  if (const ConstantInt *CI = dyn_cast<ConstantInt>(CV)) {
    operand_list[operand_list_index].type = OpTypeInt;
    operand_list[operand_list_index++].value = CI->getZExtValue();
    return;
  }

printf("[%s:%d]\n", __FUNCTION__, __LINE__);
exit(1);
}

void writeOperand(const Value *Operand)
{
  int slotindex;
  const Constant *CV;
  if (!Operand) {
    return;
  }
  if (Operand->hasName()) {
    if (isa<Constant>(Operand)) {
        operand_list[operand_list_index].type = OpTypeExternalFunction;
        operand_list[operand_list_index++].value = (uint64_t) Operand->getName().str().c_str();
    }
    else
        goto locallab;
    return;
  }
  CV = dyn_cast<Constant>(Operand);
  if (CV && !isa<GlobalValue>(CV)) {
    WriteConstantInternal(CV);// *TypePrinter, Machine, Context);
    return;
  }

  if (const MDNode *N = dyn_cast<MDNode>(Operand)) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
exit(1);
    if (N->isFunctionLocal()) {
      // Print metadata inline, not via slot reference number.
      //WriteMDNodeBodyInternal(N, TypePrinter, Machine, Context);
      return;
    }
    goto locallab;
  }

  if (const MDString *MDS = dyn_cast<MDString>(Operand)) {
printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, MDS->getString().str().c_str());
exit(1);
    //PrintEscapedString(MDS->getString());
    return;
  }

  if (Operand->getValueID() == Value::PseudoSourceValueVal ||
      Operand->getValueID() == Value::FixedStackPseudoSourceValueVal) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
exit(1);
    return;
  }
locallab:
  slotindex = getLocalSlot(Operand);
  operand_list[operand_list_index].type = OpTypeLocalRef;
  operand_list[operand_list_index++].value = slotindex;
}

static const char *getparam(int arg)
{
   char temp[MAX_CHAR_BUFFER];
   temp[0] = 0;
   if (operand_list[arg].type == OpTypeLocalRef)
       return (slotarray[operand_list[arg].value].name);
   else if (operand_list[arg].type == OpTypeExternalFunction)
       return (const char *)operand_list[arg].value;
   else if (operand_list[arg].type == OpTypeInt)
       sprintf(temp, "%lld", (long long)operand_list[arg].value);
   else if (operand_list[arg].type == OpTypeString)
       return (const char *)operand_list[arg].value;
   return strdup(temp);
}

static void print_header()
{
    if (!already_printed_header)
        fprintf(outputFile, "\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n; %s\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n", globalName);
    already_printed_header = 1;
}
static const char *opstr(unsigned opcode)
{
    switch (opcode) {
    case Instruction::Add:
    case Instruction::FAdd: return "+";
    case Instruction::Sub:
    case Instruction::FSub: return "-";
    case Instruction::Mul:
    case Instruction::FMul: return "*";
    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::FDiv: return "/";
    case Instruction::URem:
    case Instruction::SRem:
    case Instruction::FRem: return "%";
    case Instruction::And:  return "&";
    case Instruction::Or:   return "|";
    case Instruction::Xor:  return "^";
    }
    return NULL;
}
static const char *opstr_print(unsigned opcode)
{
    switch (opcode) {
    case Instruction::Add:
    case Instruction::FAdd: return "Add";
    case Instruction::Sub:
    case Instruction::FSub: return "Sub";
    case Instruction::Mul:
    case Instruction::FMul: return "Mul";
    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::FDiv: return "Div";
    case Instruction::URem:
    case Instruction::SRem:
    case Instruction::FRem: return "Rem";
    case Instruction::And:  return "And";
    case Instruction::Or:   return "Or";
    case Instruction::Xor:  return "Xor";
    }
    return NULL;
}

uint64_t getOperandValue(const Value *Operand)
{
  static uint64_t rv = 0;
  const Constant *CV = dyn_cast<Constant>(Operand);
  const ConstantInt *CI;

  if (CV && !isa<GlobalValue>(CV) && (CI = dyn_cast<ConstantInt>(CV)))
      rv = CI->getZExtValue();
  else {
      printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      exit(1);
  }
  //printf("[%s:%d] const %lld\n", __FUNCTION__, __LINE__, (long long)rv);
  return rv;
}

static DataLayout *TD;
static uint64_t executeGEPOperation(gep_type_iterator I, gep_type_iterator E)
{
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
static int dummyval;
static uint64_t LoadValueFromMemory(PointerTy Ptr, Type *Ty)
{
  const unsigned LoadBytes = TD->getTypeStoreSize(Ty);
  uint64_t rv = 0;

printf("[%s:%d] bytes %d type %x\n", __FUNCTION__, __LINE__, LoadBytes, Ty->getTypeID());
  switch (Ty->getTypeID()) {
  case Type::IntegerTyID:
    LoadIntFromMemory(&rv, (uint8_t*)Ptr, LoadBytes);
    break;
  case Type::PointerTyID:
    if (!Ptr) {
        printf("[%s:%d] %p\n", __FUNCTION__, __LINE__, Ptr);
        rv = (uint64_t)&dummyval;
    }
    else
        rv = (uint64_t) *((PointerTy*)Ptr);
    break;
  default:
    errs() << "Cannot load value of type " << *Ty << "!";
    exit(1);
  }
printf("[%s:%d] rv %llx\n", __FUNCTION__, __LINE__, (long long)rv);
  return rv;
}
void translateVerilog(const Instruction &I)
{
  int opcode = I.getOpcode();
  char instruction_label[MAX_CHAR_BUFFER];
  char vout[MAX_CHAR_BUFFER];

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
      printf(" ret=%d/%d;", globalFunction->getReturnType()->getTypeID(), operand_list_index);
      if (globalFunction->getReturnType()->getTypeID() == Type::IntegerTyID && operand_list_index > 1) {
          operand_list[0].type = OpTypeString;
          operand_list[0].value = (uint64_t)getparam(1);
          if (strlen(globalName) > 7 && !strcmp(globalName + strlen(globalName) - 7, "guardEv"))
              sprintf(vout, "%s = %s && %s" SEPARATOR "enable;", globalName, getparam(1), globalName);
          else
              sprintf(vout, "%s = %s;", globalName, getparam(1));
      }
      }
      break;
  case Instruction::Br:
      {
      printf("XLAT:            Br");
      //if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
        //const BranchInst &BI(cast<BranchInst>(I));
        //writeOperand(BI.getCondition());
        //writeOperand(BI.getSuccessor(0));
        //writeOperand(BI.getSuccessor(1));
      //} else if (isa<IndirectBrInst>(I)) {
        //writeOperand(Operand);
        //for (unsigned i = 1, e = I.getNumOperands(); i != e; ++i) {
          //writeOperand(I.getOperand(i));
        //}
      //}
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
  case Instruction::Add:
  case Instruction::FAdd:
  case Instruction::Sub:
  case Instruction::FSub:
  case Instruction::Mul:
  case Instruction::FMul:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::FDiv:
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::FRem:

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
      PointerTy Ptr = (PointerTy)slotarray[operand_list[1].value].svalue;
      if(operand_list[0].type != OpTypeLocalRef || operand_list[1].type != OpTypeLocalRef || !Ptr) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
     exit(1);
  }
      slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
#if 1
      Type *element = NULL;
      int eltype = -1;
      Type *topty = I.getOperand(0)->getType();
      while (topty->getTypeID() == Type::PointerTyID) {
          PointerType *PTy = cast<PointerType>(topty);
          element = PTy->getElementType();
          eltype = element->getTypeID();
          if (eltype != Type::PointerTyID)
              break;
          topty = element;
      }
      if (eltype == Type::FunctionTyID) {
printf("[%s:%d] FFFFFFFUUUUUUUUUUUUUUCCCCCCCCCC\n", __FUNCTION__, __LINE__);
      }
else
#endif
{
      Value *value = (Value *)LoadValueFromMemory(Ptr, I.getType());
      slotarray[operand_list[0].value].svalue = (uint8_t *)value;
      const GlobalValue *g = EE->getGlobalValueAtAddress(Ptr);
      printf("g2=%p;", g);
printf("[%s:%d] value %p\n", __FUNCTION__, __LINE__, value);
      if (g)  // remember the name of where this value came from
          slotarray[operand_list[0].value].name = strdup(g->getName().str().c_str());
      }
}
      break;
  case Instruction::Store:
      printf("XLAT:         Store");
      if (operand_list[2].type != OpTypeLocalRef || !slotarray[operand_list[2].value].ignore_debug_info)
          sprintf(vout, "%s = %s;", getparam(2), getparam(1));
      if (operand_list[2].type == OpTypeLocalRef && operand_list[1].type == OpTypeLocalRef)
          slotarray[operand_list[2].value] = slotarray[operand_list[1].value];
      break;
  //case Instruction::AtomicCmpXchg:
  //case Instruction::AtomicRMW:
  //case Instruction::Fence:
  case Instruction::GetElementPtr:
      {
      Type *element = NULL;
      int eltype = -1;
      int ptrcnt = 0;
      CLASS_META_MEMBER *classm = NULL;
      const char *ret = NULL;
      char temp[MAX_CHAR_BUFFER], tempoff[MAX_CHAR_BUFFER];
      const char *mcp = NULL;
#if 0
      const GlobalValue *g;
#endif

      printf("XLAT: GetElementPtr");
      tempoff[0] = 0;
      uint64_t Total = executeGEPOperation(gep_type_begin(I), gep_type_end(I));
      //printf(" GEP Index %lld;", (long long)Total);
      if (!slotarray[operand_list[1].value].svalue) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      uint8_t *ptr = slotarray[operand_list[1].value].svalue + Total;
      const char *cp = slotarray[operand_list[1].value].name;
      printf(" type %d. GEPname %s;", I.getOperand(0)->getType()->getTypeID(), cp);
      Type *topty = I.getOperand(0)->getType();
      while (topty->getTypeID() == Type::PointerTyID) {
          ptrcnt++;
          PointerType *PTy = cast<PointerType>(topty);
          element = PTy->getElementType();
          eltype = element->getTypeID();
          if (eltype != Type::PointerTyID)
              break;
          topty = element;
      }
      if (eltype == Type::FunctionTyID) {
          ptr = slotarray[operand_list[1].value].svalue;
#if 0
          g = EE->getGlobalValueAtAddress(((uint64_t *)slotarray[operand_list[1].value].svalue)-2);
          if (!g) {
              printf("[%s:%d] %p\n", __FUNCTION__, __LINE__, (uint64_t *)slotarray[operand_list[1].value].svalue);
              exit(1);
              tempoff[0] = 0;
          }
          else
              sprintf(tempoff, "%s/%lld", g->getName().str().c_str(), (long long)Total);
          sprintf(tempoff, "FFUNNNNNNNNNNNN/%lld", (long long)Total);
          mcp = tempoff;
#endif
          printf(" FUNCOFFSET=%lld", (long long)Total);
          goto skipfunc;
      }
      else if (eltype != Type::StructTyID) {
          printf(" itype %d.;", eltype);
          element = NULL;
      }
      if(!element) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      classm = lookup_class(element->getStructName().str().c_str(), Total);
      if(!classm) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      mcp = classm->name;
      ret = cp;
      {
      const GlobalValue *g = EE->getGlobalValueAtAddress(ptr);
      if (g) {
          ret = g->getName().str().c_str();
          mcp = "";
      }
      }
      sprintf(temp, "%s" SEPARATOR "%s", ret, mcp);
      slotarray[operand_list[0].value].name = strdup(temp);
skipfunc:
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
  //case Instruction::FPTrunc:
  //case Instruction::FPExt:
  //case Instruction::FPToUI:
  //case Instruction::FPToSI:
  //case Instruction::UIToFP:
  //case Instruction::SIToFP:
  //case Instruction::IntToPtr:
  //case Instruction::PtrToInt:
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
      const PHINode *PN = dyn_cast<PHINode>(&I);
      if (!PN) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      I.getType()->dump();
      for (unsigned op = 0, Eop = PN->getNumIncomingValues(); op < Eop; ++op) {
          writeOperand(PN->getIncomingValue(op));
          writeOperand(PN->getIncomingBlock(op));
      }
      }
      break;
  //case Instruction::Select:
  case Instruction::Call:
      {
      const CallInst *CI = dyn_cast<CallInst>(&I);
      const Value *val = CI->getCalledValue();
      //writeOperand(val);
      printf("XLAT:          Call");
      if (operand_list[1].type != OpTypeLocalRef) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
     exit(1);
  }
      extra_vtab[extra_vtab_index].vtab = operand_list[operand_list_index-1].value; // Callee is _last_ operand
      extra_vtab[extra_vtab_index].thisp = operand_list[1].value;
      if (operand_list_index > 3)
          extra_vtab[extra_vtab_index].arg = operand_list[2].value;
      extra_vtab_index++;
printf("[%s:%d]FFFFFFFFFFFFFF operand_list_index %d\n", __FUNCTION__, __LINE__, operand_list_index);
      const Instruction *t = (const Instruction *)val;
      printf ("[%p=%s]", t, t->getName().str().c_str());
      //t->dump();
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
  for (int i = 0; i < operand_list_index; i++) {
      int t = operand_list[i].value;
      if (operand_list[i].type == OpTypeLocalRef)
          printf(" op[%d]L=%d:%p:[%p=%s];", i, t, slotarray[t].svalue, slotarray[t].name, slotarray[t].name);
      else if (operand_list[i].type != OpTypeNone)
          printf(" op[%d]=%s;", i, getparam(i));
  }
  printf("\n");
  if (strlen(vout)) {
     print_header();
     fprintf(outputFile, "        %s\n", vout);
  }
}

bool opt_runOnBasicBlock(BasicBlock &BB)
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
            //printf("[%s:%d] Load %s ret %p\n", __FUNCTION__, __LINE__, cp, retv);
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
                //const char *cp = I->getName().str().c_str();
                //printf("[%s:%d] Alloca %s\n", __FUNCTION__, __LINE__, cp);
                BasicBlock::iterator PN = PI;
                while (PN != E) {
                    BasicBlock::iterator PNN = llvm::next(BasicBlock::iterator(PN));
                    if (PN->getOpcode() == Instruction::Store && retv == PN->getOperand(1))
                        newt = PN->getOperand(0);
                    //printf("[%s:%d] ROP %d: ", __FUNCTION__, __LINE__, PN->getOpcode());
                    for (User::op_iterator OI = PN->op_begin(), OE = PN->op_end(); OI != OE; ++OI) {
                        Value *val = *OI;
                        //printf(" %p", val);
                        if (val == retv && newt) {
                            //printf(" REPLACEOP");
                            *OI = newt;
                        }
                    }
                    //printf("\n");
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
                      //printf("[%s:%d] ERASE\n", __FUNCTION__, __LINE__);
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

static void verilogFunction(Function *F)
{
  FunctionType *FT = F->getFunctionType();
  int updateFlag = strlen(globalName) > 8 && !strcmp(globalName + strlen(globalName) - 8, "updateEv");
  char temp[MAX_CHAR_BUFFER];

  globalFunction = F;
  already_printed_header = 0;
  strcpy(temp, globalName);
  if (updateFlag) {
      print_header();
      strcat(temp + strlen(globalName) - 8, "guardEv");
      fprintf(outputFile, "if (%s) then begin\n", temp);
  }
  for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I)
      for (BasicBlock::const_iterator ins = I->begin(), ins_end = I->end(); ins != ins_end; ++ins)
          translateVerilog(*ins);
  printf("[%s:%d] extra %d\n", __FUNCTION__, __LINE__, extra_vtab_index);
  for (int i = 0; i < extra_vtab_index; i++) {
      printf("[%s:%d] [%d.] vt %d this %d arg %d *******************************************\n", __FUNCTION__, __LINE__, i,
          extra_vtab[i].vtab, extra_vtab[i].thisp, extra_vtab[i].arg);
      printf("[%s:%d] [%d.] vt %p/%lld this %p/%lld arg %p/%lld\n", __FUNCTION__, __LINE__, i,
          slotarray[extra_vtab[i].vtab].svalue, (long long)slotarray[extra_vtab[i].vtab].offset,
          slotarray[extra_vtab[i].thisp].svalue, (long long)slotarray[extra_vtab[i].thisp].offset,
          slotarray[extra_vtab[i].arg].svalue, (long long)slotarray[extra_vtab[i].arg].offset);
      //dump_vtable(extra_vtab[i].tmp, (uint64_t ***)extra_vtab[i].vtab);
  }
  extra_vtab_index = 0;
  memset(extra_vtab, 0, sizeof(extra_vtab));
  clearLocalSlot();
  if (updateFlag) {
      print_header();
      fprintf(outputFile, "end;\n");
  }
}

static void processFunction(Function *F, uint64_t ***thisp)
{
    int num_args = 0;
    for (Function::const_arg_iterator AI = F->arg_begin(), AE = F->arg_end(); AI != AE; ++AI) {
        int slotindex = getLocalSlot(AI);
        num_args++;
        if (AI->hasByValAttr()) {
            printf("[%s] hasByVal param not supported\n", __FUNCTION__);
            exit(1);
        }
        slotarray[slotindex].name = strdup(AI->getName().str().c_str());
        printf("%s: [%d] '%s'\n", __FUNCTION__, slotindex, slotarray[slotindex].name);
        if (!strcmp(slotarray[slotindex].name, "this")) {
            slotarray[slotindex].name = globalClassName;
            slotarray[slotindex].svalue = (uint8_t *)thisp;
        }
    }
    for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I)
        opt_runOnBasicBlock(*I);
    printf("FULLopt:\n");
    F->dump();
    printf("TRANSLATE:\n");
    if (num_args > 1) {
        printf("[%s:%d] num_args > 1 not supported %d\n", __FUNCTION__, __LINE__, num_args);
        //exit(1);
        //return;
    }
    verilogFunction(F);
}

static void dump_vtable(uint64_t **vtab, uint64_t ***thisp)
{
int arr_size = 0;
    const GlobalValue *g;

    for (int i = 0; i < 16; i++) {
        g = EE->getGlobalValueAtAddress(vtab - 8 + i);
        if (g)
            printf("[%s:%d]v %d g %p name %s\n", __FUNCTION__, __LINE__, - 8 + i, g, g->getName().str().c_str());
    }
    g = EE->getGlobalValueAtAddress(vtab-2);
    globalClassName = NULL;
    printf("[%s:%d] vtabbase %p g %p:\n", __FUNCTION__, __LINE__, vtab-2, g);
    memdump(((unsigned char *)thisp), 64, "THIS");
    if (g) {
        globalClassName = strdup(g->getName().str().c_str());
        if (g->getType()->getTypeID() == Type::PointerTyID) {
           Type *ty = g->getType()->getElementType();
           if (ty->getTypeID() == Type::ArrayTyID) {
               ArrayType *aty = cast<ArrayType>(ty);
               arr_size = aty->getNumElements();
           }
        }
        g->getType()->dump();
        fprintf(stderr, "\n");
    }
    for (int i = 0; i < arr_size-2; i++) {
       Function *f = (Function *)(vtab[i]);
       globalName = strdup(f->getName().str().c_str());
       const char *cend = globalName + (strlen(globalName)-4);
       printf("[%s:%d] [%d] p %p: %s, this %p\n", __FUNCTION__, __LINE__, i, vtab[i], globalName, thisp);
       int rettype = f->getReturnType()->getTypeID();
       if ((rettype == Type::IntegerTyID || rettype == Type::VoidTyID)
        && strcmp(cend, "D0Ev") && strcmp(cend, "D1Ev")) {
           if (strlen(globalName) <= 18 || strcmp(globalName + (strlen(globalName)-18), "setModuleEP6Module")) {
               if (!f->isDeclaration())
                   processFunction(f, thisp);
               if (trace_break_name && !strcmp(globalName, trace_break_name)) {
                   printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                   exit(1);
               }
           }
       }
    }
}
void dump_vtab(uint64_t ***thisptr)
{
    const GlobalValue *g;
    uint64_t **vtab = (uint64_t **)thisptr[0];

    for (int i = 0; i < 16; i++) {
         g = EE->getGlobalValueAtAddress(thisptr -8 + i);
if (g)
printf("[%s:%d] %d g %p name %s\n", __FUNCTION__, __LINE__, -8 + i, g, g->getName().str().c_str());
    }
    dump_vtable(vtab, thisptr);
}

static void dump_type(Module *Mod, const char *p)
{
    StructType *tgv = Mod->getTypeByName(p);
    printf("%s:] ", __FUNCTION__);
    tgv->dump();
    printf(" tgv %p\n", tgv);
}

void generate_verilog(Module *Mod)
{
  uint64_t ***t;
  uint64_t **modfirst;
  GenericValue *Ptr;
  GlobalValue *gv;
  static DataLayout foo(Mod);

  TD = &foo;
  gv = Mod->getNamedValue("_ZN6Module5firstE");
  printf("\n\n");
  gv->dump();
  printf("[%s:%d] gv %p\n", __FUNCTION__, __LINE__, gv);
  printf("[%s:%d] gvname %s\n", __FUNCTION__, __LINE__, gv->getName().str().c_str());
  printf("[%s:%d] gvtype %p\n", __FUNCTION__, __LINE__, gv->getType());
  Ptr = (GenericValue *)EE->getPointerToGlobal(gv);
  printf("[%s:%d] ptr %p\n", __FUNCTION__, __LINE__, Ptr);
  modfirst = (uint64_t **)*(PointerTy*)Ptr;
  printf("[%s:%d] value of Module::first %p\n", __FUNCTION__, __LINE__, modfirst);
  dump_type(Mod, "class.Module");
  dump_type(Mod, "class.Rule");

  while (modfirst) { /* loop through all modules */
    printf("Module vtab %p rfirst %p next %p\n\n", modfirst[0], modfirst[1], modfirst[2]);
    dump_vtab((uint64_t ***)modfirst);
    t = (uint64_t ***)modfirst[1];        // Module.rfirst
    modfirst = (uint64_t **)modfirst[2]; // Module.next

    while (t) {      /* loop through all rules for this module */
      printf("Rule %p: vtab %p next %p module %p\n", t, t[0], t[1], t[2]);
      dump_vtab(t);
      t = (uint64_t ***)t[1];             // Rule.next
    }
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
            CLASS_META *classp = &class_data[class_data_index++];
            if (classinfo_array_index++ != 0) {
                printf(" recursiveclassdefmagic %s [%p] =**** %d level %d.\n", name.c_str(), val, metanumber, classinfo_array_index);
                //exit(1);
            }
            classinfo_array[classinfo_array_index].inherit = NULL;
            classinfo_array[classinfo_array_index].members.clear();
            dumpType(nextitem);
            int ind = name.find("<");
            if (ind >= 0)
                name = name.substr(0, ind);
            name = "class." + getScope(nextitem.getContext()) + name;
            int mcount = classinfo_array[classinfo_array_index].members.size();
            printf("class %s members %d:", name.c_str(), mcount);
            classp->name = strdup(name.c_str());
            classp->node = Node;
            classp->inherit = classinfo_array[classinfo_array_index].inherit;
            classp->member_count = 0;
            classp->member = (CLASS_META_MEMBER *)malloc(sizeof(CLASS_META_MEMBER) * mcount);
            for (std::list<const MDNode *>::iterator MI = classinfo_array[classinfo_array_index].members.begin(),
                ME = classinfo_array[classinfo_array_index].members.end(); MI != ME; MI++) {
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
                printf(" %s", cp);
                int j = classp->member_count++;
                classp->member[j].node = Node;
                classp->member[j].name = strdup(cp);
            }
            printf("\n");
            std::map<std::string, const MDNode *>::iterator CI = classmap.find(name);
            if (CI != classmap.end()) {
                printf(" duplicateclassdefmagic %s [%p] =**** %d\n", name.c_str(), val, metanumber);
                //exit(1);
            }
            classinfo_array_index--;
            classmap[name] = Node;
        }
    }
}

static void dumpType(DIType litem)
{
    int tag = litem.getTag();
    if (!tag)     // Ignore elements with tag of 0
        return;
    if (tag == dwarf::DW_TAG_pointer_type) {
        DICompositeType CTy(litem);
        dumpTref(CTy.getTypeDerivedFrom());
        return;
    }
    if (tag == dwarf::DW_TAG_inheritance) {
        DICompositeType CTy(litem);
        DIArray Elements = CTy.getTypeArray();
        const Value *v = CTy.getTypeDerivedFrom();
        const MDNode *Node;
        if (v && (Node = dyn_cast<MDNode>(v))) {
            if(classinfo_array[classinfo_array_index].inherit) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
     exit(1);
  }
            classinfo_array[classinfo_array_index].inherit = Node;
        }
        dumpTref(v);
        return;
    }
    if (trace_meta)
    printf(" tag %s name %s off %3ld size %3ld",
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
                classinfo_array[classinfo_array_index].members.push_back(Node);
            }
            dumpType(Ty);
        }
    }
}
void format_type(DIType DT)
{
    dumpType(DT);
    if (trace_meta) {
    fprintf(stderr, "    %s:", __FUNCTION__);
    if (DT.isBasicType())
      if (const char *Enc = dwarf::AttributeEncodingString(DIBasicType(DT).getEncoding()))
        errs() << ", enc " << Enc;
    if (DT.isPrivate()) errs() << " [private]";
    else if (DT.isProtected()) errs() << " [protected]";
    if (DT.isArtificial()) errs() << " [artificial]";
    if (DT.isForwardDecl()) errs() << " [decl]";
    else if (DT.getTag() == dwarf::DW_TAG_structure_type || DT.getTag() == dwarf::DW_TAG_union_type ||
             DT.getTag() == dwarf::DW_TAG_enumeration_type || DT.getTag() == dwarf::DW_TAG_class_type)
      errs() << " [def]";
    if (DT.isDerivedType()) {
       errs() << " [from ";
       errs() << DIDerivedType(DT).getTypeDerivedFrom().getName();
       errs() << ']';
    }
    fprintf(stderr, "\n");
    }
    if (DT.getTag() == dwarf::DW_TAG_structure_type) {
        DICompositeType CTy = DICompositeType(DT);
        //StringRef Name = CTy.getName();
        DIArray Elements = CTy.getTypeArray();
        for (unsigned i = 0, N = Elements.getNumElements(); i < N; ++i) {
            DIDescriptor Element = Elements.getElement(i);
            if (trace_meta) {
            fprintf(stderr, "fmtstruct elt:");
            Element.dump();
            }
        }
    }
}
void processSubprogram(DISubprogram sub)
{
  if (trace_meta) {
  printf("Subprogram: %s", sub.getName().str().c_str());
  printf(" %s", sub.getLinkageName().str().c_str());
  printf(" %d", sub.getVirtuality());
  printf(" %d", sub.getVirtualIndex());
  printf(" %d", sub.getFlags());
  printf(" %d", sub.getScopeLineNumber());
  printf("\n");
  dumpTref(sub.getContainingType());
  }
  //if (MDNode *Temp = sub.getVariablesNodes()) {
      //printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      //DIType vn(Temp);
      //vn.dump();
  //}
  //DIArray variab(sub.getVariables());
  //printf("variab: ");
  //for (unsigned j = 0, je = variab.getNumElements(); j != je; j++) {
     //DIVariable ee(variab.getElement(j));
     //printf("[%s:%d] %d/%d name '%s' arg %d line %d\n", __FUNCTION__, __LINE__, j, je, ee.getName().str().c_str(), ee.getArgNumber(), ee.getLineNumber());
     //ee.dump();
  //}
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
  //fprintf(stderr, "func: ");
  //sub.getFunction()->dump();
  //printf("fdecl: ");
  //processSubprogram(DISubprogram(sub.getFunctionDeclaration()));
  dumpType(DICompositeType(sub.getType()));
}

void dump_metadata(Module *Mod)
{
  NamedMDNode *CU_Nodes = Mod->getNamedMetadata("llvm.dbg.cu");

  if (!CU_Nodes)
    return;
  for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
    DICompileUnit CU(CU_Nodes->getOperand(i));
    if (trace_meta)
    printf("\n%s: compileunit %d:%s %s\n", __FUNCTION__, CU.getLanguage(),
         // from DIScope:
         CU.getDirectory().str().c_str(), CU.getFilename().str().c_str());
    DIArray GVs = CU.getGlobalVariables();
    for (unsigned i = 0, e = GVs.getNumElements(); i != e; ++i) {
      DIGlobalVariable DIG(GVs.getElement(i));
      if (trace_meta) {
      printf("[%s:%d]globalvar\n", __FUNCTION__, __LINE__);
      DIG.dump();
      }
      format_type(DIG.getType());
    }
    DIArray SPs = CU.getSubprograms();
    for (unsigned i = 0, e = SPs.getNumElements(); i != e; ++i) {
      // dump methods
      if (trace_meta)
      printf("[%s:%d]****** methods %d/%d\n", __FUNCTION__, __LINE__, i, e);
      processSubprogram(DISubprogram(SPs.getElement(i)));
    }
    DIArray EnumTypes = CU.getEnumTypes();
    for (unsigned i = 0, e = EnumTypes.getNumElements(); i != e; ++i) {
      if (trace_meta)
      printf("[%s:%d]enumtypes\n", __FUNCTION__, __LINE__);
      format_type(DIType(EnumTypes.getElement(i)));
    }
    DIArray RetainedTypes = CU.getRetainedTypes();
    for (unsigned i = 0, e = RetainedTypes.getNumElements(); i != e; ++i) {
      if (trace_meta)
      printf("[%s:%d]retainedtypes\n", __FUNCTION__, __LINE__);
      format_type(DIType(RetainedTypes.getElement(i)));
    }
    DIArray Imports = CU.getImportedEntities();
    for (unsigned i = 0, e = Imports.getNumElements(); i != e; ++i) {
      DIImportedEntity Import = DIImportedEntity(Imports.getElement(i));
      DIDescriptor Entity = Import.getEntity();
      if (Entity.isType()) {
        if (trace_meta)
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        format_type(DIType(Entity));
      }
      else if (Entity.isSubprogram()) {
        if (trace_meta) {
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        DISubprogram(Entity)->dump();
        }
      }
      else if (Entity.isNameSpace()) {
        //printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        //DINameSpace(Entity).getContext()->dump();
      }
      else
        printf("[%s:%d] entity not type/subprog/namespace\n", __FUNCTION__, __LINE__);
    }
  }
}

static void dump_list(Module *Mod, const char *cp, const char *style)
{
    GlobalValue *gv = Mod->getNamedValue(cp);
    printf("dump_list: [%s] = %p\n", style, gv);
    PointerTy *Ptr = (PointerTy *)EE->getPointerToGlobal(gv);
    printf("dump_list: [%s] = %p\n", gv->getName().str().c_str(), Ptr);
    uint64_t **first = (uint64_t **)*Ptr;
    while (first) {                         // loop through linked list
        printf("dump_list[%s]: %p {vtab %p next %p}\n", style, first, first[0], first[1]);
        //dump_vtab((uint64_t ***)first);
        first = (uint64_t **)first[1];        // first = first->next
    }
}

int main(int argc, char **argv, char * const *envp)
{
  SMDiagnostic Err;
  std::string ErrorMsg;
  int Result;

printf("[%s:%d] start\n", __FUNCTION__, __LINE__);
  outputFile = fopen("output.tmp", "w");
  if (output_stdout)
      outputFile = stdout;
  if (dump_interpret)
      DebugFlag = true;
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  LLVMContext &Context = getGlobalContext();

  cl::ParseCommandLineOptions(argc, argv, "llvm interpreter & dynamic compiler\n");

  // load the bitcode...
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

  EE = builder.create();
  if (!EE) {
    if (!ErrorMsg.empty())
      printf("%s: error creating EE: %s\n", argv[0], ErrorMsg.c_str());
    else
      printf("%s: unknown error creating EE!\n", argv[0]);
    exit(1);
  }

  // load any additional modules specified on the command line.
  for (unsigned i = 0, e = ExtraModules.size(); i != e; ++i) {
    Module *XMod = llvm_ParseIRFile(ExtraModules[i], Err, Context);
    if (!XMod) {
      Err.print(argv[0], errs());
      return 1;
    }
    EE->addModule(XMod);
  }
  EE->DisableLazyCompilation(true);

  std::vector<std::string> InputArgv;
  InputArgv.insert(InputArgv.begin(), InputFile[0]);

  Function *EntryFn = Mod->getFunction("main");
  if (!EntryFn) {
    printf("'main' function not found in module.\n");
    return 1;
  }

  // Run static constructors.
  EE->runStaticConstructorsDestructors(false);

  for (Module::iterator I = Mod->begin(), E = Mod->end(); I != E; ++I) {
        Function *Fn = &*I;
        if (Fn != EntryFn && !Fn->isDeclaration())
          EE->getPointerToFunction(Fn);
  }

  // Trigger compilation separately so code regions that need to be invalidated will be known.
  (void)EE->getPointerToFunction(EntryFn);

  // Run main.
  Result = EE->runFunctionAsMain(EntryFn, InputArgv, envp);

  dump_metadata(Mod);

  generate_verilog(Mod);

  dump_class_data();

  dump_list(Mod, "_ZN12GuardedValueIiE5firstE", "GuardedValue");
  dump_list(Mod, "_ZN6ActionIiE5firstE", "Action");
printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
  return Result;
}
