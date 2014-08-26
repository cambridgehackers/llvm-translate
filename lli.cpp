//===- lli.cpp - LLVM Interpreter / Dynamic compiler ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This utility provides a simple wrapper around the LLVM Execution Engines,
// which allow the direct execution of LLVM programs through a Just-In-Time
// compiler, or through an interpreter if no JIT is available for this platform.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "lli"
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
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/Memory.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation.h"
#include <cerrno>

int dump_ir;// = 1;
int dump_interpret;// = 1;

using namespace llvm;

namespace {
  cl::opt<std::string>
  InputFile(cl::desc("<input bitcode>"), cl::Positional, cl::init("-"));

  cl::list<std::string>
  InputArgv(cl::ConsumeAfter, cl::desc("<program arguments>..."));

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

static ExecutionEngine *EE = 0;
std::map<const Value *, int> slotmap;
struct {
    char *name;
} slotarray[MAX_SLOTARRAY];
static int slotindex;

static uint64_t ***globalThis;
static uint64_t operand_list[MAX_OPERAND_LIST];
static int operand_list_index;

void makeLocalSlot(const Value *V)
{
  memset(&slotarray[slotindex], 0, sizeof(slotarray[slotindex]));
  slotmap.insert(std::pair<const Value *, int>(V, slotindex++));
}
int getLocalSlot(const Value *V)
{
  assert(!isa<Constant>(V) && "Can't get a constant or global slot with this!"); 
  std::map<const Value *, int>::iterator FI = slotmap.find(V);
  //return FI == slotmap.end() ? -1 : (int)FI->second;
  if (FI == slotmap.end()) {
     makeLocalSlot(V);
     return getLocalSlot(V);
  }
  return (int)FI->second;
}

void dump_type(Module *Mod, const char *p)
{
    StructType *tgv = Mod->getTypeByName(p);
    printf("%s:] ", __FUNCTION__);
    tgv->dump();
    printf(" tgv %p\n", tgv);
}

static void WriteConstantInternal(const Constant *CV)
{
  if (const ConstantInt *CI = dyn_cast<ConstantInt>(CV)) {
    operand_list[operand_list_index++] = CI->getZExtValue();
    return;
  }

printf("[%s:%d]\n", __FUNCTION__, __LINE__);
  if (const ConstantFP *CFP = dyn_cast<ConstantFP>(CV)) {
    if (&CFP->getValueAPF().getSemantics() == &APFloat::IEEEsingle ||
        &CFP->getValueAPF().getSemantics() == &APFloat::IEEEdouble) {
      // We would like to output the FP constant value in exponential notation,
      // but we cannot do this if doing so will lose precision.  Check here to
      // make sure that we only output it in exponential format if we can parse
      // the value back and get the same value.
      //
      bool ignored;
      bool isHalf = &CFP->getValueAPF().getSemantics()==&APFloat::IEEEhalf;
      bool isDouble = &CFP->getValueAPF().getSemantics()==&APFloat::IEEEdouble;
      bool isInf = CFP->getValueAPF().isInfinity();
      bool isNaN = CFP->getValueAPF().isNaN();
      if (!isHalf && !isInf && !isNaN) {
        double Val = isDouble ? CFP->getValueAPF().convertToDouble() :
                                CFP->getValueAPF().convertToFloat();
        SmallString<128> StrVal;
        raw_svector_ostream(StrVal) << Val;

        // Check to make sure that the stringized number is not some string like
        // "Inf" or NaN, that atof will accept, but the lexer will not.  Check
        // that the string matches the "[-+]?[0-9]" regex.
        //
        if ((StrVal[0] >= '0' && StrVal[0] <= '9') ||
            ((StrVal[0] == '-' || StrVal[0] == '+') &&
             (StrVal[1] >= '0' && StrVal[1] <= '9'))) {
          // Reparse stringized version!
          if (APFloat(APFloat::IEEEdouble, StrVal).convertToDouble() == Val) {
            printf("%s", StrVal.str().str().c_str());
            return;
          }
        }
      }
      assert(sizeof(double) == sizeof(uint64_t) && "assuming that double is 64 bits!");
      //char Buffer[40];
      APFloat apf = CFP->getValueAPF();
      // Halves and floats are represented in ASCII IR as double, convert.
      if (!isDouble)
        apf.convert(APFloat::IEEEdouble, APFloat::rmNearestTiesToEven, &ignored);
      //printf( "0x%s", utohex_buffer(uint64_t(apf.bitcastToAPInt().getZExtValue()), Buffer+40));
      return;
    }

    // Either half, or some form of long double.
    // These appear as a magic letter identifying the type, then a
    // fixed number of hex digits.
    printf( "0x");
    // Bit position, in the current word, of the next nibble to print.
    int shiftcount;

    if (&CFP->getValueAPF().getSemantics() == &APFloat::x87DoubleExtended) {
      // api needed to prevent premature destruction
      APInt api = CFP->getValueAPF().bitcastToAPInt();
      const uint64_t* p = api.getRawData();
      uint64_t word = p[1];
      shiftcount = 12;
      int width = api.getBitWidth();
      for (int j=0; j<width; j+=4, shiftcount-=4) {
        unsigned int nibble = (word>>shiftcount) & 15;
        if (nibble < 10)
          printf("%c", (unsigned char)(nibble + '0'));
        else
          printf("%c", (unsigned char)(nibble - 10 + 'A'));
        if (shiftcount == 0 && j+4 < width) {
          word = *p;
          shiftcount = 64;
          if (width-j-4 < 64)
            shiftcount = width-j-4;
        }
      }
      return;
    } else if (&CFP->getValueAPF().getSemantics() == &APFloat::IEEEquad) {
      shiftcount = 60;
    } else if (&CFP->getValueAPF().getSemantics() == &APFloat::PPCDoubleDouble) {
      shiftcount = 60;
    } else if (&CFP->getValueAPF().getSemantics() == &APFloat::IEEEhalf) {
      shiftcount = 12;
    } else
      llvm_unreachable("Unsupported floating point type");
    // api needed to prevent premature destruction
    APInt api = CFP->getValueAPF().bitcastToAPInt();
    const uint64_t* p = api.getRawData();
    uint64_t word = *p;
    int width = api.getBitWidth();
    for (int j=0; j<width; j+=4, shiftcount-=4) {
      unsigned int nibble = (word>>shiftcount) & 15;
      if (nibble < 10)
        printf("%c", (unsigned char)(nibble + '0'));
      else
        printf("%c", (unsigned char)(nibble - 10 + 'A'));
      if (shiftcount == 0 && j+4 < width) {
        word = *(++p);
        shiftcount = 64;
        if (width-j-4 < 64)
          shiftcount = width-j-4;
      }
    }
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
  if (!Operand) {
    return;
  }
//printf("[%s:%d] id %d \n", __FUNCTION__, __LINE__, Operand->getValueID());
  if (Operand->hasName()) {
    const char *cp = Operand->getName().str().c_str();
    if (!strcmp(cp, "this"))
        operand_list[operand_list_index++] = (uint64_t)globalThis;
    else
        printf("[%s:%d]name %s\n", __FUNCTION__, __LINE__, cp);
    return;
  }
  const Constant *CV = dyn_cast<Constant>(Operand);
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

#if 0
    if (!Machine) {
      if (N->isFunctionLocal())
        Machine = new SlotTracker(N->getFunction());
      else
        Machine = new SlotTracker(Context);
    }
    int Slot = Machine->getMetadataSlot(N);
    if (Slot == -1)
      printf( "<badref>");
    else
      printf( '!' << Slot);
#endif
    return;
  }

  if (const MDString *MDS = dyn_cast<MDString>(Operand)) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    //PrintEscapedString(MDS->getString());
    return;
  }

