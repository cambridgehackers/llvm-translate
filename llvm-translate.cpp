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
//bb
#include "llvm/Pass.h"
#include "llvm/Transforms/Vectorize.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Transforms/Utils/Local.h"

using namespace llvm;

static int dump_interpret;// = 1;
static int output_stdout = 1;
static const char *trace_break_name;// = "_ZN4Echo7respond5guardEv";

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
std::map<const Value *, int> slotmap;
struct {
    const char *name;
    int ignore_debug_info;
    uint64_t ***value;
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

#if 1
#undef DEBUG
#define DEBUG(A) A
//STATISTIC(NumFusedOps, "Number of operations fused by bb-vectorize");
static BasicBlockPass *optimize_block_item;

struct BBVectorize : public BasicBlockPass {
  static char ID; // Pass identification, replacement for typeid 
  const VectorizeConfig Config; 
  BBVectorize(const VectorizeConfig &C = VectorizeConfig()) : BasicBlockPass(ID), Config(C) {
    initializeBBVectorizePass(*PassRegistry::getPassRegistry());
  } 
  BBVectorize(Pass *P, const VectorizeConfig &C) : BasicBlockPass(ID), Config(C) {
    //AA = &P->getAnalysis<AliasAnalysis>(); //DT = &P->getAnalysis<DominatorTree>();
    //SE = &P->getAnalysis<ScalarEvolution>(); //TD = P->getAnalysisIfAvailable<DataLayout>();
    //TTI = IgnoreTargetInfo ? 0 : &P->getAnalysis<TargetTransformInfo>();
  } 
  typedef std::pair<Value *, Value *> ValuePair;
  typedef std::pair<ValuePair, int> ValuePairWithCost;
  typedef std::pair<ValuePair, size_t> ValuePairWithDepth;
  typedef std::pair<ValuePair, ValuePair> VPPair; // A ValuePair pair
  typedef std::pair<VPPair, unsigned> VPPairWithType; 
  //AliasAnalysis *AA; //DominatorTree *DT; //DataLayout *TD;
  ScalarEvolution *SE;
  const TargetTransformInfo *TTI; 
  // FIXME: const correct?  
  bool vectorizePairs(BasicBlock &BB); 
  bool getCandidatePairs(BasicBlock &BB, BasicBlock::iterator &Start,
                     DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
                     DenseSet<ValuePair> &FixedOrderPairs, DenseMap<ValuePair, int> &CandidatePairCostSavings,
                     std::vector<Value *> &PairableInsts); 
  // FIXME: The current implementation does not account for pairs that
  // are connected in multiple ways. For example:
  //   C1 = A1 / A2; C2 = A2 / A1 (which may be both direct and a swap)
  enum PairConnectionType { PairConnectionDirect, PairConnectionSwap, PairConnectionSplat }; 
  void computeConnectedPairs( DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
           DenseSet<ValuePair> &CandidatePairsSet, std::vector<Value *> &PairableInsts,
           DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs, DenseMap<VPPair, unsigned> &PairConnectionTypes);

  void buildDepMap(BasicBlock &BB, DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
           std::vector<Value *> &PairableInsts, DenseSet<ValuePair> &PairableInstUsers); 
  void choosePairs(DenseMap<Value *, std::vector<Value *> > &CandidatePairs, DenseSet<ValuePair> &CandidatePairsSet,
           DenseMap<ValuePair, int> &CandidatePairCostSavings, std::vector<Value *> &PairableInsts,
           DenseSet<ValuePair> &FixedOrderPairs, DenseMap<VPPair, unsigned> &PairConnectionTypes,
           DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs,
           DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairDeps,
           DenseSet<ValuePair> &PairableInstUsers, DenseMap<Value *, Value *>& ChosenPairs);

  void fuseChosenPairs(BasicBlock &BB, std::vector<Value *> &PairableInsts,
           DenseMap<Value *, Value *>& ChosenPairs, DenseSet<ValuePair> &FixedOrderPairs,
           DenseMap<VPPair, unsigned> &PairConnectionTypes,
           DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs,
           DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairDeps); 
  bool areInstsCompatible(Instruction *I, Instruction *J, int &CostSavings); 
  bool trackUsesOfI(DenseSet<Value *> &Users, AliasSetTracker &WriteSet, Instruction *I,
                    Instruction *J, bool UpdateUsers = true, DenseSet<ValuePair> *LoadMoveSetPairs = 0); 
void computePairsConnectedTo( DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
           DenseSet<ValuePair> &CandidatePairsSet, std::vector<Value *> &PairableInsts,
           DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs, DenseMap<VPPair, unsigned> &PairConnectionTypes, ValuePair P);

  void pruneDAGFor( DenseMap<Value *, std::vector<Value *> > &CandidatePairs, std::vector<Value *> &PairableInsts,
           DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs, DenseSet<ValuePair> &PairableInstUsers,
           DenseMap<Value *, Value *> &ChosenPairs, DenseMap<ValuePair, size_t> &DAG,
           DenseSet<ValuePair> &PrunedDAG, ValuePair J); 
  void buildInitialDAGFor( DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
           DenseSet<ValuePair> &CandidatePairsSet, std::vector<Value *> &PairableInsts,
           DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs, DenseSet<ValuePair> &PairableInstUsers,
           DenseMap<Value *, Value *> &ChosenPairs, DenseMap<ValuePair, size_t> &DAG, ValuePair J); 
  void findBestDAGFor( DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
           DenseSet<ValuePair> &CandidatePairsSet, DenseMap<ValuePair, int> &CandidatePairCostSavings,
           std::vector<Value *> &PairableInsts, DenseSet<ValuePair> &FixedOrderPairs,
           DenseMap<VPPair, unsigned> &PairConnectionTypes, DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs,
           DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairDeps, DenseSet<ValuePair> &PairableInstUsers,
           DenseMap<Value *, Value *> &ChosenPairs, DenseSet<ValuePair> &BestDAG, size_t &BestMaxDepth,
           int &BestEffSize, Value *II, std::vector<Value *>&JJ); 
  Value *getReplacementPointerInput(LLVMContext& Context, Instruction *I, Instruction *J, unsigned o); 
  void fillNewShuffleMask(LLVMContext& Context, Instruction *J, unsigned MaskOffset, unsigned NumInElem,
                   unsigned NumInElem1, unsigned IdxOffset, std::vector<Constant*> &Mask); 
  Value *getReplacementShuffleMask(LLVMContext& Context, Instruction *I, Instruction *J); 
  bool expandIEChain(LLVMContext& Context, Instruction *I, Instruction *J, unsigned o, Value *&LOp, unsigned numElemL,
                     Type *ArgTypeL, Type *ArgTypeR, bool IBeforeJ, unsigned IdxOff = 0); 
  Value *getReplacementInput(LLVMContext& Context, Instruction *I, Instruction *J, unsigned o, bool IBeforeJ); 
  void getReplacementInputsForPair(LLVMContext& Context, Instruction *I,
                   Instruction *J, SmallVectorImpl<Value *> &ReplacedOperands, bool IBeforeJ); 
  void replaceOutputsOfPair(LLVMContext& Context, Instruction *I, Instruction *J, Instruction *K,
                   Instruction *&InsertionPt, Instruction *&K1, Instruction *&K2); 
  void collectPairLoadMoveSet(BasicBlock &BB, DenseMap<Value *, Value *> &ChosenPairs,
                   DenseMap<Value *, std::vector<Value *> > &LoadMoveSet,
                   DenseSet<ValuePair> &LoadMoveSetPairs, Instruction *I); 
  void collectLoadMoveSet(BasicBlock &BB, std::vector<Value *> &PairableInsts,
                   DenseMap<Value *, Value *> &ChosenPairs, DenseMap<Value *, std::vector<Value *> > &LoadMoveSet,
                   DenseSet<ValuePair> &LoadMoveSetPairs); 
  void moveUsesOfIAfterJ(BasicBlock &BB, Instruction *&InsertionPt, Instruction *I, Instruction *J); 
  void combineMetadata(Instruction *K, const Instruction *J); 
  virtual bool runOnBasicBlock(BasicBlock &BB)
  {
printf("[%s:%d] BEGIN\n", __FUNCTION__, __LINE__);
    bool changed = false;
    // Iterate a sufficient number of times to merge types of size 1 bit,
    // then 2 bits, then 4, etc. up to half of the target vector width of the // target vector register.
    unsigned n = 1;
    //for (unsigned v = 2; (TTI || v <= Config.VectorBits) && (!Config.MaxIter || n <= Config.MaxIter); v *= 2, ++n) {
      //DEBUG(dbgs() << "BBV: fusing loop #" << n << " for " << BB.getName() << " in " << BB.getParent()->getName() << "...\n");
      if (vectorizePairs(BB))
        changed = true;
      //else
        //break;
    //} 
    if (changed ) {
      ++n;
      for (; !Config.MaxIter || n <= Config.MaxIter; ++n) {
        DEBUG(dbgs() << "BBV: fusing for non-2^n-length vectors loop #: " << n << " for " << BB.getName() << " in " << BB.getParent()->getName() << "...\n");
        if (!vectorizePairs(BB)) break;
      }
    } 
printf("[%s:%d] END %d\n", __FUNCTION__, __LINE__, changed);
    return changed;
  } 
  virtual void getAnalysisUsage(AnalysisUsage &AU) const
  {
    BasicBlockPass::getAnalysisUsage(AU);
    AU.setPreservesCFG();
  }

  static inline VectorType *getVecTypeForPair(Type *ElemTy, Type *Elem2Ty)
  {
    assert(ElemTy->getScalarType() == Elem2Ty->getScalarType() && "Cannot form vector from incompatible scalar types");
    Type *STy = ElemTy->getScalarType(); 
    unsigned numElem;
    if (VectorType *VTy = dyn_cast<VectorType>(ElemTy)) {
      numElem = VTy->getNumElements();
    } else {
      numElem = 1;
    } 
    if (VectorType *VTy = dyn_cast<VectorType>(Elem2Ty)) {
      numElem += VTy->getNumElements();
    } else {
      numElem += 1;
    } 
    return VectorType::get(STy, numElem);
  }

  static inline void getInstructionTypes(Instruction *I, Type *&T1, Type *&T2)
  {
    if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
      // For stores, it is the value type, not the pointer type that matters
      // because the value is what will come from a vector register.  
      Value *IVal = SI->getValueOperand();
      T1 = IVal->getType();
    } else {
      T1 = I->getType();
    } 
    if (CastInst *CI = dyn_cast<CastInst>(I))
      T2 = CI->getSrcTy();
    else
      T2 = T1; 
    if (SelectInst *SI = dyn_cast<SelectInst>(I)) {
      T2 = SI->getCondition()->getType();
    } else if (ShuffleVectorInst *SI = dyn_cast<ShuffleVectorInst>(I)) {
      T2 = SI->getOperand(0)->getType();
    } else if (CmpInst *CI = dyn_cast<CmpInst>(I)) {
      T2 = CI->getOperand(0)->getType();
    }
  }

  // Returns the weight associated with the provided value. A chain of
  // candidate pairs has a length given by the sum of the weights of its
  // members (one weight per pair; the weight of each member of the pair
  // is assumed to be the same). This length is then compared to the
  // chain-length threshold to determine if a given chain is significant
  // enough to be vectorized. The length is also used in comparing
  // candidate chains where longer chains are considered to be better.
  // Note: when this function returns 0, the resulting instructions are
  // not actually fused.
  inline size_t getDepthFactor(Value *V) {
    // InsertElement and ExtractElement have a depth factor of zero. This is
    // for two reasons: First, they cannot be usefully fused. Second, because
    // the pass generates a lot of these, they can confuse the simple metric
    // used to compare the dags in the next iteration. Thus, giving them a
    // weight of zero allows the pass to essentially ignore them in
    // subsequent iterations when looking for vectorization opportunities
    // while still tracking dependency chains that flow through those
    // instructions.
    if (isa<InsertElementInst>(V) || isa<ExtractElementInst>(V))
      return 0; 
    // Give a load or store half of the required depth so that load/store
    // pairs will vectorize.
    if (isa<LoadInst>(V) || isa<StoreInst>(V))
      return Config.ReqChainDepth/2; 
    return 1;
  }

  // Returns the cost of the provided instruction using TTI.
  // This does not handle loads and stores.
  unsigned getInstrCost(unsigned Opcode, Type *T1, Type *T2) {
    switch (Opcode) {
    default: break;
    case Instruction::GetElementPtr:
      // We mark this instruction as zero-cost because scalar GEPs are usually
      // lowered to the instruction addressing mode. At the moment we don't // generate vector GEPs.
      return 0;
    case Instruction::Br:
      return TTI->getCFInstrCost(Opcode);
    case Instruction::PHI:
      return 0;
    case Instruction::Add: case Instruction::FAdd: case Instruction::Sub: case Instruction::FSub:
    case Instruction::Mul: case Instruction::FMul: case Instruction::UDiv: case Instruction::SDiv:
    case Instruction::FDiv: case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
    case Instruction::Shl: case Instruction::LShr: case Instruction::AShr: case Instruction::And:
    case Instruction::Or: case Instruction::Xor:
      return TTI->getArithmeticInstrCost(Opcode, T1);
    case Instruction::Select: case Instruction::ICmp: case Instruction::FCmp:
      return TTI->getCmpSelInstrCost(Opcode, T1, T2);
    case Instruction::ZExt: case Instruction::SExt: case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::FPExt: case Instruction::PtrToInt: case Instruction::IntToPtr:
    case Instruction::SIToFP: case Instruction::UIToFP: case Instruction::Trunc:
    case Instruction::FPTrunc: case Instruction::BitCast: case Instruction::ShuffleVector:
      return TTI->getCastInstrCost(Opcode, T1, T2);
    } 
    return 1;
  }

