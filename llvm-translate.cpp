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

class DITemp : public DIDescriptor {
  friend class DIDescriptor;
public:
  explicit DITemp(const MDNode *N = 0) : DIDescriptor(N) {}
  StringRef lgetStringField(unsigned Elt) const { return  getStringField(Elt); }
  uint64_t lgetUInt64Field(unsigned Elt) const { return  getUInt64Field(Elt); }
  int64_t lgetInt64Field(unsigned Elt) const { return  getInt64Field(Elt); }
  DIDescriptor lgetDescriptorField(unsigned Elt) const { return  getDescriptorField(Elt); }
  GlobalVariable *getGlobalVariableField(unsigned Elt) const { return getGlobalVariableField(Elt); }
  Constant *getConstantField(unsigned Elt) const { return getConstantField(Elt); }
  Function *getFunctionField(unsigned Elt) const { return getFunctionField(Elt); }
};

static ExecutionEngine *EE = 0;
static std::map<const Value *, int> slotmap;
static std::map<const MDNode *, int> metamap;
static int metanumber;
static struct {
    const char *name;
    int ignore_debug_info;
    uint8_t *svalue;
} slotarray[MAX_SLOTARRAY];
static int slotarray_index;

static uint64_t ***globalThis;
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
  slotarray_index = 0;
  memset(slotarray, 0, sizeof(slotarray));
}

void dump_type(Module *Mod, const char *p)
{
    StructType *tgv = Mod->getTypeByName(p);
    printf("%s:] ", __FUNCTION__);
    tgv->dump();
    printf(" tgv %p\n", tgv);
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
  if (const ConstantFP *CFP = dyn_cast<ConstantFP>(CV)) {
    printf("[%s:%d] floating point\n", __FUNCTION__, __LINE__);
    return;
  }

  if (isa<ConstantAggregateZero>(CV)) {
    printf( "zeroinitializer");
    return;
  }

  if (const BlockAddress *BA = dyn_cast<BlockAddress>(CV)) {
    printf( "blockaddress(");
    //WriteAsOperandInternal(BA->getFunction(), &TypePrinter, Machine, Context);
    //WriteAsOperandInternal(BA->getBasicBlock(), &TypePrinter, Machine, Context);
    return;
  }

  if (const ConstantArray *CA = dyn_cast<ConstantArray>(CV)) {
    Type *ETy = CA->getType()->getElementType();
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    //TypePrinter.print(ETy);
    //WriteAsOperandInternal(CA->getOperand(0), &TypePrinter, Machine, Context);
    for (unsigned i = 1, e = CA->getNumOperands(); i != e; ++i) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      //TypePrinter.print(ETy);
      //WriteAsOperandInternal(CA->getOperand(i), &TypePrinter, Machine, Context);
    }
    return;
  }

  if (const ConstantDataArray *CA = dyn_cast<ConstantDataArray>(CV)) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    // As a special case, print the array as a string if it is an array of
    // i8 with ConstantInt values.
    if (CA->isString()) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      //PrintEscapedString(CA->getAsString());
      return;
    }

    Type *ETy = CA->getType()->getElementType();
    //TypePrinter.print(ETy);
    //WriteAsOperandInternal(CA->getElementAsConstant(0), &TypePrinter, Machine, Context);
    for (unsigned i = 1, e = CA->getNumElements(); i != e; ++i) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      //TypePrinter.print(ETy);
      //WriteAsOperandInternal(CA->getElementAsConstant(i), &TypePrinter, Machine, Context);
    }
    return;
  }

  if (const ConstantStruct *CS = dyn_cast<ConstantStruct>(CV)) {
    unsigned N = CS->getNumOperands();
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    if (N) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      //TypePrinter.print(CS->getOperand(0)->getType());
      //WriteAsOperandInternal(CS->getOperand(0), &TypePrinter, Machine, Context);
      for (unsigned i = 1; i < N; i++) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        //TypePrinter.print(CS->getOperand(i)->getType());
        //WriteAsOperandInternal(CS->getOperand(i), &TypePrinter, Machine, Context);
      }
    }
    return;
  }

  if (isa<ConstantVector>(CV) || isa<ConstantDataVector>(CV)) {
    Type *ETy = CV->getType()->getVectorElementType();
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    //TypePrinter.print(ETy);
    //WriteAsOperandInternal(CV->getAggregateElement(0U), &TypePrinter, Machine, Context);
    for (unsigned i = 1, e = CV->getType()->getVectorNumElements(); i != e;++i){
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      //TypePrinter.print(ETy);
      //WriteAsOperandInternal(CV->getAggregateElement(i), &TypePrinter, Machine, Context);
    }
    return;
  }

  if (isa<ConstantPointerNull>(CV)) {
    printf( "null");
    return;
  }

  if (isa<UndefValue>(CV)) {
    printf( "undef");
    return;
  }

  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(CV)) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    //printf( CE->getOpcodeName();
    //WriteOptimizationInfo(CE);
    //if (CE->isCompare())
      //printf( ' ' << getPredicateText(CE->getPredicate());

    for (User::const_op_iterator OI=CE->op_begin(); OI != CE->op_end(); ++OI) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      //TypePrinter.print((*OI)->getType()); //WriteAsOperandInternal(*OI, &TypePrinter, Machine, Context);
    }

    if (CE->hasIndices()) {
      ArrayRef<unsigned> Indices = CE->getIndices();
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      for (unsigned i = 0, e = Indices.size(); i != e; ++i)
        printf( "%d ",  Indices[i]);
    }

    if (CE->isCast()) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      //TypePrinter.print(CE->getType());
    }
    return;
  }
  printf( "<placeholder or erroneous Constant>");
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
    if (N->isFunctionLocal()) {
      // Print metadata inline, not via slot reference number.
      //WriteMDNodeBodyInternal(N, TypePrinter, Machine, Context);
      return;
    }
    goto locallab;
  }

  if (const MDString *MDS = dyn_cast<MDString>(Operand)) {
printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, MDS->getString().str().c_str());
    //PrintEscapedString(MDS->getString());
    return;
  }

  if (Operand->getValueID() == Value::PseudoSourceValueVal ||
      Operand->getValueID() == Value::FixedStackPseudoSourceValueVal) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    return;
  }