  if (Operand->getValueID() == Value::PseudoSourceValueVal ||
      Operand->getValueID() == Value::FixedStackPseudoSourceValueVal) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    return;
  }

  char Prefix = '%';
  int Slot = getLocalSlot(Operand);
printf("[%s:%d] slot %d\n", __FUNCTION__, __LINE__, Slot);
#if 0
  // If we have a SlotTracker, use it.
  if (Machine) {
    if (const GlobalValue *GV = dyn_cast<GlobalValue>(Operand)) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      Slot = Machine->getGlobalSlot(GV);
      Prefix = '@';
    } else {
//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      Slot = Machine->getLocalSlot(Operand);

      // If the local value didn't succeed, then we may be referring to a value
      // from a different function.  Translate it, as this can happen when using
      // address of blocks.
      if (Slot == -1)
        if ((Machine = createSlotTracker(Operand))) {
          Slot = Machine->getLocalSlot(Operand);
          delete Machine;
        }
    }
  } else if ((Machine = createSlotTracker(Operand))) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    // Otherwise, create one to get the # and then destroy it.
    if (const GlobalValue *GV = dyn_cast<GlobalValue>(Operand)) {
      Slot = Machine->getGlobalSlot(GV);
      Prefix = '@';
    } else {
      Slot = Machine->getLocalSlot(Operand);
    }
    delete Machine;
    Machine = 0;
  } else {
    Slot = -1;
  }

  if (Slot != -1)
    printf( Prefix << Slot);
  else
    printf( "<badref>");