  // This determines the relative offset of two loads or stores, returning
  // true if the offset could be determined to be some constant value.
  // For example, if OffsetInElmts == 1, then J accesses the memory directly
  // after I; if OffsetInElmts == -1 then I accesses the memory // directly after J.
  bool getPairPtrInfo(Instruction *I, Instruction *J,
      Value *&IPtr, Value *&JPtr, unsigned &IAlignment, unsigned &JAlignment, unsigned &IAddressSpace, unsigned &JAddressSpace,
      int64_t &OffsetInElmts, bool ComputeOffset = true) {
    OffsetInElmts = 0;
    if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
      LoadInst *LJ = cast<LoadInst>(J);
      IPtr = LI->getPointerOperand();
      JPtr = LJ->getPointerOperand();
      IAlignment = LI->getAlignment();
      JAlignment = LJ->getAlignment();
      IAddressSpace = LI->getPointerAddressSpace();
      JAddressSpace = LJ->getPointerAddressSpace();
    } else {
      StoreInst *SI = cast<StoreInst>(I), *SJ = cast<StoreInst>(J);
      IPtr = SI->getPointerOperand();
      JPtr = SJ->getPointerOperand();
      IAlignment = SI->getAlignment();
      JAlignment = SJ->getAlignment();
      IAddressSpace = SI->getPointerAddressSpace();
      JAddressSpace = SJ->getPointerAddressSpace();
    } 
    if (!ComputeOffset)
      return true; 
    return false;
  } 
  // Returns true if the provided CallInst represents an intrinsic that can // be vectorized.
  bool isVectorizableIntrinsic(CallInst* I) {
    Function *F = I->getCalledFunction();
    if (!F) return false; 
    Intrinsic::ID IID = (Intrinsic::ID) F->getIntrinsicID();
    if (!IID) return false; 
    switch(IID) {
    default:
      return false;
    case Intrinsic::sqrt: case Intrinsic::powi: case Intrinsic::sin: case Intrinsic::cos:
    case Intrinsic::log: case Intrinsic::log2: case Intrinsic::log10: case Intrinsic::exp:
    case Intrinsic::exp2: case Intrinsic::pow:
      return Config.VectorizeMath;
    case Intrinsic::fma: case Intrinsic::fmuladd:
      return Config.VectorizeFMA;
    }
  } 
  bool isPureIEChain(InsertElementInst *IE) {
    InsertElementInst *IENext = IE;
    do {
      if (!isa<UndefValue>(IENext->getOperand(0)) && !isa<InsertElementInst>(IENext->getOperand(0))) {
        return false;
      }
    } while ((IENext = dyn_cast<InsertElementInst>(IENext->getOperand(0)))); 
    return true;
  }
};

// implements one vectorization iteration on the provided // basic block. It returns true if the block is changed.
bool BBVectorize::vectorizePairs(BasicBlock &BB)
{
  bool ShouldContinue;
  BasicBlock::iterator Start = BB.getFirstInsertionPt(); 
  std::vector<Value *> AllPairableInsts;
  DenseMap<Value *, Value *> AllChosenPairs;
  DenseSet<ValuePair> AllFixedOrderPairs;
  DenseMap<VPPair, unsigned> AllPairConnectionTypes;
  DenseMap<ValuePair, std::vector<ValuePair> > AllConnectedPairs, AllConnectedPairDeps; 
  do {
    std::vector<Value *> PairableInsts;
    DenseMap<Value *, std::vector<Value *> > CandidatePairs;
    DenseSet<ValuePair> FixedOrderPairs;
    DenseMap<ValuePair, int> CandidatePairCostSavings;
    ShouldContinue = getCandidatePairs(BB, Start, CandidatePairs, FixedOrderPairs, CandidatePairCostSavings, PairableInsts);
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    if (PairableInsts.empty()) continue;

    // Build the candidate pair set for faster lookups.
    DenseSet<ValuePair> CandidatePairsSet;
    for (DenseMap<Value *, std::vector<Value *> >::iterator I = CandidatePairs.begin(), E = CandidatePairs.end(); I != E; ++I)
      for (std::vector<Value *>::iterator J = I->second.begin(), JE = I->second.end(); J != JE; ++J)
        CandidatePairsSet.insert(ValuePair(I->first, *J));

    // Now we have a map of all of the pairable instructions and we need to
    // select the best possible pairing. A good pairing is one such that the
    // users of the pair are also paired. This defines a (directed) forest
    // over the pairs such that two pairs are connected iff the second pair
    // uses the first.

    // Note that it only matters that both members of the second pair use some
    // element of the first pair (to allow for splatting).

    DenseMap<ValuePair, std::vector<ValuePair> > ConnectedPairs, ConnectedPairDeps;
    DenseMap<VPPair, unsigned> PairConnectionTypes;
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    computeConnectedPairs(CandidatePairs, CandidatePairsSet, PairableInsts, ConnectedPairs, PairConnectionTypes);
    if (ConnectedPairs.empty()) continue;

    for (DenseMap<ValuePair, std::vector<ValuePair> >::iterator I = ConnectedPairs.begin(), IE = ConnectedPairs.end(); I != IE; ++I)
      for (std::vector<ValuePair>::iterator J = I->second.begin(), JE = I->second.end(); J != JE; ++J)
        ConnectedPairDeps[*J].push_back(I->first);

    // Build the pairable-instruction dependency map
    DenseSet<ValuePair> PairableInstUsers;
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    buildDepMap(BB, CandidatePairs, PairableInsts, PairableInstUsers);

    // There is now a graph of the connected pairs. For each variable, pick
    // the pairing with the largest dag meeting the depth requirement on at
    // least one branch. Then select all pairings that are part of that dag
    // and remove them from the list of available pairings and pairable
    // variables.

    DenseMap<Value *, Value *> ChosenPairs;
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    choosePairs(CandidatePairs, CandidatePairsSet, CandidatePairCostSavings,
      PairableInsts, FixedOrderPairs, PairConnectionTypes, ConnectedPairs, ConnectedPairDeps, PairableInstUsers, ChosenPairs); 
    if (ChosenPairs.empty()) continue;
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    AllPairableInsts.insert(AllPairableInsts.end(), PairableInsts.begin(), PairableInsts.end());
    AllChosenPairs.insert(ChosenPairs.begin(), ChosenPairs.end());

    // Only for the chosen pairs, propagate information on fixed-order pairs,
    // pair connections, and their types to the data structures used by the
    // pair fusion procedures.
    for (DenseMap<Value *, Value *>::iterator I = ChosenPairs.begin(), IE = ChosenPairs.end(); I != IE; ++I) {
      if (FixedOrderPairs.count(*I))
        AllFixedOrderPairs.insert(*I);
      else if (FixedOrderPairs.count(ValuePair(I->second, I->first)))
        AllFixedOrderPairs.insert(ValuePair(I->second, I->first));

      for (DenseMap<Value *, Value *>::iterator J = ChosenPairs.begin(); J != IE; ++J) {
        DenseMap<VPPair, unsigned>::iterator K = PairConnectionTypes.find(VPPair(*I, *J));
        if (K != PairConnectionTypes.end()) {
          AllPairConnectionTypes.insert(*K);
        } else {
          K = PairConnectionTypes.find(VPPair(*J, *I));
          if (K != PairConnectionTypes.end())
            AllPairConnectionTypes.insert(*K);
        }
      }
    }

printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    for (DenseMap<ValuePair, std::vector<ValuePair> >::iterator I = ConnectedPairs.begin(), IE = ConnectedPairs.end(); I != IE; ++I)
      for (std::vector<ValuePair>::iterator J = I->second.begin(), JE = I->second.end(); J != JE; ++J)
        if (AllPairConnectionTypes.count(VPPair(I->first, *J))) {
          AllConnectedPairs[I->first].push_back(*J);
          AllConnectedPairDeps[*J].push_back(I->first);
        }
  } while (ShouldContinue);

  if (AllChosenPairs.empty()) return false;
  //NumFusedOps += AllChosenPairs.size();

  // A set of pairs has now been selected. It is now necessary to replace the
  // paired instructions with vector instructions. For this procedure each
  // operand must be replaced with a vector operand. This vector is formed
  // by using build_vector on the old operands. The replaced values are then
  // replaced with a vector_extract on the result.  Subsequent optimization
  // passes should coalesce the build/extract combinations.

  fuseChosenPairs(BB, AllPairableInsts, AllChosenPairs, AllFixedOrderPairs, AllPairConnectionTypes, AllConnectedPairs, AllConnectedPairDeps); 
  // It is important to cleanup here so that future iterations of this // function have less work to do.
#if 0
  (void) SimplifyInstructionsInBlock(&BB, TD, AA->getTargetLibraryInfo());
#endif
  return true;
}

// if the two provided instructions are compatible (meaning that they can be fused into a vector instruction). This assumes
// that I has already been determined to be vectorizable and that J is not // in the use dag of I.
bool BBVectorize::areInstsCompatible(Instruction *I, Instruction *J, int &CostSavings)
{
  dbgs() << "BBV: looking at " << *I << " <-> " << *J << "JCAJCAjcajca\n"; 
  CostSavings = 0; 
  // Loads and stores can be merged if they have different alignments, // but are otherwise the same.
  if (!J->isSameOperationAs(I, Instruction::CompareIgnoringAlignment))
    return false; 
  Type *IT1, *IT2, *JT1, *JT2;
  getInstructionTypes(I, IT1, IT2);
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
  getInstructionTypes(J, JT1, JT2);
  unsigned MaxTypeBits = std::max( IT1->getPrimitiveSizeInBits() + JT1->getPrimitiveSizeInBits(), IT2->getPrimitiveSizeInBits() + JT2->getPrimitiveSizeInBits());
  if (!TTI && MaxTypeBits > Config.VectorBits)
    return false; 
  // The powi intrinsic is special because only the first argument is // vectorized, the second arguments must be equal.
  CallInst *CI = dyn_cast<CallInst>(I);
  Function *FI;
  if (CI && (FI = CI->getCalledFunction())) {
    Intrinsic::ID IID = (Intrinsic::ID) FI->getIntrinsicID();
    if (IID == Intrinsic::powi) {
      Value *A1I = CI->getArgOperand(1), *A1J = cast<CallInst>(J)->getArgOperand(1);
      const SCEV *A1ISCEV = SE->getSCEV(A1I), *A1JSCEV = SE->getSCEV(A1J);
      return (A1ISCEV == A1JSCEV);
    } 
  } 
  CostSavings = 10;
  return true;
}

// Figure out whether or not J uses I and update the users and write-set
// structures associated with I. Specifically, Users represents the set of
// instructions that depend on I. WriteSet represents the set
// of memory locations that are dependent on I. If UpdateUsers is true,
// and J uses I, then Users is updated to contain J and WriteSet is updated
// to contain any memory locations to which J writes. The function returns
// true if J uses I. By default, alias analysis is used to determine
// whether J reads from memory that overlaps with a location in WriteSet.
// If LoadMoveSet is not null, then it is a previously-computed map
// where the key is the memory-based user instruction and the value is
// the instruction to be compared with I. So, if LoadMoveSet is provided,
// then the alias analysis is not used. This is necessary because this
// function is called during the process of moving instructions during
// vectorization and the results of the alias analysis are not stable during
// that process.
bool BBVectorize::trackUsesOfI(DenseSet<Value *> &Users, AliasSetTracker &WriteSet, Instruction *I,
                     Instruction *J, bool UpdateUsers, DenseSet<ValuePair> *LoadMoveSetPairs)
{
  bool UsesI = false; 
  // This instruction may already be marked as a user due, for example, to // being a member of a selected pair.
  if (Users.count(J))
    UsesI = true; 
  if (!UsesI)
    for (User::op_iterator JU = J->op_begin(), JE = J->op_end(); JU != JE; ++JU) {
      Value *V = *JU;
      if (I == V || Users.count(V)) {
        UsesI = true;
        break;
      }
    }
  if (!UsesI && J->mayReadFromMemory()) {
    if (LoadMoveSetPairs) {
      UsesI = LoadMoveSetPairs->count(ValuePair(J, I));
#if 0
    } else {
      for (AliasSetTracker::iterator W = WriteSet.begin(), WE = WriteSet.end(); W != WE; ++W) {
        if (W->aliasesUnknownInst(J, *AA)) {
          UsesI = true;
          break;
        }
      }
#endif
    }
  } 
  if (UsesI && UpdateUsers) {
    if (J->mayWriteToMemory()) WriteSet.add(J);
    Users.insert(J);
  } 
  return UsesI;
}