locallab:
  slotindex = getLocalSlot(Operand);
  operand_list[operand_list_index].type = OpTypeLocalRef;
  operand_list[operand_list_index++].value = slotindex;
  if (!slotarray[slotindex].svalue && !slotarray[slotindex].name) {
      slotarray[slotindex].name = strdup(Operand->getName().str().c_str());
      if (!strcmp(slotarray[slotindex].name, "this")) {
           slotarray[slotindex].name = globalClassName;
           slotarray[slotindex].svalue = (uint8_t *)globalThis;
      }
  }
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
  assert(LoadBytes <= sizeof(*Dst));
  memcpy(Dst, Src, LoadBytes);
}
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
  operand_list_index = 0;
  memset(operand_list, 0, sizeof(operand_list));
  if (I.hasName()) {
    int t = getLocalSlot(&I);
    operand_list[operand_list_index].type = OpTypeLocalRef;
    operand_list[operand_list_index++].value = t;
    slotarray[t].name = strdup(I.getName().str().c_str());
    sprintf(instruction_label, "%10s/%d: ", slotarray[t].name, t);
  } else if (!I.getType()->isVoidTy()) {
    char temp[MAX_CHAR_BUFFER];
    int t = getLocalSlot(&I);
    operand_list[operand_list_index].type = OpTypeLocalRef;
    operand_list[operand_list_index++].value = t;
    sprintf(temp, "%%%d", t);
    slotarray[t].name = strdup(temp);
    sprintf(instruction_label, "%10s/%d: ", slotarray[t].name, t);
  }
  else {
    operand_list_index++;
    sprintf(instruction_label, "            : ");
  }
  if (isa<CallInst>(I) && cast<CallInst>(I).isTailCall())
    printf("tail ");
  const Value *Operand = I.getNumOperands() ? I.getOperand(0) : 0;
  if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
    const BranchInst &BI(cast<BranchInst>(I));
    writeOperand(BI.getCondition());
    writeOperand(BI.getSuccessor(0));
    writeOperand(BI.getSuccessor(1));
  } else if (isa<SwitchInst>(I)) {
    const SwitchInst& SI(cast<SwitchInst>(I));
    writeOperand(SI.getCondition());
    writeOperand(SI.getDefaultDest());
    for (SwitchInst::ConstCaseIt i = SI.case_begin(), e = SI.case_end(); i != e; ++i) {
      writeOperand(i.getCaseValue());
      writeOperand(i.getCaseSuccessor());
    }
  } else if (isa<IndirectBrInst>(I)) {
    writeOperand(Operand);
    for (unsigned i = 1, e = I.getNumOperands(); i != e; ++i) {
      writeOperand(I.getOperand(i));
    }
  } else if (const PHINode *PN = dyn_cast<PHINode>(&I)) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      I.getType()->dump();
    for (unsigned op = 0, Eop = PN->getNumIncomingValues(); op < Eop; ++op) {
      writeOperand(PN->getIncomingValue(op));
      writeOperand(PN->getIncomingBlock(op));
    }
  } else if (const ExtractValueInst *EVI = dyn_cast<ExtractValueInst>(&I)) {
    writeOperand(I.getOperand(0));
    for (const unsigned *i = EVI->idx_begin(), *e = EVI->idx_end(); i != e; ++i)
      printf(", %x", *i);
  } else if (const InsertValueInst *IVI = dyn_cast<InsertValueInst>(&I)) {
    writeOperand(I.getOperand(0));
    writeOperand(I.getOperand(1));
    for (const unsigned *i = IVI->idx_begin(), *e = IVI->idx_end(); i != e; ++i)
      printf(", %x", *i);
  } else if (const LandingPadInst *LPI = dyn_cast<LandingPadInst>(&I)) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      I.getType()->dump();
    printf(" personality ");
    writeOperand(I.getOperand(0));
    if (LPI->isCleanup())
      printf("          cleanup");
    for (unsigned i = 0, e = LPI->getNumClauses(); i != e; ++i) {
      if (i != 0 || LPI->isCleanup()) printf("\n");
      if (LPI->isCatch(i))
        printf("          catch ");
      else
        printf("          filter ");
      writeOperand(LPI->getClause(i));
    }
  } else if (const CallInst *CI = dyn_cast<CallInst>(&I)) {
    Operand = CI->getCalledValue();
    writeOperand(Operand);
  } else if (const InvokeInst *II = dyn_cast<InvokeInst>(&I)) {
    Operand = II->getCalledValue();
    writeOperand(Operand);
    writeOperand(II->getNormalDest());
    writeOperand(II->getUnwindDest());
  } else if (Operand) {   // Print the normal way.
    for (unsigned i = 0, E = I.getNumOperands(); i != E; ++i) {
      writeOperand(I.getOperand(i));
    }
  }
  char vout[MAX_CHAR_BUFFER];
  vout[0] = 0;
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
      printf("XLAT:            Br");
      break;
  //case Instruction::Switch:
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
      assert (operand_list[0].type == OpTypeLocalRef);
      slotarray[operand_list[0].value].name = strdup(temp);
      }
      break;

  // Memory instructions...
  //case Instruction::Alloca: // ignore
  case Instruction::Load:
      {
      printf("XLAT:          Load");
      PointerTy Ptr = (PointerTy)slotarray[operand_list[1].value].svalue;
      assert(operand_list[0].type == OpTypeLocalRef && operand_list[1].type == OpTypeLocalRef);
      slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
      Value *value = (Value *)LoadValueFromMemory(Ptr, I.getType());
      slotarray[operand_list[0].value].svalue = (uint8_t *)value;
      const GlobalValue *g = EE->getGlobalValueAtAddress(Ptr);
      printf("g2=%p;", g);
printf("[%s:%d] value %p\n", __FUNCTION__, __LINE__, value);
      if (g)  // remember the name of where this value came from
          slotarray[operand_list[0].value].name = strdup(g->getName().str().c_str());
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
      printf("XLAT: GetElementPtr");
      const char *ret = NULL;
      char temp[MAX_CHAR_BUFFER];
      uint64_t Total = executeGEPOperation(gep_type_begin(I), gep_type_end(I));
      printf(" GEP Index %lld;", (long long)Total);
      assert (slotarray[operand_list[1].value].svalue);
      uint8_t *ptr = slotarray[operand_list[1].value].svalue + Total;
      const char *cp = slotarray[operand_list[1].value].name;
      printf(" name %s;", cp);
      //if (!strcmp(cp, "this")) {
          const GlobalValue *g = EE->getGlobalValueAtAddress(ptr);
          //g = EE->getGlobalValueAtAddress(*(uint64_t **)ptr);
          //printf("g2=%p;", g);
          if (g)
              ret = g->getName().str().c_str();
          //cp = globalClassName;
      //}
      if (!ret) {
          sprintf(temp, "%s" SEPARATOR "%lld", cp, (long long)Total);
          ret = temp;
      }
      slotarray[operand_list[0].value].name = strdup(ret);
      slotarray[operand_list[0].value].svalue = ptr;
      }
      break;

  // Convert instructions...
  case Instruction::Trunc:
      printf("XLAT:         Trunc");
      assert(operand_list[0].type == OpTypeLocalRef && operand_list[1].type == OpTypeLocalRef);
      slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
      break;
  case Instruction::ZExt:
      printf("XLAT:          Zext");
      assert(operand_list[0].type == OpTypeLocalRef && operand_list[1].type == OpTypeLocalRef);
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
      assert (operand_list[0].type == OpTypeLocalRef && operand_list[1].type == OpTypeLocalRef);
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
      printf("XLAT:           PHI");
      break;
  //case Instruction::Select:
  case Instruction::Call:
      {
      printf("XLAT:          Call");
      assert (operand_list[1].type == OpTypeLocalRef);
      const Instruction *t = (const Instruction *)slotarray[operand_list[1].value].svalue;
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
  clearLocalSlot();
  if (updateFlag) {
      print_header();
      fprintf(outputFile, "end;\n");
  }
}