#endif
}

// This member is called for each Instruction in a function..
void printInstruction(const Instruction &I) 
{
  
  operand_list_index = 0;
  memset(operand_list, 0, sizeof(operand_list));

  if (I.hasName()) {
    printf("%10s: ", I.getName().str().c_str());
  } else if (!I.getType()->isVoidTy()) {
    // Print out the def slot taken.
    int t = getLocalSlot(&I);
#if 0
    if (t == -1) {
      makeLocalSlot(&I);
      t = getLocalSlot(&I);
    }
#endif
    printf("%10d: ",  t);
//search->second);
  }
  else
    printf("          : ");
  if (isa<CallInst>(I) && cast<CallInst>(I).isTailCall())
    printf("tail ");
  // If this is an atomic load or store, print out the atomic marker.
  if ((isa<LoadInst>(I)  && cast<LoadInst>(I).isAtomic()) ||
      (isa<StoreInst>(I) && cast<StoreInst>(I).isAtomic()))
    printf(" atomic");
  // If this is a volatile operation, print out the volatile marker.
  if ((isa<LoadInst>(I)  && cast<LoadInst>(I).isVolatile()) ||
      (isa<StoreInst>(I) && cast<StoreInst>(I).isVolatile()) ||
      (isa<AtomicCmpXchgInst>(I) && cast<AtomicCmpXchgInst>(I).isVolatile()) ||
      (isa<AtomicRMWInst>(I) && cast<AtomicRMWInst>(I).isVolatile()))
    printf(" volatile");
  // Print out optimization information.
  //WriteOptimizationInfo(&I);
  // Print out the compare instruction predicates
  //if (const CmpInst *CI = dyn_cast<CmpInst>(&I))
    //printf(' ' << getPredicateText(CI->getPredicate()));
  // Print out the atomicrmw operation
  //if (const AtomicRMWInst *RMWI = dyn_cast<AtomicRMWInst>(&I))
    //writeAtomicRMWOperation(RMWI->getOperation());
  // Print out the type of the operands...
  const Value *Operand = I.getNumOperands() ? I.getOperand(0) : 0;
  // Special case conditional branches to swizzle the condition out to the front
  if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
    const BranchInst &BI(cast<BranchInst>(I));
    writeOperand(BI.getCondition());
    writeOperand(BI.getSuccessor(0));
    writeOperand(BI.getSuccessor(1));
  } else if (isa<SwitchInst>(I)) {
    const SwitchInst& SI(cast<SwitchInst>(I));
    // Special case switch instruction to get formatting nice and correct.
    writeOperand(SI.getCondition());
    writeOperand(SI.getDefaultDest());
    printf(" [");
    for (SwitchInst::ConstCaseIt i = SI.case_begin(), e = SI.case_end(); i != e; ++i) {
      writeOperand(i.getCaseValue());
      writeOperand(i.getCaseSuccessor());
    }
  } else if (isa<IndirectBrInst>(I)) {
    // Special case indirectbr instruction to get formatting nice and correct.
    writeOperand(Operand);
    for (unsigned i = 1, e = I.getNumOperands(); i != e; ++i) {
      writeOperand(I.getOperand(i));
    }
  } else if (const PHINode *PN = dyn_cast<PHINode>(&I)) {
    //TypePrinter.print(I.getType());
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
    //TypePrinter.print(I.getType());
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
  } else if (isa<ReturnInst>(I) && !Operand) {
    printf(" void");
  } else if (const CallInst *CI = dyn_cast<CallInst>(&I)) {
    // Print the calling convention being used.
    if (CI->getCallingConv() != CallingConv::C) {
      printf(" ");
      //PrintCallingConv(CI->getCallingConv());
    }
    Operand = CI->getCalledValue();
    PointerType *PTy = cast<PointerType>(Operand->getType());
    FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
    Type *RetTy = FTy->getReturnType();
    //const AttributeSet &PAL = CI->getAttributes();
    //if (PAL.hasAttributes(AttributeSet::ReturnIndex))
      //printf(' ' << PAL.getAsString(AttributeSet::ReturnIndex));
    // If possible, print out the short form of the call instruction.  We can
    // only do this if the first argument is a pointer to a nonvararg function,
    // and if the return type is not a pointer to a function.
    //
    if (!FTy->isVarArg() && (!RetTy->isPointerTy() || !cast<PointerType>(RetTy)->getElementType()->isFunctionTy())) {
      //TypePrinter.print(RetTy);
printf("[%s:%d] shortformcall dumpreturntype\n", __FUNCTION__, __LINE__);
      RetTy->dump();
    }
    writeOperand(Operand);
    for (unsigned op = 0, Eop = CI->getNumArgOperands(); op < Eop; ++op) {
      ///writeParamOperand(CI->getArgOperand(op), PAL, op + 1);
    }
    //if (PAL.hasAttributes(AttributeSet::FunctionIndex))
      //printf(" #" < < Machine.getAttributeGroupSlot(PAL.getFnAttributes()));
  } else if (const InvokeInst *II = dyn_cast<InvokeInst>(&I)) {
    Operand = II->getCalledValue();
    PointerType *PTy = cast<PointerType>(Operand->getType());
    FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
    Type *RetTy = FTy->getReturnType();
    //const AttributeSet &PAL = II->getAttributes();
    // Print the calling convention being used.
    if (II->getCallingConv() != CallingConv::C) {
      //PrintCallingConv(II->getCallingConv());
    }
    //if (PAL.hasAttributes(AttributeSet::ReturnIndex))
      //printf(' ' < < PAL.getAsString(AttributeSet::ReturnIndex));
    // If possible, print out the short form of the invoke instruction. We can
    // only do this if the first argument is a pointer to a nonvararg function,
    // and if the return type is not a pointer to a function.
    //
    if (!FTy->isVarArg() &&
        (!RetTy->isPointerTy() ||
         !cast<PointerType>(RetTy)->getElementType()->isFunctionTy())) {
      //TypePrinter.print(RetTy);
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      RetTy->dump();
    }
    writeOperand(Operand);
    for (unsigned op = 0, Eop = II->getNumArgOperands(); op < Eop; ++op) {
      //writeParamOperand(II->getArgOperand(op), PAL, op + 1);
    }
    //if (PAL.hasAttributes(AttributeSet::FunctionIndex))
      //printf(" #" < < Machine.getAttributeGroupSlot(PAL.getFnAttributes()));
    writeOperand(II->getNormalDest());
    writeOperand(II->getUnwindDest());
  } else if (const AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
    //TypePrinter.print(AI->getAllocatedType());
    if (!AI->getArraySize() || AI->isArrayAllocation()) {
      writeOperand(AI->getArraySize());
    }
    //if (AI->getAlignment()) {
      //printf(", align " << AI->getAlignment());
    //}
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
  // Print atomic ordering/alignment for memory operations
  //if (const LoadInst *LI = dyn_cast<LoadInst>(&I)) {
    //if (LI->isAtomic())
      //writeAtomic(LI->getOrdering(), LI->getSynchScope());
    //if (LI->getAlignment())
      //printf(", align " << LI->getAlignment());
  //} else if (const StoreInst *SI = dyn_cast<StoreInst>(&I)) {
    //if (SI->isAtomic())
      //writeAtomic(SI->getOrdering(), SI->getSynchScope());
    //if (SI->getAlignment())
      //printf(", align " << SI->getAlignment());
  //} else if (const AtomicCmpXchgInst *CXI = dyn_cast<AtomicCmpXchgInst>(&I)) {
    //writeAtomic(CXI->getOrdering(), CXI->getSynchScope());
  //} else if (const AtomicRMWInst *RMWI = dyn_cast<AtomicRMWInst>(&I)) {
    //writeAtomic(RMWI->getOrdering(), RMWI->getSynchScope());
  //} else if (const FenceInst *FI = dyn_cast<FenceInst>(&I)) {
    //writeAtomic(FI->getOrdering(), FI->getSynchScope());
  //}
  // Print Metadata info.
  SmallVector<std::pair<unsigned, MDNode*>, 4> InstMD;
  I.getAllMetadata(InstMD);
  if (!InstMD.empty()) {
    SmallVector<StringRef, 8> MDNames;
    I.getType()->getContext().getMDKindNames(MDNames);
    for (unsigned i = 0, e = InstMD.size(); i != e; ++i) {
      //unsigned Kind = InstMD[i].first;
       //if (Kind < MDNames.size()) {
         //printf(", !" << MDNames[Kind]);
       //} else {
         //printf(", !<unknown kind #" << Kind << ">");
       //}
      //WriteAsOperandInternal(InstMD[i].second, &TypePrinter, &Machine, TheModule);
    }
  }
  int opcode = I.getOpcode();
  printf("    ");
  switch (I.getOpcode()) {
  // Terminators
  case Instruction::Ret:
      printf("XLAT: Ret");
      break;
  //case Instruction::Br:
  //case Instruction::Switch:
  //case Instruction::IndirectBr:
  //case Instruction::Invoke:
  //case Instruction::Resume:
  case Instruction::Unreachable:
      printf("XLAT: Unreachable");
      break;

  // Standard binary operators...
  case Instruction::Add:
      printf("XLAT: Add");
      break;
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

  // Memory instructions...
  //case Instruction::Alloca:
  case Instruction::Load:
      printf("XLAT: Load");
      break;
  case Instruction::Store:
      printf("XLAT: Store");
      break;
  //case Instruction::AtomicCmpXchg:
  //case Instruction::AtomicRMW:
  //case Instruction::Fence:
  case Instruction::GetElementPtr:
      {
      printf("XLAT: GetElementPtr");
      if (operand_list_index >= 3 && operand_list[0]) {
          uint64_t *val = *(uint64_t **)(operand_list[0] + 8 * (1+operand_list[2] + operand_list[3]));
          const GlobalValue *g = EE->getGlobalValueAtAddress(val);
          //printf(" g=%p", g);
          if (g)
              printf(" g='%s'", g->getName().str().c_str());
          //if (g) g->dump();
      }
      }
      break;

  // Convert instructions...
  //case Instruction::Trunc:
  //case Instruction::ZExt:
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
      printf("XLAT: BitCast");
      break;
  //case Instruction::AddrSpaceCast:

  // Other instructions...
  case Instruction::ICmp:
      printf("XLAT: ICmp");
      break;
  //case Instruction::FCmp:
  //case Instruction::PHI:
  //case Instruction::Select:
  case Instruction::Call:
      printf("XLAT: Call");
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
      printf(" op[%d]=%llx;", i, (long long)operand_list[i]);
  }
  printf("\n");
}