// This function iterates over all instruction pairs in the provided
// basic block and collects all candidate pairs for vectorization.
bool BBVectorize::getCandidatePairs(BasicBlock &BB, BasicBlock::iterator &Start,
      DenseMap<Value *, std::vector<Value *> > &CandidatePairs, DenseSet<ValuePair> &FixedOrderPairs,
      DenseMap<ValuePair, int> &CandidatePairCostSavings, std::vector<Value *> &PairableInsts)
{
  size_t TotalPairs = 0;
  BasicBlock::iterator E = BB.end();
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
  if (Start == E) return false; 
  bool ShouldContinue = false, IAfterStart = false;
  for (BasicBlock::iterator I = Start++; I != E; ++I) {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    if (I == Start) IAfterStart = true; 
    // Look for an instruction with which to pair instruction *I...
    DenseSet<Value *> Users;
    //AliasSetTracker WriteSet(*AA);
    //if (I->mayWriteToMemory()) WriteSet.add(I); 
    bool JAfterStart = IAfterStart;
    BasicBlock::iterator J = llvm::next(I);
    for (unsigned ss = 0; J != E && ss <= Config.SearchLimit; ++J, ++ss) {
      if (J == Start) JAfterStart = true; 
      // J does not use I, and comes before the first use of I, so it can be // merged with I if the instructions are compatible.
      int CostSavings, FixedOrder = 0;
      if (!areInstsCompatible(I, J, CostSavings)) continue; 
      // J is a candidate for merging with I.
      if (!PairableInsts.size() || PairableInsts[PairableInsts.size()-1] != I) {
        PairableInsts.push_back(I);
      } 
      CandidatePairs[I].push_back(J);
      ++TotalPairs;
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
      if (TTI)
        CandidatePairCostSavings.insert(ValuePairWithCost(ValuePair(I, J), CostSavings)); 
      if (FixedOrder == 1)
        FixedOrderPairs.insert(ValuePair(I, J));
      else if (FixedOrder == -1)
        FixedOrderPairs.insert(ValuePair(J, I)); 
      // The next call to this function must start after the last instruction // selected during this invocation.
      if (JAfterStart) {
        Start = llvm::next(J);
        IAfterStart = JAfterStart = false;
      } 
      dbgs() << "BBV: candidate pair " << *I << " <-> " << *J << " (cost savings: " << CostSavings << ")\n"; 
      // If we have already found too many pairs, break here and this function
      // will be called again starting after the last instruction selected // during this invocation.
      if (PairableInsts.size() >= Config.MaxInsts || TotalPairs >= Config.MaxPairs) {
        ShouldContinue = true;
        break;
      }
    } 
    if (ShouldContinue)
      break;
  } 
  DEBUG(dbgs() << "BBV: found " << PairableInsts.size() << " instructions with candidate pairs\n"); 
  return ShouldContinue;
}

// Finds candidate pairs connected to the pair P = <PI, PJ>. This means that
// it looks for pairs such that both members have an input which is an // output of PI or PJ.
void BBVectorize::computePairsConnectedTo( DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
                DenseSet<ValuePair> &CandidatePairsSet, std::vector<Value *> &PairableInsts,
                DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs,
                DenseMap<VPPair, unsigned> &PairConnectionTypes, ValuePair P)
{
  StoreInst *SI, *SJ;

  // For each possible pairing for this variable, look at the uses of
  // the first value...
  for (Value::use_iterator I = P.first->use_begin(), E = P.first->use_end(); I != E; ++I) {
    if (isa<LoadInst>(*I)) {
      // A pair cannot be connected to a load because the load only takes one
      // operand (the address) and it is a scalar even after vectorization.
      continue;
    } else if ((SI = dyn_cast<StoreInst>(*I)) && P.first == SI->getPointerOperand()) {
      // Similarly, a pair cannot be connected to a store through its // pointer operand.
      continue;
    } 
    // For each use of the first variable, look for uses of the second // variable...
    for (Value::use_iterator J = P.second->use_begin(), E2 = P.second->use_end(); J != E2; ++J) {
      if ((SJ = dyn_cast<StoreInst>(*J)) && P.second == SJ->getPointerOperand())
        continue; 
      // Look for <I, J>:
      if (CandidatePairsSet.count(ValuePair(*I, *J))) {
        VPPair VP(P, ValuePair(*I, *J));
        ConnectedPairs[VP.first].push_back(VP.second);
        PairConnectionTypes.insert(VPPairWithType(VP, PairConnectionDirect));
      } 
      // Look for <J, I>:
      if (CandidatePairsSet.count(ValuePair(*J, *I))) {
        VPPair VP(P, ValuePair(*J, *I));
        ConnectedPairs[VP.first].push_back(VP.second);
        PairConnectionTypes.insert(VPPairWithType(VP, PairConnectionSwap));
      }
    } 
    // Look for cases where just the first value in the pair is used by // both members of another pair (splatting).
    for (Value::use_iterator J = P.first->use_begin(); J != E; ++J) {
      if ((SJ = dyn_cast<StoreInst>(*J)) && P.first == SJ->getPointerOperand())
        continue; 
      if (CandidatePairsSet.count(ValuePair(*I, *J))) {
        VPPair VP(P, ValuePair(*I, *J));
        ConnectedPairs[VP.first].push_back(VP.second);
        PairConnectionTypes.insert(VPPairWithType(VP, PairConnectionSplat));
      }
    }
  } 
  // Look for cases where just the second value in the pair is used by // both members of another pair (splatting).
  for (Value::use_iterator I = P.second->use_begin(), E = P.second->use_end(); I != E; ++I) {
    if (isa<LoadInst>(*I))
      continue;
    else if ((SI = dyn_cast<StoreInst>(*I)) && P.second == SI->getPointerOperand())
      continue; 
    for (Value::use_iterator J = P.second->use_begin(); J != E; ++J) {
      if ((SJ = dyn_cast<StoreInst>(*J)) && P.second == SJ->getPointerOperand())
        continue; 
      if (CandidatePairsSet.count(ValuePair(*I, *J))) {
        VPPair VP(P, ValuePair(*I, *J));
        ConnectedPairs[VP.first].push_back(VP.second);
        PairConnectionTypes.insert(VPPairWithType(VP, PairConnectionSplat));
      }
    }
  }
}

// This function figures out which pairs are connected.  Two pairs are
// connected if some output of the first pair forms an input to both members // of the second pair.
void BBVectorize::computeConnectedPairs( DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
                DenseSet<ValuePair> &CandidatePairsSet, std::vector<Value *> &PairableInsts,
                DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs, DenseMap<VPPair, unsigned> &PairConnectionTypes)
{
  for (std::vector<Value *>::iterator PI = PairableInsts.begin(), PE = PairableInsts.end(); PI != PE; ++PI) {
    DenseMap<Value *, std::vector<Value *> >::iterator PP = CandidatePairs.find(*PI);
    if (PP == CandidatePairs.end())
      continue; 
    for (std::vector<Value *>::iterator P = PP->second.begin(), E = PP->second.end(); P != E; ++P)
      computePairsConnectedTo(CandidatePairs, CandidatePairsSet, PairableInsts, ConnectedPairs, PairConnectionTypes, ValuePair(*PI, *P));
  } 
  size_t TotalPairs = 0;
  for (DenseMap<ValuePair, std::vector<ValuePair> >::iterator I = ConnectedPairs.begin(), IE = ConnectedPairs.end(); I != IE; ++I)
     TotalPairs += I->second.size();
  dbgs() << "BBV: found " << TotalPairs << " pair connections.\n";
}

// This function builds a set of use tuples such that <A, B> is in the set
// if B is in the use dag of A. If B is in the use dag of A, then B // depends on the output of A.
void BBVectorize::buildDepMap( BasicBlock &BB, DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
                    std::vector<Value *> &PairableInsts, DenseSet<ValuePair> &PairableInstUsers)
{
  DenseSet<Value *> IsInPair;
  for (DenseMap<Value *, std::vector<Value *> >::iterator C = CandidatePairs.begin(), E = CandidatePairs.end(); C != E; ++C) {
    IsInPair.insert(C->first);
    IsInPair.insert(C->second.begin(), C->second.end());
  } 
  // Iterate through the basic block, recording all users of each // pairable instruction.  
  BasicBlock::iterator E = BB.end(), EL = BasicBlock::iterator(cast<Instruction>(PairableInsts.back()));
  for (BasicBlock::iterator I = BB.getFirstInsertionPt(); I != E; ++I) {
    if (IsInPair.find(I) == IsInPair.end()) continue; 
    DenseSet<Value *> Users;
    //AliasSetTracker WriteSet(*AA);
    //if (I->mayWriteToMemory()) WriteSet.add(I); 
    for (BasicBlock::iterator J = llvm::next(I); J != E; ++J) {
      //(void) trackUsesOfI(Users, WriteSet, I, J); 
      if (J == EL)
        break;
    } 
    for (DenseSet<Value *>::iterator U = Users.begin(), E = Users.end(); U != E; ++U) {
      if (IsInPair.find(*U) == IsInPair.end()) continue;
      PairableInstUsers.insert(ValuePair(I, *U));
    } 
    if (I == EL)
      break;
  }
}

// This function builds the initial dag of connected pairs with the // pair J at the root.
void BBVectorize::buildInitialDAGFor( DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
                DenseSet<ValuePair> &CandidatePairsSet, std::vector<Value *> &PairableInsts,
                DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs, DenseSet<ValuePair> &PairableInstUsers,
                DenseMap<Value *, Value *> &ChosenPairs, DenseMap<ValuePair, size_t> &DAG, ValuePair J)
{
  // Each of these pairs is viewed as the root node of a DAG. The DAG
  // is then walked (depth-first). As this happens, we keep track of
  // the pairs that compose the DAG and the maximum depth of the DAG.
  SmallVector<ValuePairWithDepth, 32> Q;
  // General depth-first post-order traversal:
  Q.push_back(ValuePairWithDepth(J, getDepthFactor(J.first)));
  do {
    ValuePairWithDepth QTop = Q.back(); 
    // Push each child onto the queue:
    bool MoreChildren = false;
    size_t MaxChildDepth = QTop.second;
    DenseMap<ValuePair, std::vector<ValuePair> >::iterator QQ = ConnectedPairs.find(QTop.first);
    if (QQ != ConnectedPairs.end())
      for (std::vector<ValuePair>::iterator k = QQ->second.begin(), ke = QQ->second.end(); k != ke; ++k) {
        // Make sure that this child pair is still a candidate:
        if (CandidatePairsSet.count(*k)) {
          DenseMap<ValuePair, size_t>::iterator C = DAG.find(*k);
          if (C == DAG.end()) {
            size_t d = getDepthFactor(k->first);
            Q.push_back(ValuePairWithDepth(*k, QTop.second+d));
            MoreChildren = true;
          } else {
            MaxChildDepth = std::max(MaxChildDepth, C->second);
          }
        }
      } 
    if (!MoreChildren) {
      // Record the current pair as part of the DAG:
      DAG.insert(ValuePairWithDepth(QTop.first, MaxChildDepth));
      Q.pop_back();
    }
  } while (!Q.empty());
}

