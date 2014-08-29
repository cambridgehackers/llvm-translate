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
#include "llvm/ADT/Triple.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/IRBuilder.h"
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

static int dump_interpret;// = 1;
static int output_stdout;// = 1;

using namespace llvm;

namespace {
  cl::list<std::string>
  InputFile(cl::Positional, cl::OneOrMore, cl::desc("<input bitcode>"));

  cl::opt<std::string>
  MArch("march",
        cl::desc("Architecture to generate assembly for (see --version)"));

  cl::list<std::string>
  MAttrs("mattr",
         cl::CommaSeparated,
         cl::desc("Target specific attributes (-mattr=help for details)"),
         cl::value_desc("a1,+a2,-a3,..."));

  cl::list<std::string>
  ExtraModules("extra-module",
         cl::desc("Extra modules to be loaded"),
         cl::value_desc("input bitcode"));
}

#define MAX_SLOTARRAY 1000
#define MAX_OPERAND_LIST 200
#define MAX_CHAR_BUFFER 1000

static ExecutionEngine *EE = 0;
std::map<const Value *, int> slotmap;
struct {
    const char *name;
    uint64_t ***value;
} slotarray[MAX_SLOTARRAY];
static int slotarray_index;

static uint64_t ***globalThis;
static const char *globalName;
static const char *globalClassName;
static FILE *outputFile;
enum {OpTypeNone, OpTypeInt, OpTypeLocalRef, OpTypeExternalFunction, OpTypeString};
static struct {
   int type;
   uint64_t value;
} operand_list[MAX_OPERAND_LIST];
static int operand_list_index;