static void processFunction(Function *F)
{
  if (!F->isDeclaration()) {
      for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I)
          opt_runOnBasicBlock(*I);
      verilogFunction(F);
  }
}

void dump_vtab(uint64_t ***thisptr)
{
int arr_size = 0;
    const GlobalValue *g;
    globalThis = thisptr;
    uint64_t **vtab = (uint64_t **)thisptr[0];

    for (int i = 0; i < 16; i++) {
         g = EE->getGlobalValueAtAddress(thisptr -8 + i);
if (g)
printf("[%s:%d] %d g %p name %s\n", __FUNCTION__, __LINE__, -8 + i, g, g->getName().str().c_str());
    }
    for (int i = 0; i < 16; i++) {
         g = EE->getGlobalValueAtAddress(vtab - 8 + i);
if (g)
printf("[%s:%d]v %d g %p name %s\n", __FUNCTION__, __LINE__, - 8 + i, g, g->getName().str().c_str());
    }
    g = EE->getGlobalValueAtAddress(vtab-2);
    globalClassName = NULL;
    printf("[%s:%d] vtabbase %p g %p:\n", __FUNCTION__, __LINE__, vtab-2, g);
memdump(((unsigned char *)globalThis), 64, "THIS");
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
       printf("[%s:%d] [%d] p %p: %s, this %p\n", __FUNCTION__, __LINE__, i, vtab[i], globalName, globalThis);
       int rettype = f->getReturnType()->getTypeID();
       if ((rettype == Type::IntegerTyID || rettype == Type::VoidTyID)
        && strcmp(cend, "D0Ev") && strcmp(cend, "D1Ev")) {
           if (strlen(globalName) <= 18 || strcmp(globalName + (strlen(globalName)-18), "setModuleEP6Module")) {
               processFunction(f);
               printf("FULL:\n");
               f->dump();
               printf("\n");
               if (trace_break_name && !strcmp(globalName, trace_break_name)) {
                   printf("[%s:%d]\n", __FUNCTION__, __LINE__);
                   exit(1);
               }
           }
       }
    }
}