// Given some initial dag, prune it by removing conflicting pairs (pairs
// that cannot be simultaneously chosen for vectorization).
void BBVectorize::pruneDAGFor( DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
            std::vector<Value *> &PairableInsts, DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs,
            DenseSet<ValuePair> &PairableInstUsers,
            DenseMap<Value *, Value *> &ChosenPairs,
            DenseMap<ValuePair, size_t> &DAG, DenseSet<ValuePair> &PrunedDAG, ValuePair J)
{
  SmallVector<ValuePairWithDepth, 32> Q;
  // General depth-first post-order traversal:
  Q.push_back(ValuePairWithDepth(J, getDepthFactor(J.first)));
  do {
    ValuePairWithDepth QTop = Q.pop_back_val();
    PrunedDAG.insert(QTop.first); 
    // Visit each child, pruning as necessary...
    SmallVector<ValuePairWithDepth, 8> BestChildren;
    DenseMap<ValuePair, std::vector<ValuePair> >::iterator QQ = ConnectedPairs.find(QTop.first);
    if (QQ == ConnectedPairs.end())
      continue; 
    for (std::vector<ValuePair>::iterator K = QQ->second.begin(), KE = QQ->second.end(); K != KE; ++K) {
      DenseMap<ValuePair, size_t>::iterator C = DAG.find(*K);
      if (C == DAG.end()) continue; 
      // This child is in the DAG, now we need to make sure it is the
      // best of any conflicting children. There could be multiple
      // conflicting children, so first, determine if we're keeping
      // this child, then delete conflicting children as necessary.

      // It is also necessary to guard against pairing-induced
      // dependencies. Consider instructions a .. x .. y .. b
      // such that (a,b) are to be fused and (x,y) are to be fused
      // but a is an input to x and b is an output from y. This
      // means that y cannot be moved after b but x must be moved
      // after b for (a,b) to be fused. In other words, after
      // fusing (a,b) we have y .. a/b .. x where y is an input
      // to a/b and x is an output to a/b: x and y can no longer
      // be legally fused. To prevent this condition, we must
      // make sure that a child pair added to the DAG is not
      // both an input and output of an already-selected pair.

      // Pairing-induced dependencies can also form from more complicated
      // cycles. The pair vs. pair conflicts are easy to check, and so
      // that is done explicitly for "fast rejection", and because for
      // child vs. child conflicts, we may prefer to keep the current
      // pair in preference to the already-selected child.
      DenseSet<ValuePair> CurrentPairs; 
      bool CanAdd = true;
      for (SmallVectorImpl<ValuePairWithDepth>::iterator C2 = BestChildren.begin(), E2 = BestChildren.end(); C2 != E2; ++C2) {
        if (C2->first.first == C->first.first || C2->first.first == C->first.second ||
            C2->first.second == C->first.first || C2->first.second == C->first.second) {
          if (C2->second >= C->second) {
            CanAdd = false;
            break;
          } 
          CurrentPairs.insert(C2->first);
        }
      }
      if (!CanAdd) continue; 
      // Even worse, this child could conflict with another node already
      // selected for the DAG. If that is the case, ignore this child.
      for (DenseSet<ValuePair>::iterator T = PrunedDAG.begin(), E2 = PrunedDAG.end(); T != E2; ++T) {
        if (T->first == C->first.first || T->first == C->first.second ||
            T->second == C->first.first || T->second == C->first.second) {
          CanAdd = false;
          break;
        } 
        CurrentPairs.insert(*T);
      }
      if (!CanAdd) continue; 
      // And check the queue too...
      for (SmallVectorImpl<ValuePairWithDepth>::iterator C2 = Q.begin(), E2 = Q.end(); C2 != E2; ++C2) {
        if (C2->first.first == C->first.first || C2->first.first == C->first.second ||
            C2->first.second == C->first.first || C2->first.second == C->first.second) {
          CanAdd = false;
          break;
        } 
        CurrentPairs.insert(C2->first);
      }
      if (!CanAdd) continue; 
      // Last but not least, check for a conflict with any of the // already-chosen pairs.
      for (DenseMap<Value *, Value *>::iterator C2 = ChosenPairs.begin(), E2 = ChosenPairs.end(); C2 != E2; ++C2) {
        CurrentPairs.insert(*C2);
      }
      if (!CanAdd) continue; 

      // This child can be added, but we may have chosen it in preference
      // to an already-selected child. Check for this here, and if a
      // conflict is found, then remove the previously-selected child
      // before adding this one in its place.
      for (SmallVectorImpl<ValuePairWithDepth>::iterator C2 = BestChildren.begin(); C2 != BestChildren.end();) {
        if (C2->first.first == C->first.first || C2->first.first == C->first.second ||
            C2->first.second == C->first.first || C2->first.second == C->first.second)
          C2 = BestChildren.erase(C2);
        else
          ++C2;
      } 
      BestChildren.push_back(ValuePairWithDepth(C->first, C->second));
    } 
    for (SmallVectorImpl<ValuePairWithDepth>::iterator C = BestChildren.begin(), E2 = BestChildren.end(); C != E2; ++C) {
      size_t DepthF = getDepthFactor(C->first.first);
      Q.push_back(ValuePairWithDepth(C->first, QTop.second+DepthF));
    }
  } while (!Q.empty());
}

// This function finds the best dag of mututally-compatible connected
// pairs, given the choice of root pairs as an iterator range.
void BBVectorize::findBestDAGFor( DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
            DenseSet<ValuePair> &CandidatePairsSet, DenseMap<ValuePair, int> &CandidatePairCostSavings,
            std::vector<Value *> &PairableInsts, DenseSet<ValuePair> &FixedOrderPairs,
            DenseMap<VPPair, unsigned> &PairConnectionTypes, DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs,
            DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairDeps,
            DenseSet<ValuePair> &PairableInstUsers, DenseMap<Value *, Value *> &ChosenPairs,
            DenseSet<ValuePair> &BestDAG, size_t &BestMaxDepth, int &BestEffSize, Value *II, std::vector<Value *>&JJ)
{
  for (std::vector<Value *>::iterator J = JJ.begin(), JE = JJ.end(); J != JE; ++J) {
    ValuePair IJ(II, *J);
    if (!CandidatePairsSet.count(IJ))
      continue; 
    // Before going any further, make sure that this pair does not
    // conflict with any already-selected pairs (see comment below // near the DAG pruning for more details).
    DenseSet<ValuePair> ChosenPairSet;
    bool DoesConflict = false;
    for (DenseMap<Value *, Value *>::iterator C = ChosenPairs.begin(), E = ChosenPairs.end(); C != E; ++C) {
      ChosenPairSet.insert(*C);
    }
    if (DoesConflict) continue; 
    DenseMap<ValuePair, size_t> DAG;
    buildInitialDAGFor(CandidatePairs, CandidatePairsSet, PairableInsts, ConnectedPairs, PairableInstUsers, ChosenPairs, DAG, IJ); 
    // Because we'll keep the child with the largest depth, the largest
    // depth is still the same in the unpruned DAG.
    size_t MaxDepth = DAG.lookup(IJ); 
    dbgs() << "BBV: found DAG for pair {" << *IJ.first << " <-> " << *IJ.second << "} of depth " << MaxDepth << " and size " << DAG.size() << "\n"; 
    // At this point the DAG has been constructed, but, may contain // contradictory children (meaning that different children of
    // some dag node may be attempting to fuse the same instruction).  // So now we walk the dag again, in the case of a conflict,
    // keep only the child with the largest depth. To break a tie, // favor the first child.  
    DenseSet<ValuePair> PrunedDAG;
    pruneDAGFor(CandidatePairs, PairableInsts, ConnectedPairs, PairableInstUsers,
                 ChosenPairs, DAG, PrunedDAG, IJ); 
    int EffSize = 0;
    if (TTI) {
      DenseSet<Value *> PrunedDAGInstrs;
      for (DenseSet<ValuePair>::iterator S = PrunedDAG.begin(), E = PrunedDAG.end(); S != E; ++S) {
        PrunedDAGInstrs.insert(S->first);
        PrunedDAGInstrs.insert(S->second);
      } 
      // The set of pairs that have already contributed to the total cost.
      DenseSet<ValuePair> IncomingPairs; 
      // If the cost model were perfect, this might not be necessary; but we
      // need to make sure that we don't get stuck vectorizing our own // shuffle chains.
      bool HasNontrivialInsts = false; 
      // The node weights represent the cost savings associated with
      // fusing the pair of instructions.
      for (DenseSet<ValuePair>::iterator S = PrunedDAG.begin(), E = PrunedDAG.end(); S != E; ++S) {
        if (!isa<ShuffleVectorInst>(S->first) && !isa<InsertElementInst>(S->first) && !isa<ExtractElementInst>(S->first))
          HasNontrivialInsts = true; 
        bool FlipOrder = false; 
        if (getDepthFactor(S->first)) {
          int ESContrib = CandidatePairCostSavings.find(*S)->second;
          dbgs() << "\tweight {" << *S->first << " <-> " << *S->second << "} = " << ESContrib << "\n";
          EffSize += ESContrib;
        } 
        // The edge weights contribute in a negative sense: they represent // the cost of shuffles.
        DenseMap<ValuePair, std::vector<ValuePair> >::iterator SS = ConnectedPairDeps.find(*S);
        if (SS != ConnectedPairDeps.end()) {
          unsigned NumDepsDirect = 0, NumDepsSwap = 0;
          for (std::vector<ValuePair>::iterator T = SS->second.begin(), TE = SS->second.end(); T != TE; ++T) {
            VPPair Q(*S, *T);
            if (!PrunedDAG.count(Q.second))
              continue;
            DenseMap<VPPair, unsigned>::iterator R = PairConnectionTypes.find(VPPair(Q.second, Q.first));
            assert(R != PairConnectionTypes.end() && "Cannot find pair connection type");
            if (R->second == PairConnectionDirect)
              ++NumDepsDirect;
            else if (R->second == PairConnectionSwap)
              ++NumDepsSwap;
          } 
          // If there are more swaps than direct connections, then
          // the pair order will be flipped during fusion. So the real // number of swaps is the minimum number.
          FlipOrder = !FixedOrderPairs.count(*S) && ((NumDepsSwap > NumDepsDirect) || FixedOrderPairs.count(ValuePair(S->second, S->first))); 
          for (std::vector<ValuePair>::iterator T = SS->second.begin(), TE = SS->second.end(); T != TE; ++T) {
            VPPair Q(*S, *T);
            if (!PrunedDAG.count(Q.second))
              continue;
            DenseMap<VPPair, unsigned>::iterator R = PairConnectionTypes.find(VPPair(Q.second, Q.first));
            assert(R != PairConnectionTypes.end() && "Cannot find pair connection type");
            Type *Ty1 = Q.second.first->getType(),
                 *Ty2 = Q.second.second->getType();
            Type *VTy = getVecTypeForPair(Ty1, Ty2);
            if ((R->second == PairConnectionDirect && FlipOrder) ||
                (R->second == PairConnectionSwap && !FlipOrder)  || R->second == PairConnectionSplat) {
              int ESContrib = (int) getInstrCost(Instruction::ShuffleVector, VTy, VTy); 
              if (VTy->getVectorNumElements() == 2) {
                if (R->second == PairConnectionSplat)
                  ESContrib = std::min(ESContrib, (int) TTI->getShuffleCost( TargetTransformInfo::SK_Broadcast, VTy));
                else
                  ESContrib = std::min(ESContrib, (int) TTI->getShuffleCost( TargetTransformInfo::SK_Reverse, VTy));
              }

              dbgs() << "\tcost {" << *Q.second.first << " <-> " << *Q.second.second << "} -> {" << *S->first << " <-> " << *S->second << "} = " << ESContrib << "\n";
              EffSize -= ESContrib;
            }
          }
        } 
        // Compute the cost of outgoing edges. We assume that edges outgoing
        // to shuffles, inserts or extracts can be merged, and so contribute // no additional cost.
        if (!S->first->getType()->isVoidTy()) {
          Type *Ty1 = S->first->getType(), *Ty2 = S->second->getType();
          Type *VTy = getVecTypeForPair(Ty1, Ty2); 
          bool NeedsExtraction = false;
          for (Value::use_iterator I = S->first->use_begin(), IE = S->first->use_end(); I != IE; ++I) {
            if (ShuffleVectorInst *SI = dyn_cast<ShuffleVectorInst>(*I)) {
              // Shuffle can be folded if it has no other input
              if (isa<UndefValue>(SI->getOperand(1)))
                continue;
            }
            if (isa<ExtractElementInst>(*I))
              continue;
            if (PrunedDAGInstrs.count(*I))
              continue;
            NeedsExtraction = true;
            break;
          } 
          if (NeedsExtraction) {
            int ESContrib;
            if (Ty1->isVectorTy()) {
              ESContrib = (int) getInstrCost(Instruction::ShuffleVector, Ty1, VTy);
              ESContrib = std::min(ESContrib, (int) TTI->getShuffleCost(
                TargetTransformInfo::SK_ExtractSubvector, VTy, 0, Ty1));
            } else
              ESContrib = (int) TTI->getVectorInstrCost( Instruction::ExtractElement, VTy, 0); 
            dbgs() << "\tcost {" << *S->first << "} = " << ESContrib << "\n";
            EffSize -= ESContrib;
          } 
          NeedsExtraction = false;
          for (Value::use_iterator I = S->second->use_begin(), IE = S->second->use_end(); I != IE; ++I) {
            if (ShuffleVectorInst *SI = dyn_cast<ShuffleVectorInst>(*I)) {
              // Shuffle can be folded if it has no other input
              if (isa<UndefValue>(SI->getOperand(1)))
                continue;
            }
            if (isa<ExtractElementInst>(*I))
              continue;
            if (PrunedDAGInstrs.count(*I))
              continue;
            NeedsExtraction = true;
            break;
          } 
          if (NeedsExtraction) {
            int ESContrib;
            if (Ty2->isVectorTy()) {
              ESContrib = (int) getInstrCost(Instruction::ShuffleVector, Ty2, VTy);
              ESContrib = std::min(ESContrib, (int) TTI->getShuffleCost(
                TargetTransformInfo::SK_ExtractSubvector, VTy, Ty1->isVectorTy() ? Ty1->getVectorNumElements() : 1, Ty2));
            } else
              ESContrib = (int) TTI->getVectorInstrCost( Instruction::ExtractElement, VTy, 1);
            dbgs() << "\tcost {" << *S->second << "} = " << ESContrib << "\n";
            EffSize -= ESContrib;
          }
        } 
        // Compute the cost of incoming edges.
        if (!isa<LoadInst>(S->first) && !isa<StoreInst>(S->first)) {
          Instruction *S1 = cast<Instruction>(S->first), *S2 = cast<Instruction>(S->second);
          for (unsigned o = 0; o < S1->getNumOperands(); ++o) {
            Value *O1 = S1->getOperand(o), *O2 = S2->getOperand(o); 
            // Combining constants into vector constants (or small vector // constants into larger ones are assumed free).
            if (isa<Constant>(O1) && isa<Constant>(O2))
              continue; 
            if (FlipOrder)
              std::swap(O1, O2); 
            ValuePair VP  = ValuePair(O1, O2);
            ValuePair VPR = ValuePair(O2, O1); 
            // Internal edges are not handled here.
            if (PrunedDAG.count(VP) || PrunedDAG.count(VPR))
              continue; 
            Type *Ty1 = O1->getType(), *Ty2 = O2->getType();
            Type *VTy = getVecTypeForPair(Ty1, Ty2); 
            // Combining vector operations of the same type is also assumed // folded with other operations.
            if (Ty1 == Ty2) {
              // If both are insert elements, then both can be widened.
              InsertElementInst *IEO1 = dyn_cast<InsertElementInst>(O1), *IEO2 = dyn_cast<InsertElementInst>(O2);
              if (IEO1 && IEO2 && isPureIEChain(IEO1) && isPureIEChain(IEO2))
                continue;
              // If both are extract elements, and both have the same input
              // type, then they can be replaced with a shuffle
              ExtractElementInst *EIO1 = dyn_cast<ExtractElementInst>(O1), *EIO2 = dyn_cast<ExtractElementInst>(O2);
              if (EIO1 && EIO2 && EIO1->getOperand(0)->getType() == EIO2->getOperand(0)->getType())
                continue;
              // If both are a shuffle with equal operand types and only two
              // unqiue operands, then they can be replaced with a single // shuffle
              ShuffleVectorInst *SIO1 = dyn_cast<ShuffleVectorInst>(O1), *SIO2 = dyn_cast<ShuffleVectorInst>(O2);
              if (SIO1 && SIO2 && SIO1->getOperand(0)->getType() == SIO2->getOperand(0)->getType()) {
                SmallSet<Value *, 4> SIOps;
                SIOps.insert(SIO1->getOperand(0));
                SIOps.insert(SIO1->getOperand(1));
                SIOps.insert(SIO2->getOperand(0));
                SIOps.insert(SIO2->getOperand(1));
                if (SIOps.size() <= 2)
                  continue;
              }
            } 
            int ESContrib;
            // This pair has already been formed.
            if (IncomingPairs.count(VP)) {
              continue;
            } else if (IncomingPairs.count(VPR)) {
              ESContrib = (int) getInstrCost(Instruction::ShuffleVector, VTy, VTy); 
              if (VTy->getVectorNumElements() == 2)
                ESContrib = std::min(ESContrib, (int) TTI->getShuffleCost( TargetTransformInfo::SK_Reverse, VTy));
            } else if (!Ty1->isVectorTy() && !Ty2->isVectorTy()) {
              ESContrib = (int) TTI->getVectorInstrCost( Instruction::InsertElement, VTy, 0);
              ESContrib += (int) TTI->getVectorInstrCost( Instruction::InsertElement, VTy, 1);
            } else if (!Ty1->isVectorTy()) {
              // O1 needs to be inserted into a vector of size O2, and then
              // both need to be shuffled together.
              ESContrib = (int) TTI->getVectorInstrCost( Instruction::InsertElement, Ty2, 0);
              ESContrib += (int) getInstrCost(Instruction::ShuffleVector, VTy, Ty2);
            } else if (!Ty2->isVectorTy()) {
              // O2 needs to be inserted into a vector of size O1, and then
              // both need to be shuffled together.
              ESContrib = (int) TTI->getVectorInstrCost( Instruction::InsertElement, Ty1, 0);
              ESContrib += (int) getInstrCost(Instruction::ShuffleVector, VTy, Ty1);
            } else {
              Type *TyBig = Ty1, *TySmall = Ty2;
              if (Ty2->getVectorNumElements() > Ty1->getVectorNumElements())
                std::swap(TyBig, TySmall); 
              ESContrib = (int) getInstrCost(Instruction::ShuffleVector, VTy, TyBig);
              if (TyBig != TySmall)
                ESContrib += (int) getInstrCost(Instruction::ShuffleVector, TyBig, TySmall);
            } 
            dbgs() << "\tcost {" << *O1 << " <-> " << *O2 << "} = " << ESContrib << "\n";
            EffSize -= ESContrib;
            IncomingPairs.insert(VP);
          }
        }
      } 
      if (!HasNontrivialInsts) {
        dbgs() << "\tNo non-trivial instructions in DAG;" " override to zero effective size\n";
        EffSize = 0;
      }
    } else {
      for (DenseSet<ValuePair>::iterator S = PrunedDAG.begin(), E = PrunedDAG.end(); S != E; ++S)
        EffSize += (int) getDepthFactor(S->first);
    } 
    dbgs() << "BBV: found pruned DAG for pair {" << *IJ.first << " <-> " << *IJ.second << "} of depth " << MaxDepth << " and size " << PrunedDAG.size() << " (effective size: " << EffSize << ")\n";
    if ((MaxDepth >= Config.ReqChainDepth) && EffSize > 0 && EffSize > BestEffSize) {
      BestMaxDepth = MaxDepth;
      BestEffSize = EffSize;
      BestDAG = PrunedDAG;
    }
  }
}