void printBasicBlock(const BasicBlock *BB) 
{
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
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
  if (F->isDeclaration())
    printf("declare ");
  else
    printf("define ");
  //PrintLinkage(F->getLinkage());
  //PrintVisibility(F->getVisibility());
  // Print the calling convention.
  if (F->getCallingConv() != CallingConv::C) {
    //PrintCallingConv(F->getCallingConv());
  }
  FunctionType *FT = F->getFunctionType();
  //if (Attrs.hasAttributes(AttributeSet::ReturnIndex))
    //printf( Attrs.getAsString(AttributeSet::ReturnIndex) << ' ');
  //TypePrinter.print(F->getReturnType());
printf("[%s:%d] dumptype\n", __FUNCTION__, __LINE__);
      F->getReturnType()->dump();
  //WriteAsOperandInternal(F, &TypePrinter, &Machine, F->getParent());
  //Machine.incorporateFunction(F);
  // Loop over the arguments, printing them...
  unsigned Idx = 1;
  if (!F->isDeclaration()) {
    // If this isn't a declaration, print the argument names as well.
    for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
      //TypePrinter.print(I->getType());
      printf("[%s:%d] dumptype\n", __FUNCTION__, __LINE__);
      I->getType()->dump();
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
printf("[%s:%d] dumptype\n", __FUNCTION__, __LINE__);
      FT->getParamType(i)->dump();
      //if (Attrs.hasAttributes(i+1))
        //printf(' ' << Attrs.getAsString(i+1));
    }
  }
  // Finish printing arguments...
  if (FT->isVarArg()) {
  }
  if (F->hasUnnamedAddr())
    printf(" unnamed_addr");
  //if (Attrs.hasAttributes(AttributeSet::FunctionIndex))
    //printf(" #" << Machine.getAttributeGroupSlot(Attrs.getFnAttributes()));
  if (F->hasSection()) {
    //PrintEscapedString(F->getSection());
  }
  if (F->hasPrefixData()) {
    printf(" prefix ");
    writeOperand(F->getPrefixData());
  }
  if (F->isDeclaration()) {
  } else {
    for (Function::const_iterator I = F->begin(), E = F->end(); I != E; ++I)
      printBasicBlock(I);
  }

  slotmap.clear();
  slotindex = 0;
}