void makeLocalSlot(const Value *V)
{
  slotmap.insert(std::pair<const Value *, int>(V, slotarray_index++));
}
int getLocalSlot(const Value *V)
{
  //assert(!isa<Constant>(V) && "Can't get a constant or global slot with this!"); 
  std::map<const Value *, int>::iterator FI = slotmap.find(V);
  //return FI == slotmap.end() ? -1 : (int)FI->second;
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
    //TypePrinter.print(ETy);
    //WriteAsOperandInternal(CA->getOperand(0), &TypePrinter, Machine, Context);
    for (unsigned i = 1, e = CA->getNumOperands(); i != e; ++i) {
      //TypePrinter.print(ETy);
      //WriteAsOperandInternal(CA->getOperand(i), &TypePrinter, Machine, Context);
    }
    return;
  }

  if (const ConstantDataArray *CA = dyn_cast<ConstantDataArray>(CV)) {
    // As a special case, print the array as a string if it is an array of
    // i8 with ConstantInt values.
    if (CA->isString()) {
      //PrintEscapedString(CA->getAsString());
      return;
    }

    Type *ETy = CA->getType()->getElementType();
    //TypePrinter.print(ETy);
    //WriteAsOperandInternal(CA->getElementAsConstant(0), &TypePrinter, Machine, Context);
    for (unsigned i = 1, e = CA->getNumElements(); i != e; ++i) {
      //TypePrinter.print(ETy);
      //WriteAsOperandInternal(CA->getElementAsConstant(i), &TypePrinter, Machine, Context);
    }
    return;
  }

  if (const ConstantStruct *CS = dyn_cast<ConstantStruct>(CV)) {
    unsigned N = CS->getNumOperands();
    if (N) {
      //TypePrinter.print(CS->getOperand(0)->getType());
      //WriteAsOperandInternal(CS->getOperand(0), &TypePrinter, Machine, Context); 
      for (unsigned i = 1; i < N; i++) {
        //TypePrinter.print(CS->getOperand(i)->getType());
        //WriteAsOperandInternal(CS->getOperand(i), &TypePrinter, Machine, Context);
      }
    }
    return;
  }

  if (isa<ConstantVector>(CV) || isa<ConstantDataVector>(CV)) {
    Type *ETy = CV->getType()->getVectorElementType();
    //TypePrinter.print(ETy);
    //WriteAsOperandInternal(CV->getAggregateElement(0U), &TypePrinter, Machine, Context);
    for (unsigned i = 1, e = CV->getType()->getVectorNumElements(); i != e;++i){
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
    //printf( CE->getOpcodeName();
    //WriteOptimizationInfo(CE);
    //if (CE->isCompare())
      //printf( ' ' << getPredicateText(CE->getPredicate());

    for (User::const_op_iterator OI=CE->op_begin(); OI != CE->op_end(); ++OI) {
      //TypePrinter.print((*OI)->getType()); //WriteAsOperandInternal(*OI, &TypePrinter, Machine, Context);
    }

    if (CE->hasIndices()) {
      ArrayRef<unsigned> Indices = CE->getIndices();
      for (unsigned i = 0, e = Indices.size(); i != e; ++i)
        printf( "%d ",  Indices[i]);
    }

    if (CE->isCast()) {
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
  if (!slotarray[slotindex].value) {
      slotarray[slotindex].value = (uint64_t ***) Operand;
      slotarray[slotindex].name = strdup(Operand->getName().str().c_str());
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

// This member is called for each Instruction in a function..
void printInstruction(const Instruction &I) 
{
  int opcode = I.getOpcode();
  char instruction_label[MAX_CHAR_BUFFER];
  operand_list_index = 0;
  memset(operand_list, 0, sizeof(operand_list));
  if (I.hasName()) {
    int t = getLocalSlot(&I);
    operand_list[operand_list_index].type = OpTypeLocalRef;
    operand_list[operand_list_index++].value = t;
    slotarray[t].value = (uint64_t ***) &I;
    slotarray[t].name = strdup(I.getName().str().c_str());
    sprintf(instruction_label, "%10s/%d: ", slotarray[t].name, t);
  } else if (!I.getType()->isVoidTy()) {
    char temp[MAX_CHAR_BUFFER];
    // Print out the def slot taken.
    int t = getLocalSlot(&I);
    operand_list[operand_list_index].type = OpTypeLocalRef;
    operand_list[operand_list_index++].value = t;
    sprintf(temp, "%%%d", t);
    slotarray[t].value = (uint64_t ***) &I;
    slotarray[t].name = strdup(temp);
    //sprintf(instruction_label, "%12d: ",  t);
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
    PointerType *PTy = cast<PointerType>(Operand->getType());
    FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
    //Type *RetTy = FTy->getReturnType();
    //if (!FTy->isVarArg() && (!RetTy->isPointerTy() || !cast<PointerType>(RetTy)->getElementType()->isFunctionTy())) {
//printf("[%s:%d] shortformcall dumpreturntype\n", __FUNCTION__, __LINE__);
      //RetTy->dump();
    //}
    writeOperand(Operand);
    for (unsigned op = 0, Eop = CI->getNumArgOperands(); op < Eop; ++op) {
      ///writeParamOperand(CI->getArgOperand(op), PAL, op + 1);
    }
  } else if (const InvokeInst *II = dyn_cast<InvokeInst>(&I)) {
    Operand = II->getCalledValue();
    PointerType *PTy = cast<PointerType>(Operand->getType());
    FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
    Type *RetTy = FTy->getReturnType();
    if (!FTy->isVarArg() &&
        (!RetTy->isPointerTy() ||
         !cast<PointerType>(RetTy)->getElementType()->isFunctionTy())) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      RetTy->dump();
    }
    writeOperand(Operand);
    for (unsigned op = 0, Eop = II->getNumArgOperands(); op < Eop; ++op) {
      //writeParamOperand(II->getArgOperand(op), PAL, op + 1);
    }
    writeOperand(II->getNormalDest());
    writeOperand(II->getUnwindDest());
  } else if (const AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
    //TypePrinter.print(AI->getAllocatedType());
    if (!AI->getArraySize() || AI->isArrayAllocation()) {
      writeOperand(AI->getArraySize());
    }
  } else if (isa<CastInst>(I)) {
    writeOperand(Operand);
    //TypePrinter.print(I.getType());
  } else if (isa<VAArgInst>(I)) {
    writeOperand(Operand);
    //TypePrinter.print(I.getType());
  } else if (Operand) {   // Print the normal way.
    for (unsigned i = 0, E = I.getNumOperands(); i != E; ++i) {
      writeOperand(I.getOperand(i));
    }
  }
  // Print Metadata info.
  SmallVector<std::pair<unsigned, MDNode*>, 4> InstMD;
  I.getAllMetadata(InstMD);
  if (!InstMD.empty()) {
    SmallVector<StringRef, 8> MDNames;
    I.getType()->getContext().getMDKindNames(MDNames);
    for (unsigned i = 0, e = InstMD.size(); i != e; ++i) {
      unsigned Kind = InstMD[i].first;
       if (Kind < MDNames.size()) {
         //printf(", !%s", MDNames[Kind].str().c_str());
       } else {
         //printf(", !<unknown kind #%d>", Kind);
       }
      //InstMD[i].second->dump();
      //WriteAsOperandInternal(InstMD[i].second, &TypePrinter, &Machine, TheModule);
    }
  }
  char vout[MAX_CHAR_BUFFER];
  vout[0] = 0;
  switch (opcode) {
  case Instruction::Alloca:
      return;  // ignore
  case Instruction::Call:
      if (operand_list[1].type == OpTypeExternalFunction && !strcmp((const char *)operand_list[1].value, "llvm.dbg.declare"))
          return;  // ignore
      if (operand_list[1].type == OpTypeExternalFunction && !strcmp((const char *)operand_list[1].value, "printf"))
          return;  // ignore for now
      break;
  };
  printf("%s    ", instruction_label);
  switch (opcode) {
  // Terminators
  case Instruction::Ret:
      printf("XLAT:           Ret");
      if (operand_list_index > 1) {
          operand_list[0].type = OpTypeString;
          operand_list[0].value = (uint64_t)getparam(1);
          sprintf(vout, "%s = %s && %s_enable;", globalName, getparam(1), globalName);
      }
      break;
  //case Instruction::Br:
  //case Instruction::Switch:
  //case Instruction::IndirectBr:
  //case Instruction::Invoke:
  //case Instruction::Resume:
  case Instruction::Unreachable:
      printf("XLAT:   Unreachable");
      break;

  // Standard binary operators...
  case Instruction::Add:
  //case Instruction::FAdd:
  //case Instruction::Sub:
  //case Instruction::FSub:
  //case Instruction::Mul:
  //case Instruction::FMul:
  //case Instruction::UDiv:
  //case Instruction::SDiv:
  //case Instruction::FDiv:
  //case Instruction::URem:
  //case Instruction::SRem:
  //case Instruction::FRem:

  // Logical operators...
  //case Instruction::And:
  //case Instruction::Or:
  //case Instruction::Xor:
      {
      const char *op1 = getparam(1), *op2 = getparam(2), *opstr = "+";
      char temp[MAX_CHAR_BUFFER];
      temp[0] = 0;
      printf("XLAT:           Add");
      sprintf(temp, "((%s) %s (%s))", op1, opstr, op2);
      if (operand_list[0].type == OpTypeLocalRef)
          slotarray[operand_list[0].value].name = strdup(temp);
      }
      break;

  // Memory instructions...
  //case Instruction::Alloca: // ignore
  case Instruction::Load:
      printf("XLAT:          Load");
      if (operand_list[0].type == OpTypeLocalRef && operand_list[1].type == OpTypeLocalRef)
          slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
      break;
  case Instruction::Store:
      printf("XLAT:         Store");
      sprintf(vout, "%s = %s;", getparam(2), getparam(1));
      if (operand_list[1].type == OpTypeLocalRef && operand_list[2].type == OpTypeLocalRef)
          slotarray[operand_list[1].value] = slotarray[operand_list[2].value];
      break;
  //case Instruction::AtomicCmpXchg:
  //case Instruction::AtomicRMW:
  //case Instruction::Fence:
  case Instruction::GetElementPtr:
      {
      printf("XLAT: GetElementPtr");
      const char *ret = NULL;
      if (operand_list_index >= 3 && operand_list[1].type == OpTypeLocalRef) {
        char temp[MAX_CHAR_BUFFER];
        const char *cp = slotarray[operand_list[1].value].name;
printf("[%s:%d] name %s\n", __FUNCTION__, __LINE__, cp);
        if (!strcmp(cp, "this")) {
          uint64_t **val = globalThis[1+operand_list[3].value];
          const GlobalValue *g = EE->getGlobalValueAtAddress((void *)val);
printf("[%s:%d] glo %p g %p gcn %s\n", __FUNCTION__, __LINE__, globalThis, g, globalClassName);
          if (g)
              ret = g->getName().str().c_str();
          else {
              sprintf(temp, "%s_%lld", globalClassName, (long long)operand_list[3].value);
              ret = temp;
          }
        }
        else {
          char temp[MAX_CHAR_BUFFER];
          sprintf(temp, "%s_%lld", cp, (long long)operand_list[3].value);
          ret = strdup(temp);
        }
      }
      if (operand_list[0].type == OpTypeLocalRef && ret)
          slotarray[operand_list[0].value].name = strdup(ret);
      }
      break;

  // Convert instructions...
  //case Instruction::Trunc:
  case Instruction::ZExt:
      printf("XLAT:          Zext");
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
      }
      break;
  //case Instruction::FCmp:
  //case Instruction::PHI:
  //case Instruction::Select:
  case Instruction::Call:
      printf("XLAT:          Call");
      if (operand_list[1].type == OpTypeLocalRef) {
          uint64_t ***t = (uint64_t ***)slotarray[operand_list[1].value].value;
          printf ("[op %p %p %p]", t, *t, **t);
          const GlobalValue *g = EE->getGlobalValueAtAddress(t-2);
printf("[%s:%d] g %p\n", __FUNCTION__, __LINE__, g);
          g = EE->getGlobalValueAtAddress(*t-2);
printf("[%s:%d] g %p\n", __FUNCTION__, __LINE__, g);
          g = EE->getGlobalValueAtAddress(**t-2);
printf("[%s:%d] g %p\n", __FUNCTION__, __LINE__, g);
      }
//jca
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
      break;
  }
  for (int i = 0; i < operand_list_index; i++) {
      if (operand_list[i].type == OpTypeLocalRef)
          printf(" op[%d]L=%d:%p:%s;", i, operand_list[i].value, slotarray[operand_list[i].value].value, slotarray[operand_list[i].value].name);
      else if (operand_list[i].type != OpTypeNone)
          printf(" op[%d]=%s;", i, getparam(i));
  }
  printf("\n");
  if (strlen(vout))
     fprintf(outputFile, "        %s\n", vout);
}

void printBasicBlock(const BasicBlock *BB) 
{
  if (BB->hasName()) {              // Print out the label if it exists...
    //PrintLLVMName(BB->getName(), LabelPrefix);
  } else if (!BB->use_empty()) {      // Don't print block # of no uses...
    //printf("\n); <label>:";
    //int Slot = Machine.getLocalSlot(BB);
    //if (Slot != -1)
      //printf(Slot);
    //else
      //printf("<badref>");
  }
  if (BB->getParent() == 0) {
    printf("); Error: Block without parent!");
  } else if (BB != &BB->getParent()->getEntryBlock()) {  // Not the entry block?
#if 0
    const_pred_iterator PI = pred_begin(BB), PE = pred_end(BB);
    if (PI == PE) {
    } else {
      writeOperand(*PI);
      for (++PI; PI != PE; ++PI) {
        writeOperand(*PI);
      }
    }
#endif
  }
  for (BasicBlock::const_iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
    printInstruction(*I);
  }
}

static void processFunction(const Function *F) 
{
  //if (F->isMaterializable())
    //printf("); Materializable\n";
  const AttributeSet &Attrs = F->getAttributes();
  if (Attrs.hasAttributes(AttributeSet::FunctionIndex)) {
    AttributeSet AS = Attrs.getFnAttributes();
    std::string AttrStr;
    unsigned Idx = 0;
    for (unsigned E = AS.getNumSlots(); Idx != E; ++Idx)
      if (AS.getSlotIndex(Idx) == AttributeSet::FunctionIndex)
        break;
    for (AttributeSet::iterator I = AS.begin(Idx), E = AS.end(Idx); I != E; ++I) {
      Attribute Attr = *I;
      if (!Attr.isStringAttribute()) {
        if (!AttrStr.empty()) AttrStr += ' ';
        AttrStr += Attr.getAsString();
      }
    }
    //if (!AttrStr.empty())
      //printf("); Function Attrs: " << AttrStr << '\n';
  }
  //if (F->isDeclaration()) printf("declare "); else printf("define ");
  //PrintLinkage(F->getLinkage());
  //PrintVisibility(F->getVisibility());
  // Print the calling convention.
  if (F->getCallingConv() != CallingConv::C) {
    //PrintCallingConv(F->getCallingConv());
  }
  FunctionType *FT = F->getFunctionType();
  //if (Attrs.hasAttributes(AttributeSet::ReturnIndex))
    //printf( Attrs.getAsString(AttributeSet::ReturnIndex) << ' ');
  //WriteAsOperandInternal(F, &TypePrinter, &Machine, F->getParent());
  //Machine.incorporateFunction(F);
  // Loop over the arguments, printing them...
  unsigned Idx = 1;
  if (!F->isDeclaration()) {
    // If this isn't a declaration, print the argument names as well.
    for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
      //TypePrinter.print(I->getType());
      //printf("[%s:%d] dumptype\n", __FUNCTION__, __LINE__);
      //I->getType()->dump();
      //if (Attrs.hasAttributes(Idx))
        //Attrs.getAsString(Idx);
      //if (I->hasName()) {
        //PrintLLVMName(I);
      //}
      Idx++;
    }
  } else {
    // Otherwise, print the types from the function type.
    for (unsigned i = 0, e = FT->getNumParams(); i != e; ++i) {
      //TypePrinter.print(FT->getParamType(i));
//printf("[%s:%d] dumptype\n", __FUNCTION__, __LINE__);
      //FT->getParamType(i)->dump();
      //if (Attrs.hasAttributes(i+1))
        //printf(' ' << Attrs.getAsString(i+1));
    }
  }
  // Finish printing arguments...
  if (FT->isVarArg()) {
  }
  //if (F->hasUnnamedAddr()) printf(" unnamed_addr");
  //if (Attrs.hasAttributes(AttributeSet::FunctionIndex)) //printf(" #" << Machine.getAttributeGroupSlot(Attrs.getFnAttributes()));
  //if (F->hasSection()) { //PrintEscapedString(F->getSection()); }
  if (F->hasPrefixData()) {
    printf(" prefix ");
    writeOperand(F->getPrefixData());
  }

  //if (F->getReturnType()->getTypeID() == Type::IntegerTyID)
  int updateFlag = strlen(globalName) > 8 && !strcmp(globalName + strlen(globalName) - 8, "updateEv");
  char temp[MAX_CHAR_BUFFER];
  strcpy(temp, globalName);
  if (updateFlag) {
      strcat(temp + strlen(globalName) - 8, "guardEv");
      fprintf(outputFile, "if (%s) then begin\n", temp);
  }
  if (!F->isDeclaration()) {
    for (Function::const_iterator I = F->begin(), E = F->end(); I != E; ++I)
      printBasicBlock(I);
  }
  clearLocalSlot();
  if (updateFlag)
      fprintf(outputFile, "end;\n");
}

void dump_vtab(uint64_t ***thisptr)
{
int arr_size = 0;
    globalThis = thisptr;
    uint64_t **vtab = (uint64_t **)thisptr[0];
    const GlobalValue *g = EE->getGlobalValueAtAddress(vtab-2);
    globalClassName = NULL;
    printf("[%s:%d] vtabbase %p g %p:\n", __FUNCTION__, __LINE__, vtab-2, g);
    if (g) {
        globalClassName = strdup(g->getName().str().c_str());
        if (g->getType()->getTypeID() == Type::PointerTyID) {
           Type *ty = g->getType()->getElementType();
           if (ty->getTypeID() == Type::ArrayTyID) {
               ArrayType *aty = cast<ArrayType>(ty);
               arr_size = aty->getNumElements();
           }
        }
        //g->getType()->dump();
    }
    for (int i = 0; i < arr_size-2; i++) {
       Function *f = (Function *)(vtab[i]);
       globalName = strdup(f->getName().str().c_str());
       const char *cend = globalName + (strlen(globalName)-4);
       printf("[%s:%d] [%d] p %p: %s, this %p\n", __FUNCTION__, __LINE__, i, vtab[i], globalName, globalThis);
       if (strcmp(cend, "D0Ev") && strcmp(cend, "D1Ev")) {
           if (strlen(globalName) <= 18 || strcmp(globalName + (strlen(globalName)-18), "setModuleEP6Module")) {
               fprintf(outputFile, "\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n; %s\n;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;\n", globalName);
               processFunction(f);
               printf("FULL:\n");
               f->dump();
               printf("\n");
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
printf("[%s:%d] get %x DEB %x\n", __FUNCTION__, __LINE__, getDebugMetadataVersionFromModule(*Result), DEBUG_METADATA_VERSION);
  return Result;
}

void format_type(DIType DT)
{
    //if (DT.getTag() == dwarf::DW_TAG_subroutine_type)
       //return;
    fprintf(stderr, "    %s:", __FUNCTION__);
    fprintf(stderr, "[ %s ]", dwarf::TagString(DT.getTag()));
    StringRef Res = DT.getName();
    if (!Res.empty())
      errs() << " [" << Res << "]";
    errs() << " [line " << DT.getLineNumber() << ", size " << DT.getSizeInBits()
       << ", align " << DT.getAlignInBits() << ", offset " << DT.getOffsetInBits();
    if (DT.isBasicType())
      if (const char *Enc =
              dwarf::AttributeEncodingString(DIBasicType(DT).getEncoding()))
        errs() << ", enc " << Enc;
    errs() << "]";
    if (DT.isPrivate()) errs() << " [private]";
    else if (DT.isProtected()) errs() << " [protected]";
    if (DT.isArtificial()) errs() << " [artificial]";
    if (DT.isForwardDecl()) errs() << " [decl]";
    else if (DT.getTag() == dwarf::DW_TAG_structure_type ||
             DT.getTag() == dwarf::DW_TAG_union_type ||
             DT.getTag() == dwarf::DW_TAG_enumeration_type ||
             DT.getTag() == dwarf::DW_TAG_class_type)
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
 StringRef Name = CTy.getName();
    DIArray Elements = CTy.getTypeArray();
    for (unsigned i = 0, N = Elements.getNumElements(); i < N; ++i) {
      DIDescriptor Element = Elements.getElement(i);
fprintf(stderr, "struct elt:");
Element.dump();
    }
    }
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
      //printf("[%s:%d]methods\n", __FUNCTION__, __LINE__);
      //SPs.getElement(i)->dump();
      //processSubprogram(DISubprogram(SPs.getElement(i)));
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
      exit(1);
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

  // Add the module's name to the start of the vector of arguments to main().
  std::vector<std::string> InputArgv;
  InputArgv.insert(InputArgv.begin(), InputFile[0]);

  // Call the main function from M as if its signature were:
  //   int main (int argc, char **argv, const char **envp)
  // using the contents of Args to determine argc & argv, and the contents of
  // EnvVars to determine envp.
  //
  Function *EntryFn = Mod->getFunction("main");
  if (!EntryFn) {
    printf("'main' function not found in module.\n");
    return -1;
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