// Given the list of candidate pairs, this function selects those
// that will be fused into vector instructions.
void BBVectorize::choosePairs( DenseMap<Value *, std::vector<Value *> > &CandidatePairs,
              DenseSet<ValuePair> &CandidatePairsSet, DenseMap<ValuePair, int> &CandidatePairCostSavings,
              std::vector<Value *> &PairableInsts, DenseSet<ValuePair> &FixedOrderPairs,
              DenseMap<VPPair, unsigned> &PairConnectionTypes, DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs,
              DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairDeps,
              DenseSet<ValuePair> &PairableInstUsers, DenseMap<Value *, Value *>& ChosenPairs)
{
  DenseMap<Value *, std::vector<Value *> > CandidatePairs2;
  for (DenseSet<ValuePair>::iterator I = CandidatePairsSet.begin(), E = CandidatePairsSet.end(); I != E; ++I) {
    std::vector<Value *> &JJ = CandidatePairs2[I->second];
    if (JJ.empty()) JJ.reserve(32);
    JJ.push_back(I->first);
  } 
  for (std::vector<Value *>::iterator I = PairableInsts.begin(), E = PairableInsts.end(); I != E; ++I) {
    // The number of possible pairings for this variable:
    size_t NumChoices = CandidatePairs.lookup(*I).size();
    if (!NumChoices) continue; 
    std::vector<Value *> &JJ = CandidatePairs[*I]; 
    // The best pair to choose and its dag:
    size_t BestMaxDepth = 0;
    int BestEffSize = 0;
    DenseSet<ValuePair> BestDAG;
    findBestDAGFor(CandidatePairs, CandidatePairsSet, CandidatePairCostSavings,
                    PairableInsts, FixedOrderPairs, PairConnectionTypes, ConnectedPairs, ConnectedPairDeps,
                    PairableInstUsers, ChosenPairs, BestDAG, BestMaxDepth, BestEffSize, *I, JJ); 
    if (BestDAG.empty())
      continue; 
    // A dag has been chosen (or not) at this point. If no dag was
    // chosen, then this instruction, I, cannot be paired (and is no longer // considered).  
    DEBUG(dbgs() << "BBV: selected pairs in the best DAG for: " << *cast<Instruction>(*I) << "\n"); 
    for (DenseSet<ValuePair>::iterator S = BestDAG.begin(), SE2 = BestDAG.end(); S != SE2; ++S) {
      // Insert the members of this dag into the list of chosen pairs.
      ChosenPairs.insert(ValuePair(S->first, S->second));
      DEBUG(dbgs() << "BBV: selected pair: " << *S->first << " <-> " << *S->second << "\n"); 
      // Remove all candidate pairs that have values in the chosen dag.
      std::vector<Value *> &KK = CandidatePairs[S->first];
      for (std::vector<Value *>::iterator K = KK.begin(), KE = KK.end(); K != KE; ++K) {
        if (*K == S->second)
          continue; 
        CandidatePairsSet.erase(ValuePair(S->first, *K));
      } 
      std::vector<Value *> &LL = CandidatePairs2[S->second];
      for (std::vector<Value *>::iterator L = LL.begin(), LE = LL.end(); L != LE; ++L) {
        if (*L == S->first)
          continue; 
        CandidatePairsSet.erase(ValuePair(*L, S->second));
      } 
      std::vector<Value *> &MM = CandidatePairs[S->second];
      for (std::vector<Value *>::iterator M = MM.begin(), ME = MM.end(); M != ME; ++M) {
        assert(*M != S->first && "Flipped pair in candidate list?");
        CandidatePairsSet.erase(ValuePair(S->second, *M));
      } 
      std::vector<Value *> &NN = CandidatePairs2[S->first];
      for (std::vector<Value *>::iterator N = NN.begin(), NE = NN.end(); N != NE; ++N) {
        assert(*N != S->second && "Flipped pair in candidate list?");
        CandidatePairsSet.erase(ValuePair(*N, S->first));
      }
    }
  } 
  DEBUG(dbgs() << "BBV: selected " << ChosenPairs.size() << " pairs.\n");
}

std::string getReplacementName(Instruction *I, bool IsInput, unsigned o, unsigned n = 0)
{
  if (!I->hasName())
    return ""; 
  return (I->getName() + (IsInput ? ".v.i" : ".v.r") + utostr(o) + (n > 0 ? "." + utostr(n) : "")).str();
}

// Returns the value that is to be used as the pointer input to the vector // instruction that fuses I with J.
Value *BBVectorize::getReplacementPointerInput(LLVMContext& Context, Instruction *I, Instruction *J, unsigned o)
{
  Value *IPtr, *JPtr;
  unsigned IAlignment, JAlignment, IAddressSpace, JAddressSpace;
  int64_t OffsetInElmts; 
  // Note: the analysis might fail here, that is why the pair order has
  // been precomputed (OffsetInElmts must be unused here).
  (void) getPairPtrInfo(I, J, IPtr, JPtr, IAlignment, JAlignment, IAddressSpace, JAddressSpace, OffsetInElmts, false); 
  // The pointer value is taken to be the one with the lowest offset.
  Value *VPtr = IPtr; 
  Type *ArgTypeI = IPtr->getType()->getPointerElementType();
  Type *ArgTypeJ = JPtr->getType()->getPointerElementType();
  Type *VArgType = getVecTypeForPair(ArgTypeI, ArgTypeJ);
  Type *VArgPtrType = PointerType::get(VArgType, IPtr->getType()->getPointerAddressSpace());
  return new BitCastInst(VPtr, VArgPtrType, getReplacementName(I, true, o), /* insert before */ I);
}

void BBVectorize::fillNewShuffleMask(LLVMContext& Context, Instruction *J, unsigned MaskOffset, unsigned NumInElem,
                   unsigned NumInElem1, unsigned IdxOffset, std::vector<Constant*> &Mask)
{
  unsigned NumElem1 = J->getType()->getVectorNumElements();
  for (unsigned v = 0; v < NumElem1; ++v) {
    int m = cast<ShuffleVectorInst>(J)->getMaskValue(v);
    if (m < 0) {
      Mask[v+MaskOffset] = UndefValue::get(Type::getInt32Ty(Context));
    } else {
      unsigned mm = m + (int) IdxOffset;
      if (m >= (int) NumInElem1)
        mm += (int) NumInElem; 
      Mask[v+MaskOffset] = ConstantInt::get(Type::getInt32Ty(Context), mm);
    }
  }
}

// Returns the value that is to be used as the vector-shuffle mask to the // vector instruction that fuses I with J.
Value *BBVectorize::getReplacementShuffleMask(LLVMContext& Context, Instruction *I, Instruction *J)
{
  // This is the shuffle mask. We need to append the second
  // mask to the first, and the numbers need to be adjusted.  
  Type *ArgTypeI = I->getType();
  Type *ArgTypeJ = J->getType();
  Type *VArgType = getVecTypeForPair(ArgTypeI, ArgTypeJ); 
  unsigned NumElemI = ArgTypeI->getVectorNumElements(); 
  // Get the total number of elements in the fused vector type.
  // By definition, this must equal the number of elements in // the final mask.
  unsigned NumElem = VArgType->getVectorNumElements();
  std::vector<Constant*> Mask(NumElem); 
  Type *OpTypeI = I->getOperand(0)->getType();
  unsigned NumInElemI = OpTypeI->getVectorNumElements();
  Type *OpTypeJ = J->getOperand(0)->getType();
  unsigned NumInElemJ = OpTypeJ->getVectorNumElements(); 
  // The fused vector will be:
  // -----------------------------------------------------
  // | NumInElemI | NumInElemJ | NumInElemI | NumInElemJ |
  // -----------------------------------------------------
  // from which we'll extract NumElem total elements (where the first NumElemI
  // of them come from the mask in I and the remainder come from the mask // in J.  
  // For the mask from the first pair...
  fillNewShuffleMask(Context, I, 0,        NumInElemJ, NumInElemI, 0,          Mask); 
  // For the mask from the second pair...
  fillNewShuffleMask(Context, J, NumElemI, NumInElemI, NumInElemJ, NumInElemI, Mask); 
  return ConstantVector::get(Mask);
}