void dump_vtab(uint64_t ***thisptr)
{
int arr_size = 0;
    globalThis = thisptr;
    uint64_t **vtab = (uint64_t **)thisptr[0];
    const GlobalValue *g = EE->getGlobalValueAtAddress(vtab-2);
    printf("[%s:%d] vtabbase %p g %p:\n", __FUNCTION__, __LINE__, vtab-2, g);
    if (g) {
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
       const char *cp = f->getName().str().c_str();
       const char *cend = cp + (strlen(cp)-4);
       printf("[%s:%d] [%d] p %p: %s\n", __FUNCTION__, __LINE__, i, vtab[i], cp);
       if (strcmp(cend, "D0Ev") && strcmp(cend, "D1Ev")) {
           if (!strcmp(cend, "rdEv")) {
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
#if 0
  const Function *guard = Mod->getFunction("_ZN5Count5count5guardEv"); //Count::done::guard");
  printf("[%s:%d] guard %p\n", __FUNCTION__, __LINE__, guard);
  if (guard) {
     processFunction(guard);
     printf("FULL:\n");
     guard->dump();
  }
#endif
}

int main(int argc, char **argv, char * const *envp)
{
  SMDiagnostic Err;
  std::string ErrorMsg;
  int Result;

printf("[%s:%d] start\n", __FUNCTION__, __LINE__);
  if (dump_interpret)
      DebugFlag = true;
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  LLVMContext &Context = getGlobalContext();

  cl::ParseCommandLineOptions(argc, argv, "llvm interpreter & dynamic compiler\n");

  // Load the bitcode...
  Module *Mod = ParseIRFile(InputFile, Err, Context);
  if (!Mod) {
    Err.print(argv[0], errs());
    return 1;
  }

  // If not jitting lazily, load the whole bitcode file eagerly too.
    if (Mod->MaterializeAllPermanently(&ErrorMsg)) {
      errs() << argv[0] << ": bitcode didn't read correctly.\n";
      errs() << "Reason: " << ErrorMsg << "\n";
      exit(1);
    }

  if (dump_ir) {
    ModulePass *DebugIRPass = createDebugIRPass();
    DebugIRPass->runOnModule(*Mod);
  }

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
      errs() << argv[0] << ": error creating EE: " << ErrorMsg << "\n";
    else
      errs() << argv[0] << ": unknown error creating EE!\n";
    exit(1);
  }

  // Load any additional modules specified on the command line.
  for (unsigned i = 0, e = ExtraModules.size(); i != e; ++i) {
    Module *XMod = ParseIRFile(ExtraModules[i], Err, Context);
    if (!XMod) {
      Err.print(argv[0], errs());
      return 1;
    }
    EE->addModule(XMod);
  }
  EE->DisableLazyCompilation(true);

  // Add the module's name to the start of the vector of arguments to main().
  InputArgv.insert(InputArgv.begin(), InputFile);

  // Call the main function from M as if its signature were:
  //   int main (int argc, char **argv, const char **envp)
  // using the contents of Args to determine argc & argv, and the contents of
  // EnvVars to determine envp.
  //
  Function *EntryFn = Mod->getFunction("main");
  if (!EntryFn) {
    errs() << "\'main\' function not found in module.\n";
    return -1;
  }
  // Run static constructors.
  EE->runStaticConstructorsDestructors(false);

  for (Module::iterator I = Mod->begin(), E = Mod->end(); I != E; ++I) {
        Function *Fn = &*I;
        if (Fn != EntryFn && !Fn->isDeclaration())
          EE->getPointerToFunction(Fn);
  }

  // Trigger compilation separately so code regions that need to be 
  // invalidated will be known.
  (void)EE->getPointerToFunction(EntryFn);

  // Run main.
  Result = EE->runFunctionAsMain(EntryFn, InputArgv, envp);

  generate_verilog(Mod);

printf("[%s:%d] end\n", __FUNCTION__, __LINE__);
  return Result;
}