void generate_verilog(Module *Mod)
{
  uint64_t ***t;
  uint64_t **modfirst;
  GenericValue *Ptr;
  GlobalValue *gv;

static DataLayout foo = DataLayout(Mod);
  TD = &foo;
  std::string Name = "_ZN6Module5firstE";
  gv = Mod->getNamedValue(Name);
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

static void dumpType(DIType litem);
static void dumpTref(Value *val)
{
    if (!val)
        return;
#if 0
    Value *val = DIDerivedType(litem).getTypeDerivedFrom();
printf("[%s:%d]\n",__FUNCTION__, __LINE__);
    if (const MDNode *Node = dyn_cast<MDNode>(val)) {
printf("[%s:%d]got node\n", __FUNCTION__, __LINE__);
    printf( "JJ!{");
    for (unsigned mi = 0, me = Node->getNumOperands(); mi != me; ++mi) {
          const Value *V = Node->getOperand(mi);
          if (V == 0)
            printf( "null");
          else {
            //TypePrinter->print(V->getType(), Out);
            printf(" MMM");
            V->getType()->dump();
            if (const MDNode *Nodeinner = dyn_cast<MDNode>(V)) {
printf("[%s:%d]%d %d\n", __FUNCTION__, __LINE__, mi, Nodeinner->getNumOperands());
            }
          }
          if (mi + 1 != me)
            printf( ", ");
        }
        printf( "}");
    }
#endif
    if (const MDNode *Node = dyn_cast<MDNode>(val)) {
    DIType nextitem(Node);
    int tag = nextitem.getTag();
    printf("TRef: %s;", dwarf::TagString(tag));
    std::map<const MDNode *, int>::iterator FI = metamap.find(Node);
    if (FI != metamap.end())
        printf(" magic %p = ref %d\n", val, FI->second);
    else {
        printf(" magic %p =**** %d\n", val, metanumber);
        metamap[Node] = metanumber++;
        dumpType(nextitem);
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
    printf(" tag %s name %s off %3ld size %3ld",
        dwarf::TagString(tag), litem.getName().str().c_str(),
        (long)litem.getOffsetInBits()/8, (long)litem.getSizeInBits()/8);
    if (litem.getTag() == dwarf::DW_TAG_subprogram) {
        DISubprogram sub(litem);
        printf(" link %s", sub.getLinkageName().str().c_str());
    }
    printf("\n");
    DICompositeType CTy(litem);
    switch (tag) {
    case dwarf::DW_TAG_class_type:
    case dwarf::DW_TAG_array_type:
    case dwarf::DW_TAG_enumeration_type:
    case dwarf::DW_TAG_structure_type:
    case dwarf::DW_TAG_union_type:
    case dwarf::DW_TAG_inheritance:
        dumpTref(CTy.getTypeDerivedFrom());
        dumpTref(CTy.getContainingType());
        /* fall through */
    case dwarf::DW_TAG_subroutine_type:
        DIArray Elements = CTy.getTypeArray();
        for (unsigned k = 0, N = Elements.getNumElements(); k < N; ++k) {
            DIType Ty(Elements.getElement(k));
            dumpType(Ty);
        }
        break;
    }
}
void format_type(DIType DT)
{
    fprintf(stderr, "    %s:", __FUNCTION__);
    dumpType(DT);
    //fprintf(stderr, "[ %s ]", dwarf::TagString(DT.getTag()));
    //StringRef Res = DT.getName();
    //if (!Res.empty())
      //errs() << " [" << Res << "]";
    //errs() << " [line " << DT.getLineNumber() << ", size " << DT.getSizeInBits() << ", align " << DT.getAlignInBits() << ", offset " << DT.getOffsetInBits();
    if (DT.isBasicType())
      if (const char *Enc = dwarf::AttributeEncodingString(DIBasicType(DT).getEncoding()))
        errs() << ", enc " << Enc;
    //errs() << "]";
    if (DT.isPrivate()) errs() << " [private]";
    else if (DT.isProtected()) errs() << " [protected]";
    if (DT.isArtificial()) errs() << " [artificial]";
    if (DT.isForwardDecl()) errs() << " [decl]";
    else if (DT.getTag() == dwarf::DW_TAG_structure_type || DT.getTag() == dwarf::DW_TAG_union_type ||
             DT.getTag() == dwarf::DW_TAG_enumeration_type || DT.getTag() == dwarf::DW_TAG_class_type)
      errs() << " [def]";
    if (DT.isVector()) errs() << " [vector]";
    if (DT.isStaticMember()) errs() << " [static]";
    if (DT.isDerivedType()) {
       errs() << " [from ";
       errs() << DIDerivedType(DT).getTypeDerivedFrom().getName();
       errs() << ']';
    }
    fprintf(stderr, "\n");
    if (DT.getTag() == dwarf::DW_TAG_structure_type) {
        DICompositeType CTy = DICompositeType(DT);
        //StringRef Name = CTy.getName();
        DIArray Elements = CTy.getTypeArray();
        for (unsigned i = 0, N = Elements.getNumElements(); i < N; ++i) {
            DIDescriptor Element = Elements.getElement(i);
            fprintf(stderr, "fmtstruct elt:");
            Element.dump();
        }
    }
}
void processSubprogram(DISubprogram sub)
{
  printf("Subprogram: %s", sub.getName().str().c_str());
  printf(" %s", sub.getLinkageName().str().c_str());
  printf(" %d", sub.getLineNumber());
  printf(" %d", sub.isLocalToUnit());
  printf(" %d", sub.isDefinition());
  printf(" %d", sub.getVirtuality());
  printf(" %d", sub.getVirtualIndex());
  printf(" %d", sub.getFlags());
  printf(" %d", sub.getScopeLineNumber());
  printf("\n");
  dumpTref(sub.getContainingType());
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
      printf("tparam: ");
      for (unsigned j = 0, je = tparam.getNumElements(); j != je; j++) {
        printf("[%s:%d] %d/%d\n", __FUNCTION__, __LINE__, j, je);
         DIDescriptor ee(tparam.getElement(j));
         ee.dump();
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
    printf("\n%s: compileunit %d:%s %s\n", __FUNCTION__, CU.getLanguage(),
         // from DIScope:
         CU.getDirectory().str().c_str(), CU.getFilename().str().c_str());
    DIArray GVs = CU.getGlobalVariables();
    for (unsigned i = 0, e = GVs.getNumElements(); i != e; ++i) {
      DIGlobalVariable DIG(GVs.getElement(i));
      printf("[%s:%d]globalvar\n", __FUNCTION__, __LINE__);
      DIG.dump();
      format_type(DIG.getType());
    }
    DIArray SPs = CU.getSubprograms();
    for (unsigned i = 0, e = SPs.getNumElements(); i != e; ++i) {
      // dump methods
      printf("[%s:%d]****** methods %d/%d\n", __FUNCTION__, __LINE__, i, e);
      processSubprogram(DISubprogram(SPs.getElement(i)));
    }
    DIArray EnumTypes = CU.getEnumTypes();
    for (unsigned i = 0, e = EnumTypes.getNumElements(); i != e; ++i) {
      printf("[%s:%d]enumtypes\n", __FUNCTION__, __LINE__);
      format_type(DIType(EnumTypes.getElement(i)));
    }
    DIArray RetainedTypes = CU.getRetainedTypes();
    for (unsigned i = 0, e = RetainedTypes.getNumElements(); i != e; ++i) {
      printf("[%s:%d]retainedtypes\n", __FUNCTION__, __LINE__);
      format_type(DIType(RetainedTypes.getElement(i)));
    }
    DIArray Imports = CU.getImportedEntities();
    for (unsigned i = 0, e = Imports.getNumElements(); i != e; ++i) {
      DIImportedEntity Import = DIImportedEntity(Imports.getElement(i));
      DIDescriptor Entity = Import.getEntity();
      if (Entity.isType()) {
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        format_type(DIType(Entity));
      }
      else if (Entity.isSubprogram()) {
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        DISubprogram(Entity)->dump();
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

  generate_verilog(Mod);

  dump_metadata(Mod);
printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
  return Result;
}