bool BBVectorize::expandIEChain(LLVMContext& Context, Instruction *I, Instruction *J, unsigned o, Value *&LOp,
                                unsigned numElemL, Type *ArgTypeL, Type *ArgTypeH, bool IBeforeJ, unsigned IdxOff)
{
  bool ExpandedIEChain = false;
  if (InsertElementInst *LIE = dyn_cast<InsertElementInst>(LOp)) {
    // If we have a pure insertelement chain, then this can be rewritten
    // into a chain that directly builds the larger type.
    if (isPureIEChain(LIE)) {
      SmallVector<Value *, 8> VectElemts(numElemL, UndefValue::get(ArgTypeL->getScalarType()));
      InsertElementInst *LIENext = LIE;
      do {
        unsigned Idx = cast<ConstantInt>(LIENext->getOperand(2))->getSExtValue();
        VectElemts[Idx] = LIENext->getOperand(1);
      } while ((LIENext = dyn_cast<InsertElementInst>(LIENext->getOperand(0)))); 
      LIENext = 0;
      Value *LIEPrev = UndefValue::get(ArgTypeH);
      for (unsigned i = 0; i < numElemL; ++i) {
        if (isa<UndefValue>(VectElemts[i])) continue;
        LIENext = InsertElementInst::Create(LIEPrev, VectElemts[i], ConstantInt::get(Type::getInt32Ty(Context), i + IdxOff),
                           getReplacementName(IBeforeJ ? I : J, true, o, i+1));
        LIENext->insertBefore(IBeforeJ ? J : I);
        LIEPrev = LIENext;
      } 
      LOp = LIENext ? (Value*) LIENext : UndefValue::get(ArgTypeH);
      ExpandedIEChain = true;
    }
  } 
  return ExpandedIEChain;
}

static unsigned getNumScalarElements(Type *Ty)
{
  if (VectorType *VecTy = dyn_cast<VectorType>(Ty))
    return VecTy->getNumElements();
  return 1;
}

// Returns the value to be used as the specified operand of the vector
// instruction that fuses I with J.
Value *BBVectorize::getReplacementInput(LLVMContext& Context, Instruction *I, Instruction *J, unsigned o, bool IBeforeJ)
{
  Value *CV0 = ConstantInt::get(Type::getInt32Ty(Context), 0);
  Value *CV1 = ConstantInt::get(Type::getInt32Ty(Context), 1); 
  // Compute the fused vector type for this operand
  Type *ArgTypeI = I->getOperand(o)->getType();
  Type *ArgTypeJ = J->getOperand(o)->getType();
  VectorType *VArgType = getVecTypeForPair(ArgTypeI, ArgTypeJ); 
  Instruction *L = I, *H = J;
  Type *ArgTypeL = ArgTypeI, *ArgTypeH = ArgTypeJ; 
  unsigned numElemL = getNumScalarElements(ArgTypeL);
  unsigned numElemH = getNumScalarElements(ArgTypeH); 
  Value *LOp = L->getOperand(o);
  Value *HOp = H->getOperand(o);
  unsigned numElem = VArgType->getNumElements(); 
  // First, we check if we can reuse the "original" vector outputs (if these
  // exist). We might need a shuffle.
  ExtractElementInst *LEE = dyn_cast<ExtractElementInst>(LOp);
  ExtractElementInst *HEE = dyn_cast<ExtractElementInst>(HOp);
  ShuffleVectorInst *LSV = dyn_cast<ShuffleVectorInst>(LOp);
  ShuffleVectorInst *HSV = dyn_cast<ShuffleVectorInst>(HOp);

  // FIXME: If we're fusing shuffle instructions, then we can't apply this
  // optimization. The input vectors to the shuffle might be a different
  // length from the shuffle outputs. Unfortunately, the replacement
  // shuffle mask has already been formed, and the mask entries are sensitive
  // to the sizes of the inputs.
  bool IsSizeChangeShuffle = isa<ShuffleVectorInst>(L) && (LOp->getType() != L->getType() || HOp->getType() != H->getType()); 
  if ((LEE || LSV) && (HEE || HSV) && !IsSizeChangeShuffle) {
    // We can have at most two unique vector inputs.
    bool CanUseInputs = true;
    Value *I1, *I2 = 0;
    if (LEE) {
      I1 = LEE->getOperand(0);
    } else {
      I1 = LSV->getOperand(0);
      I2 = LSV->getOperand(1);
      if (I2 == I1 || isa<UndefValue>(I2))
        I2 = 0;
    } 
    if (HEE) {
      Value *I3 = HEE->getOperand(0);
      if (!I2 && I3 != I1)
        I2 = I3;
      else if (I3 != I1 && I3 != I2)
        CanUseInputs = false;
    } else {
      Value *I3 = HSV->getOperand(0);
      if (!I2 && I3 != I1)
        I2 = I3;
      else if (I3 != I1 && I3 != I2)
        CanUseInputs = false;

      if (CanUseInputs) {
        Value *I4 = HSV->getOperand(1);
        if (!isa<UndefValue>(I4)) {
          if (!I2 && I4 != I1)
            I2 = I4;
          else if (I4 != I1 && I4 != I2)
            CanUseInputs = false;
        }
      }
    } 
    if (CanUseInputs) {
      unsigned LOpElem = cast<Instruction>(LOp)->getOperand(0)->getType() ->getVectorNumElements(); 
      unsigned HOpElem = cast<Instruction>(HOp)->getOperand(0)->getType() ->getVectorNumElements(); 
      // We have one or two input vectors. We need to map each index of the
      // operands to the index of the original vector.
      SmallVector<std::pair<int, int>, 8>  II(numElem);
      for (unsigned i = 0; i < numElemL; ++i) {
        int Idx, INum;
        if (LEE) {
          Idx = cast<ConstantInt>(LEE->getOperand(1))->getSExtValue();
          INum = LEE->getOperand(0) == I1 ? 0 : 1;
        } else {
          Idx = LSV->getMaskValue(i);
          if (Idx < (int) LOpElem) {
            INum = LSV->getOperand(0) == I1 ? 0 : 1;
          } else {
            Idx -= LOpElem;
            INum = LSV->getOperand(1) == I1 ? 0 : 1;
          }
        }

        II[i] = std::pair<int, int>(Idx, INum);
      }
      for (unsigned i = 0; i < numElemH; ++i) {
        int Idx, INum;
        if (HEE) {
          Idx = cast<ConstantInt>(HEE->getOperand(1))->getSExtValue();
          INum = HEE->getOperand(0) == I1 ? 0 : 1;
        } else {
          Idx = HSV->getMaskValue(i);
          if (Idx < (int) HOpElem) {
            INum = HSV->getOperand(0) == I1 ? 0 : 1;
          } else {
            Idx -= HOpElem;
            INum = HSV->getOperand(1) == I1 ? 0 : 1;
          }
        } 
        II[i + numElemL] = std::pair<int, int>(Idx, INum);
      } 
      // We now have an array which tells us from which index of which
      // input vector each element of the operand comes.
      VectorType *I1T = cast<VectorType>(I1->getType());
      unsigned I1Elem = I1T->getNumElements(); 
      if (!I2) {
        // In this case there is only one underlying vector input. Check for
        // the trivial case where we can use the input directly.
        if (I1Elem == numElem) {
          bool ElemInOrder = true;
          for (unsigned i = 0; i < numElem; ++i) {
            if (II[i].first != (int) i && II[i].first != -1) {
              ElemInOrder = false;
              break;
            }
          } 
          if (ElemInOrder)
            return I1;
        } 
        // A shuffle is needed.
        std::vector<Constant *> Mask(numElem);
        for (unsigned i = 0; i < numElem; ++i) {
          int Idx = II[i].first;
          if (Idx == -1)
            Mask[i] = UndefValue::get(Type::getInt32Ty(Context));
          else
            Mask[i] = ConstantInt::get(Type::getInt32Ty(Context), Idx);
        } 
        Instruction *S = new ShuffleVectorInst(I1, UndefValue::get(I1T), ConstantVector::get(Mask),
                                getReplacementName(IBeforeJ ? I : J, true, o));
        S->insertBefore(IBeforeJ ? J : I);
        return S;
      }

      VectorType *I2T = cast<VectorType>(I2->getType());
      unsigned I2Elem = I2T->getNumElements();

      // This input comes from two distinct vectors. The first step is to
      // make sure that both vectors are the same length. If not, the
      // smaller one will need to grow before they can be shuffled together.
      if (I1Elem < I2Elem) {
        std::vector<Constant *> Mask(I2Elem);
        unsigned v = 0;
        for (; v < I1Elem; ++v)
          Mask[v] = ConstantInt::get(Type::getInt32Ty(Context), v);
        for (; v < I2Elem; ++v)
          Mask[v] = UndefValue::get(Type::getInt32Ty(Context)); 
        Instruction *NewI1 = new ShuffleVectorInst(I1, UndefValue::get(I1T), ConstantVector::get(Mask),
                                getReplacementName(IBeforeJ ? I : J, true, o, 1));
        NewI1->insertBefore(IBeforeJ ? J : I);
        I1 = NewI1;
        I1T = I2T;
        I1Elem = I2Elem;
      } else if (I1Elem > I2Elem) {
        std::vector<Constant *> Mask(I1Elem);
        unsigned v = 0;
        for (; v < I2Elem; ++v)
          Mask[v] = ConstantInt::get(Type::getInt32Ty(Context), v);
        for (; v < I1Elem; ++v)
          Mask[v] = UndefValue::get(Type::getInt32Ty(Context)); 
        Instruction *NewI2 = new ShuffleVectorInst(I2, UndefValue::get(I2T), ConstantVector::get(Mask),
                                getReplacementName(IBeforeJ ? I : J, true, o, 1));
        NewI2->insertBefore(IBeforeJ ? J : I);
        I2 = NewI2;
        I2T = I1T;
        I2Elem = I1Elem;
      } 
      // Now that both I1 and I2 are the same length we can shuffle them // together (and use the result).
      std::vector<Constant *> Mask(numElem);
      for (unsigned v = 0; v < numElem; ++v) {
        if (II[v].first == -1) {
          Mask[v] = UndefValue::get(Type::getInt32Ty(Context));
        } else {
          int Idx = II[v].first + II[v].second * I1Elem;
          Mask[v] = ConstantInt::get(Type::getInt32Ty(Context), Idx);
        }
      } 
      Instruction *NewOp = new ShuffleVectorInst(I1, I2, ConstantVector::get(Mask),
                              getReplacementName(IBeforeJ ? I : J, true, o));
      NewOp->insertBefore(IBeforeJ ? J : I);
      return NewOp;
    }
  } 
  Type *ArgType = ArgTypeL;
  if (numElemL < numElemH) {
    if (numElemL == 1 && expandIEChain(Context, I, J, o, HOp, numElemH, ArgTypeL, VArgType, IBeforeJ, 1)) {
      // This is another short-circuit case: we're combining a scalar into
      // a vector that is formed by an IE chain. We've just expanded the IE
      // chain, now insert the scalar and we're done.  
      Instruction *S = InsertElementInst::Create(HOp, LOp, CV0, getReplacementName(IBeforeJ ? I : J, true, o));
      S->insertBefore(IBeforeJ ? J : I);
      return S;
    } else if (!expandIEChain(Context, I, J, o, LOp, numElemL, ArgTypeL, ArgTypeH, IBeforeJ)) {
      // The two vector inputs to the shuffle must be the same length,
      // so extend the smaller vector to be the same length as the larger one.
      Instruction *NLOp;
      if (numElemL > 1) { 
        std::vector<Constant *> Mask(numElemH);
        unsigned v = 0;
        for (; v < numElemL; ++v)
          Mask[v] = ConstantInt::get(Type::getInt32Ty(Context), v);
        for (; v < numElemH; ++v)
          Mask[v] = UndefValue::get(Type::getInt32Ty(Context)); 
        NLOp = new ShuffleVectorInst(LOp, UndefValue::get(ArgTypeL), ConstantVector::get(Mask),
                                     getReplacementName(IBeforeJ ? I : J, true, o, 1));
      } else {
        NLOp = InsertElementInst::Create(UndefValue::get(ArgTypeH), LOp, CV0,
                                         getReplacementName(IBeforeJ ? I : J, true, o, 1));
      } 
      NLOp->insertBefore(IBeforeJ ? J : I);
      LOp = NLOp;
    } 
    ArgType = ArgTypeH;
  } else if (numElemL > numElemH) {
    if (numElemH == 1 && expandIEChain(Context, I, J, o, LOp, numElemL, ArgTypeH, VArgType, IBeforeJ)) {
      Instruction *S = InsertElementInst::Create(LOp, HOp, ConstantInt::get(Type::getInt32Ty(Context), numElemL),
                                  getReplacementName(IBeforeJ ? I : J, true, o));
      S->insertBefore(IBeforeJ ? J : I);
      return S;
    } else if (!expandIEChain(Context, I, J, o, HOp, numElemH, ArgTypeH, ArgTypeL, IBeforeJ)) {
      Instruction *NHOp;
      if (numElemH > 1) {
        std::vector<Constant *> Mask(numElemL);
        unsigned v = 0;
        for (; v < numElemH; ++v)
          Mask[v] = ConstantInt::get(Type::getInt32Ty(Context), v);
        for (; v < numElemL; ++v)
          Mask[v] = UndefValue::get(Type::getInt32Ty(Context)); 
        NHOp = new ShuffleVectorInst(HOp, UndefValue::get(ArgTypeH), ConstantVector::get(Mask),
                                     getReplacementName(IBeforeJ ? I : J, true, o, 1));
      } else {
        NHOp = InsertElementInst::Create(UndefValue::get(ArgTypeL), HOp, CV0,
                                         getReplacementName(IBeforeJ ? I : J, true, o, 1));
      } 
      NHOp->insertBefore(IBeforeJ ? J : I);
      HOp = NHOp;
    }
  } 
  if (ArgType->isVectorTy()) {
    unsigned numElem = VArgType->getVectorNumElements();
    std::vector<Constant*> Mask(numElem);
    for (unsigned v = 0; v < numElem; ++v) {
      unsigned Idx = v;
      // If the low vector was expanded, we need to skip the extra
      // undefined entries.
      if (v >= numElemL && numElemH > numElemL)
        Idx += (numElemH - numElemL);
      Mask[v] = ConstantInt::get(Type::getInt32Ty(Context), Idx);
    } 
    Instruction *BV = new ShuffleVectorInst(LOp, HOp, ConstantVector::get(Mask), getReplacementName(IBeforeJ ? I : J, true, o));
    BV->insertBefore(IBeforeJ ? J : I);
    return BV;
  } 
  Instruction *BV1 = InsertElementInst::Create( UndefValue::get(VArgType), LOp, CV0, getReplacementName(IBeforeJ ? I : J, true, o, 1));
  BV1->insertBefore(IBeforeJ ? J : I);
  Instruction *BV2 = InsertElementInst::Create(BV1, HOp, CV1, getReplacementName(IBeforeJ ? I : J, true, o, 2));
  BV2->insertBefore(IBeforeJ ? J : I);
  return BV2;
}

// This function creates an array of values that will be used as the inputs // to the vector instruction that fuses I with J.
void BBVectorize::getReplacementInputsForPair(LLVMContext& Context, Instruction *I, Instruction *J,
                   SmallVectorImpl<Value *> &ReplacedOperands, bool IBeforeJ)
{
  unsigned NumOperands = I->getNumOperands(); 
  for (unsigned p = 0, o = NumOperands-1; p < NumOperands; ++p, --o) {
    // Iterate backward so that we look at the store pointer // first and know whether or not we need to flip the inputs.  
    if (isa<LoadInst>(I) || (o == 1 && isa<StoreInst>(I))) { // This is the pointer for a load/store instruction.
      ReplacedOperands[o] = getReplacementPointerInput(Context, I, J, o);
      continue;
    } else if (isa<CallInst>(I)) {
      Function *F = cast<CallInst>(I)->getCalledFunction();
      Intrinsic::ID IID = (Intrinsic::ID) F->getIntrinsicID();
      if (o == NumOperands-1) {
        BasicBlock &BB = *I->getParent(); 
        Module *M = BB.getParent()->getParent();
        Type *ArgTypeI = I->getType();
        Type *ArgTypeJ = J->getType();
        Type *VArgType = getVecTypeForPair(ArgTypeI, ArgTypeJ); 
        ReplacedOperands[o] = Intrinsic::getDeclaration(M, IID, VArgType);
        continue;
      } else if (IID == Intrinsic::powi && o == 1) {
        // The second argument of powi is a single integer and we've already
        // checked that both arguments are equal. As a result, we just keep // I's second argument.
        ReplacedOperands[o] = I->getOperand(o);
        continue;
      }
    } else if (isa<ShuffleVectorInst>(I) && o == NumOperands-1) {
      ReplacedOperands[o] = getReplacementShuffleMask(Context, I, J);
      continue;
    } 
    ReplacedOperands[o] = getReplacementInput(Context, I, J, o, IBeforeJ);
  }
}

// This function creates two values that represent the outputs of the
// original I and J instructions. These are generally vector shuffles
// or extracts. In many cases, these will end up being unused and, thus, // eliminated by later passes.
void BBVectorize::replaceOutputsOfPair(LLVMContext& Context, Instruction *I,
                   Instruction *J, Instruction *K, Instruction *&InsertionPt, Instruction *&K1, Instruction *&K2)
{
  if (isa<StoreInst>(I)) {
    //AA->replaceWithNewValue(I, K);
    //AA->replaceWithNewValue(J, K);
  } else {
    Type *IType = I->getType();
    Type *JType = J->getType(); 
    VectorType *VType = getVecTypeForPair(IType, JType);
    unsigned numElem = VType->getNumElements(); 
    unsigned numElemI = getNumScalarElements(IType);
    unsigned numElemJ = getNumScalarElements(JType); 
    if (IType->isVectorTy()) {
      std::vector<Constant*> Mask1(numElemI), Mask2(numElemI);
      for (unsigned v = 0; v < numElemI; ++v) {
        Mask1[v] = ConstantInt::get(Type::getInt32Ty(Context), v);
        Mask2[v] = ConstantInt::get(Type::getInt32Ty(Context), numElemJ+v);
      } 
      K1 = new ShuffleVectorInst(K, UndefValue::get(VType), ConstantVector::get( Mask1), getReplacementName(K, false, 1));
    } else {
      Value *CV0 = ConstantInt::get(Type::getInt32Ty(Context), 0);
      K1 = ExtractElementInst::Create(K, CV0, getReplacementName(K, false, 1));
    } 
    if (JType->isVectorTy()) {
      std::vector<Constant*> Mask1(numElemJ), Mask2(numElemJ);
      for (unsigned v = 0; v < numElemJ; ++v) {
        Mask1[v] = ConstantInt::get(Type::getInt32Ty(Context), v);
        Mask2[v] = ConstantInt::get(Type::getInt32Ty(Context), numElemI+v);
      } 
      K2 = new ShuffleVectorInst(K, UndefValue::get(VType), ConstantVector::get( Mask2), getReplacementName(K, false, 2));
    } else {
      Value *CV1 = ConstantInt::get(Type::getInt32Ty(Context), numElem-1);
      K2 = ExtractElementInst::Create(K, CV1, getReplacementName(K, false, 2));
    } 
    K1->insertAfter(K);
    K2->insertAfter(K1);
    InsertionPt = K2;
  }
}

// Move all uses of the function I (including pairing-induced uses) after J.
void BBVectorize::moveUsesOfIAfterJ(BasicBlock &BB, Instruction *&InsertionPt, Instruction *I, Instruction *J)
{
  // Skip to the first instruction past I.
  BasicBlock::iterator L = llvm::next(BasicBlock::iterator(I)); 
  DenseSet<Value *> Users;
  for (; cast<Instruction>(L) != J;) {
      // Move this instruction
      Instruction *InstToMove = L; ++L; 
      DEBUG(dbgs() << "BBV: moving: " << *InstToMove << " to after " << *InsertionPt << "\n");
      InstToMove->removeFromParent();
      InstToMove->insertAfter(InsertionPt);
      InsertionPt = InstToMove;
  }
}

// Collect all load instruction that are in the move set of a given first
// pair member.  These loads depend on the first instruction, I, and so need
// to be moved after J (the second instruction) when the pair is fused.
void BBVectorize::collectPairLoadMoveSet(BasicBlock &BB, DenseMap<Value *, Value *> &ChosenPairs,
       DenseMap<Value *, std::vector<Value *> > &LoadMoveSet, DenseSet<ValuePair> &LoadMoveSetPairs, Instruction *I)
{
  // Skip to the first instruction past I.
  BasicBlock::iterator L = llvm::next(BasicBlock::iterator(I)); 
  DenseSet<Value *> Users;
  //AliasSetTracker WriteSet(*AA);
  //if (I->mayWriteToMemory()) WriteSet.add(I); 
  // Note: We cannot end the loop when we reach J because J could be moved
  // farther down the use chain by another instruction pairing. Also, J
  // could be before I if this is an inverted input.
  for (BasicBlock::iterator E = BB.end(); cast<Instruction>(L) != E; ++L) {
    //if (trackUsesOfI(Users, WriteSet, I, L)) {
      if (L->mayReadFromMemory()) {
        LoadMoveSet[L].push_back(I);
        LoadMoveSetPairs.insert(ValuePair(L, I));
      }
    //}
  }
}

// In cases where both load/stores and the computation of their pointers
// are chosen for vectorization, we can end up in a situation where the
// aliasing analysis starts returning different query results as the
// process of fusing instruction pairs continues. Because the algorithm
// relies on finding the same use dags here as were found earlier, we'll
// need to precompute the necessary aliasing information here and then
// manually update it during the fusion process.
void BBVectorize::collectLoadMoveSet(BasicBlock &BB, std::vector<Value *> &PairableInsts,
                   DenseMap<Value *, Value *> &ChosenPairs, DenseMap<Value *, std::vector<Value *> > &LoadMoveSet,
                   DenseSet<ValuePair> &LoadMoveSetPairs)
{
  for (std::vector<Value *>::iterator PI = PairableInsts.begin(), PIE = PairableInsts.end(); PI != PIE; ++PI) {
    DenseMap<Value *, Value *>::iterator P = ChosenPairs.find(*PI);
    if (P == ChosenPairs.end()) continue; 
    Instruction *I = cast<Instruction>(P->first);
    collectPairLoadMoveSet(BB, ChosenPairs, LoadMoveSet, LoadMoveSetPairs, I);
  }
}

// When the first instruction in each pair is cloned, it will inherit its
// parent's metadata. This metadata must be combined with that of the other
// instruction in a safe way.
void BBVectorize::combineMetadata(Instruction *K, const Instruction *J)
{
  SmallVector<std::pair<unsigned, MDNode*>, 4> Metadata;
  K->getAllMetadataOtherThanDebugLoc(Metadata);
  for (unsigned i = 0, n = Metadata.size(); i < n; ++i) {
    unsigned Kind = Metadata[i].first;
    MDNode *JMD = J->getMetadata(Kind);
    MDNode *KMD = Metadata[i].second; 
    switch (Kind) {
    default:
      K->setMetadata(Kind, 0); // Remove unknown metadata
      break;
    case LLVMContext::MD_tbaa:
      K->setMetadata(Kind, MDNode::getMostGenericTBAA(JMD, KMD));
      break;
    case LLVMContext::MD_fpmath:
      K->setMetadata(Kind, MDNode::getMostGenericFPMath(JMD, KMD));
      break;
    }
  }
}

// This function fuses the chosen instruction pairs into vector instructions,
// taking care preserve any needed scalar outputs and, then, it reorders the
// remaining instructions as needed (users of the first member of the pair
// need to be moved to after the location of the second member of the pair
// because the vector instruction is inserted in the location of the pair's // second member).
void BBVectorize::fuseChosenPairs(BasicBlock &BB, std::vector<Value *> &PairableInsts,
           DenseMap<Value *, Value *> &ChosenPairs, DenseSet<ValuePair> &FixedOrderPairs,
           DenseMap<VPPair, unsigned> &PairConnectionTypes, DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairs,
           DenseMap<ValuePair, std::vector<ValuePair> > &ConnectedPairDeps)
{
  LLVMContext& Context = BB.getContext(); 
  // During the vectorization process, the order of the pairs to be fused
  // could be flipped. So we'll add each pair, flipped, into the ChosenPairs
  // list. After a pair is fused, the flipped pair is removed from the list.
  DenseSet<ValuePair> FlippedPairs;
  for (DenseMap<Value *, Value *>::iterator P = ChosenPairs.begin(), E = ChosenPairs.end(); P != E; ++P)
    FlippedPairs.insert(ValuePair(P->second, P->first));
  for (DenseSet<ValuePair>::iterator P = FlippedPairs.begin(), E = FlippedPairs.end(); P != E; ++P)
    ChosenPairs.insert(*P); 
  DenseMap<Value *, std::vector<Value *> > LoadMoveSet;
  DenseSet<ValuePair> LoadMoveSetPairs;
  collectLoadMoveSet(BB, PairableInsts, ChosenPairs, LoadMoveSet, LoadMoveSetPairs); 
  DEBUG(dbgs() << "BBV: initial: \n" << BB << "\n"); 
  for (BasicBlock::iterator PI = BB.getFirstInsertionPt(); PI != BB.end();) {
    DenseMap<Value *, Value *>::iterator P = ChosenPairs.find(PI);
    if (P == ChosenPairs.end()) {
      ++PI;
      continue;
    } 
    if (getDepthFactor(P->first) == 0) {
      // These instructions are not really fused, but are tracked as though
      // they are. Any case in which it would be interesting to fuse them
      // will be taken care of by InstCombine.
      //--NumFusedOps;
      ++PI;
      continue;
    } 
    Instruction *I = cast<Instruction>(P->first), *J = cast<Instruction>(P->second); 
    DEBUG(dbgs() << "BBV: fusing: " << *I << " <-> " << *J << "\n"); 
    // Remove the pair and flipped pair from the list.
    DenseMap<Value *, Value *>::iterator FP = ChosenPairs.find(P->second);
    assert(FP != ChosenPairs.end() && "Flipped pair not found in list");
    ChosenPairs.erase(FP);
    ChosenPairs.erase(P);
    bool FlipPairOrder = FixedOrderPairs.count(ValuePair(J, I));
    if (!FlipPairOrder && !FixedOrderPairs.count(ValuePair(I, J))) {
      // This pair does not have a fixed order, and so we might want to
      // flip it if that will yield fewer shuffles. We count the number
      // of dependencies connected via swaps, and those directly connected,
      // and flip the order if the number of swaps is greater.
      bool OrigOrder = true;
      DenseMap<ValuePair, std::vector<ValuePair> >::iterator IJ = ConnectedPairDeps.find(ValuePair(I, J));
      if (IJ == ConnectedPairDeps.end()) {
        IJ = ConnectedPairDeps.find(ValuePair(J, I));
        OrigOrder = false;
      } 
      if (IJ != ConnectedPairDeps.end()) {
        unsigned NumDepsDirect = 0, NumDepsSwap = 0;
        for (std::vector<ValuePair>::iterator T = IJ->second.begin(), TE = IJ->second.end(); T != TE; ++T) {
          VPPair Q(IJ->first, *T);
          DenseMap<VPPair, unsigned>::iterator R = PairConnectionTypes.find(VPPair(Q.second, Q.first));
          assert(R != PairConnectionTypes.end() && "Cannot find pair connection type");
          if (R->second == PairConnectionDirect)
            ++NumDepsDirect;
          else if (R->second == PairConnectionSwap)
            ++NumDepsSwap;
        }

        if (!OrigOrder)
          std::swap(NumDepsDirect, NumDepsSwap);

        if (NumDepsSwap > NumDepsDirect) {
          FlipPairOrder = true;
          DEBUG(dbgs() << "BBV: reordering pair: " << *I << " <-> " << *J << "\n");
        }
      }
    } 
    Instruction *L = I, *H = J;
    if (FlipPairOrder)
      std::swap(H, L);

    // If the pair being fused uses the opposite order from that in the pair
    // connection map, then we need to flip the types.
    DenseMap<ValuePair, std::vector<ValuePair> >::iterator HL = ConnectedPairs.find(ValuePair(H, L));
    if (HL != ConnectedPairs.end())
      for (std::vector<ValuePair>::iterator T = HL->second.begin(), TE = HL->second.end(); T != TE; ++T) {
        VPPair Q(HL->first, *T);
        DenseMap<VPPair, unsigned>::iterator R = PairConnectionTypes.find(Q);
        assert(R != PairConnectionTypes.end() && "Cannot find pair connection type");
        if (R->second == PairConnectionDirect)
          R->second = PairConnectionSwap;
        else if (R->second == PairConnectionSwap)
          R->second = PairConnectionDirect;
      }

    bool LBeforeH = !FlipPairOrder;
    unsigned NumOperands = I->getNumOperands();
    SmallVector<Value *, 3> ReplacedOperands(NumOperands);
    getReplacementInputsForPair(Context, L, H, ReplacedOperands, LBeforeH); 
    // Make a copy of the original operation, change its type to the vector
    // type and replace its operands with the vector operands.
    Instruction *K = L->clone();
    if (L->hasName())
      K->takeName(L);
    else if (H->hasName())
      K->takeName(H); 
    if (!isa<StoreInst>(K))
      K->mutateType(getVecTypeForPair(L->getType(), H->getType())); 
    combineMetadata(K, H);
    K->intersectOptionalDataWith(H); 
    for (unsigned o = 0; o < NumOperands; ++o)
      K->setOperand(o, ReplacedOperands[o]); 
    K->insertAfter(J); 
    // Instruction insertion point:
    Instruction *InsertionPt = K;
    Instruction *K1 = 0, *K2 = 0;
    replaceOutputsOfPair(Context, L, H, K, InsertionPt, K1, K2);

    // The use dag of the first original instruction must be moved to after
    // the location of the second instruction. The entire use dag of the
    // first instruction is disjoint from the input dag of the second
    // (by definition), and so commutes with it.

    moveUsesOfIAfterJ(BB, InsertionPt, I, J); 
    if (!isa<StoreInst>(I)) {
      L->replaceAllUsesWith(K1);
      H->replaceAllUsesWith(K2);
      //AA->replaceWithNewValue(L, K1);
      //AA->replaceWithNewValue(H, K2);
    }

    // Instructions that may read from memory may be in the load move set.
    // Once an instruction is fused, we no longer need its move set, and so
    // the values of the map never need to be updated. However, when a load
    // is fused, we need to merge the entries from both instructions in the
    // pair in case those instructions were in the move set of some other
    // yet-to-be-fused pair. The loads in question are the keys of the map.
    if (I->mayReadFromMemory()) {
      std::vector<ValuePair> NewSetMembers;
      DenseMap<Value *, std::vector<Value *> >::iterator II = LoadMoveSet.find(I);
      if (II != LoadMoveSet.end())
        for (std::vector<Value *>::iterator N = II->second.begin(), NE = II->second.end(); N != NE; ++N)
          NewSetMembers.push_back(ValuePair(K, *N));
      DenseMap<Value *, std::vector<Value *> >::iterator JJ = LoadMoveSet.find(J);
      if (JJ != LoadMoveSet.end())
        for (std::vector<Value *>::iterator N = JJ->second.begin(), NE = JJ->second.end(); N != NE; ++N)
          NewSetMembers.push_back(ValuePair(K, *N));
      for (std::vector<ValuePair>::iterator A = NewSetMembers.begin(), AE = NewSetMembers.end(); A != AE; ++A) {
        LoadMoveSet[A->first].push_back(A->second);
        LoadMoveSetPairs.insert(*A);
      }
    } 
    // Before removing I, set the iterator to the next instruction.
    PI = llvm::next(BasicBlock::iterator(I));
    if (cast<Instruction>(PI) == J)
      ++PI; 
    SE->forgetValue(I);
    SE->forgetValue(J);
    I->eraseFromParent();
    J->eraseFromParent(); 
    DEBUG(dbgs() << "BBV: block is now: \n" << BB << "\n");
  } 
  DEBUG(dbgs() << "BBV: final: \n" << BB << "\n");
}

char BBVectorize::ID = 0;
#define BBV_NAME "bb-vectorize"
static const char bb_vectorize_name[] = "Basic-Block Vectorization";
INITIALIZE_PASS_BEGIN(BBVectorize, BBV_NAME, bb_vectorize_name, false, false)
//INITIALIZE_AG_DEPENDENCY(AliasAnalysis)
//INITIALIZE_AG_DEPENDENCY(TargetTransformInfo)
//INITIALIZE_PASS_DEPENDENCY(DominatorTree)
//INITIALIZE_PASS_DEPENDENCY(ScalarEvolution)
INITIALIZE_PASS_END(BBVectorize, BBV_NAME, bb_vectorize_name, false, false)
static cl::opt<unsigned> ReqChainDepth("bb-vectorize-req-chain-depth", cl::init(6), cl::Hidden, cl::desc("The required chain depth for vectorization")); 
static cl::opt<unsigned> SearchLimit("bb-vectorize-search-limit", cl::init(400), cl::Hidden, cl::desc("The maximum search distance for instruction pairs")); 
static cl::opt<unsigned> VectorBits("bb-vectorize-vector-bits", cl::init(128), cl::Hidden, cl::desc("The size of the native vector registers")); 
static cl::opt<unsigned> MaxIter("bb-vectorize-max-iter", cl::init(0), cl::Hidden, cl::desc("The maximum number of pairing iterations")); 
static cl::opt<unsigned> MaxInsts("bb-vectorize-max-instr-per-group", cl::init(500), cl::Hidden, cl::desc("The maximum number of pairable instructions per group")); 
static cl::opt<unsigned> MaxPairs("bb-vectorize-max-pairs-per-group", cl::init(3000), cl::Hidden, cl::desc("The maximum number of candidate instruction pairs per group")); 
static cl::opt<bool> NoMath("bb-vectorize-no-math", cl::init(false), cl::Hidden, cl::desc("Don't floating-point math intrinsics")); 
static cl::opt<bool> NoFMA("bb-vectorize-no-fma", cl::init(false), cl::Hidden, cl::desc("Don't the fused-multiply-add intrinsic")); 
static cl::opt<bool> NoSelect("bb-vectorize-no-select", cl::init(false), cl::Hidden, cl::desc("Don't select instructions")); 
static cl::opt<bool> NoCmp("bb-vectorize-no-cmp", cl::init(false), cl::Hidden, cl::desc("Don't comparison instructions")); 

VectorizeConfig::VectorizeConfig()
{
  VectorBits = ::VectorBits;
  VectorizeMath = !::NoMath;
  VectorizeFMA = !::NoFMA;
  VectorizeSelect = !::NoSelect;
  VectorizeCmp = !::NoCmp;
  ReqChainDepth= ::ReqChainDepth;
  SearchLimit = ::SearchLimit;
  MaxInsts = ::MaxInsts;
  MaxPairs = ::MaxPairs;
  MaxIter = ::MaxIter;
}
#endif

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
    //PointerType *PTy = cast<PointerType>(Operand->getType());
    //FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
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
    if (!FTy->isVarArg() && (!RetTy->isPointerTy() || !cast<PointerType>(RetTy)->getElementType()->isFunctionTy())) {
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
      if (operand_list[0].type == OpTypeLocalRef)
          slotarray[operand_list[0].value].ignore_debug_info = 1;
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
      {
      printf("XLAT:           Ret");
      printf(" ret=%d/%d;", globalFunction->getReturnType()->getTypeID(), operand_list_index);
      if (globalFunction->getReturnType()->getTypeID() == Type::IntegerTyID && operand_list_index > 1) {
          operand_list[0].type = OpTypeString;
          operand_list[0].value = (uint64_t)getparam(1);
          sprintf(vout, "%s = %s && %s_enable;", globalName, getparam(1), globalName);
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
      if (operand_list_index >= 3 && operand_list[1].type == OpTypeLocalRef) {
        const char *cp = slotarray[operand_list[1].value].name;
//printf(" name %s;", cp);
        if (!strcmp(cp, "this")) {
          uint64_t **val = globalThis[1+operand_list[3].value];
          const GlobalValue *g = EE->getGlobalValueAtAddress((void *)val);
printf("GGglo %p g %p gcn %s;", globalThis, g, globalClassName);
          if (g)
              ret = g->getName().str().c_str();
          cp = globalClassName;
        }
        if (!ret) {
          sprintf(temp, "%s_%lld", cp, (long long)operand_list[3].value);
          ret = temp;
        }
      }
      if (operand_list[0].type == OpTypeLocalRef && ret)
          slotarray[operand_list[0].value].name = strdup(ret);
      }
      break;

  // Convert instructions...
  case Instruction::Trunc:
      printf("XLAT:         Trunc");
      if (operand_list[0].type == OpTypeLocalRef && operand_list[1].type == OpTypeLocalRef)
          slotarray[operand_list[0].value] = slotarray[operand_list[1].value];
      break;
  case Instruction::ZExt:
      printf("XLAT:          Zext");
      if (operand_list[0].type == OpTypeLocalRef && operand_list[1].type == OpTypeLocalRef)
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
      if (operand_list[0].type == OpTypeLocalRef && operand_list[1].type == OpTypeLocalRef)
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
      }
      break;
  //case Instruction::FCmp:
  case Instruction::PHI:
      printf("XLAT:           PHI");
      break;
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
          printf(" op[%d]L=%lld:%p:%s;", i, (long long)operand_list[i].value, slotarray[operand_list[i].value].value, slotarray[operand_list[i].value].name);
      else if (operand_list[i].type != OpTypeNone)
          printf(" op[%d]=%s;", i, getparam(i));
  }
  printf("\n");
  if (strlen(vout)) {
     print_header();
     fprintf(outputFile, "        %s\n", vout);
  }
}

static void processFunction(Function *F)
{
  globalFunction = F;
  already_printed_header = 0;
  FunctionType *FT = F->getFunctionType();
  int updateFlag = strlen(globalName) > 8 && !strcmp(globalName + strlen(globalName) - 8, "updateEv");
  char temp[MAX_CHAR_BUFFER];
  strcpy(temp, globalName);
  if (updateFlag) {
      print_header();
      strcat(temp + strlen(globalName) - 8, "guardEv");
      fprintf(outputFile, "if (%s) then begin\n", temp);
  }
  if (!F->isDeclaration()) {
    for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I) {
      optimize_block_item->runOnBasicBlock(*I);
      for (BasicBlock::const_iterator ins = I->begin(), ins_end = I->end(); ins != ins_end; ++ins) {
        printInstruction(*ins);
      }
    }
  }
  clearLocalSlot();
  if (updateFlag) {
      print_header();
      fprintf(outputFile, "end;\n");
  }
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
    errs() << " [line " << DT.getLineNumber() << ", size " << DT.getSizeInBits() << ", align " << DT.getAlignInBits() << ", offset " << DT.getOffsetInBits();
    if (DT.isBasicType())
      if (const char *Enc = dwarf::AttributeEncodingString(DIBasicType(DT).getEncoding()))
        errs() << ", enc " << Enc;
    errs() << "]";
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
      return 1;
    }

  //ModulePass *DebugIRPass = createDebugIRPass();
  //DebugIRPass->runOnModule(*Mod);
  //peepPass->runOnModule(*Mod);
static const VectorizeConfig C;
  optimize_block_item = new BBVectorize(C);
//printf("[%s:%d]\n", __FUNCTION__, __LINE__); exit(1);

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
