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

#include <set>
#include "llvm/DebugInfo.h"
#include "llvm/InstVisitor.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/FormattedStream.h"

#define SEPARATOR ":"

#define MAX_BASIC_BLOCK_FLAGS 0x10
#define MAX_SLOTARRAY 1000
#define MAX_OPERAND_LIST 200
#define MAX_CHAR_BUFFER 1000
#define MAX_CLASS_DEFS  200
#define MAX_VTAB_EXTRA 100

class SLOTARRAY_TYPE {
public:
    const char *name;
    uint8_t *svalue;
    uint64_t offset;
    SLOTARRAY_TYPE() {
        name = NULL;
        svalue = NULL;
        offset = 0;
    }
};
class MAPTYPE_WORK {
public:
    int derived;
    const MDNode *CTy;
    char *addr;
    std::string aname;
    MAPTYPE_WORK(int a, const MDNode *b, char *c, std::string d) {
       derived = a;
       CTy = b;
       addr = c;
       aname = d;
    }
};

class VTABLE_WORK {
public:
    int f;
    Function ***thisp;
    SLOTARRAY_TYPE arg;
    VTABLE_WORK(int a, Function ***b, SLOTARRAY_TYPE c) {
       f = a;
       thisp = b;
       arg = c;
    }
};

typedef struct {
    const char   *name;
    const MDNode *node;
    const MDNode *inherit;
    int           member_count;
    std::list<const MDNode *> memberl;
} CLASS_META;

typedef struct {
   int type;
   uint64_t value;
} OPERAND_ITEM_TYPE;

enum {OpTypeNone, OpTypeInt, OpTypeLocalRef, OpTypeExternalFunction, OpTypeString};

enum SpecialGlobalClass { NotSpecial = 0, GlobalCtors, GlobalDtors, NotPrinted };
class CWriter : public FunctionPass, public InstVisitor<CWriter> {
    raw_fd_ostream &Out;
    raw_fd_ostream &OutHeader;
    std::map<const ConstantFP *, unsigned> FPConstantMap;
    DenseMap<const Value*, unsigned> AnonValueNumbers;
    unsigned NextAnonValueNumber;
    DenseMap<StructType*, unsigned> UnnamedStructIDs;
    unsigned NextTypeID;
  public:
    static char ID;
    explicit CWriter(raw_fd_ostream &o, raw_fd_ostream &oh)
      : FunctionPass(ID), Out(o), OutHeader(oh), NextAnonValueNumber(0), NextTypeID(0) { }
    virtual const char *getPassName() const { return "C backend"; }
    virtual bool doInitialization(Module &M);
    bool runOnFunction(Function &F) {
      if (!F.isDeclaration() && F.getName() != "_Z16run_main_programv" && F.getName() != "main")
          printFunction(F);
      return false;
    }
    virtual bool doFinalization(Module &M) {
      flushStruct();
      FPConstantMap.clear();
      UnnamedStructIDs.clear();
      return false;
    }
    raw_ostream &printType(raw_ostream &Out, Type *Ty, bool isSigned = false, const std::string &VariableName = "", bool IgnoreName = false, const AttributeSet &PAL = AttributeSet());
    raw_ostream &printSimpleType(raw_ostream &Out, Type *Ty, bool isSigned, const std::string &NameSoFar = "");
    std::string getStructName(StructType *ST);
    void writeOperand(Value *Operand, bool Indirect, bool Static = false);
    void writeInstComputationInline(Instruction &I);
    void writeOperandInternal(Value *Operand, bool Static = false);
    void writeOperandWithCast(Value* Operand, unsigned Opcode);
    void writeOperandWithCast(Value* Operand, const ICmpInst &I);
    bool writeInstructionCast(const Instruction &I);
    void writeMemoryAccess(Value *Operand, Type *OperandType, bool IsVolatile, unsigned Alignment);
  private :
    void printContainedStructs(Type *Ty);
    void printFunctionSignature(const Function *F, bool Prototype);
    void printFunction(Function &);
    void printBasicBlock(BasicBlock *BB);
    void printCast(unsigned opcode, Type *SrcTy, Type *DstTy);
    void printConstant(Constant *CPV, bool Static);
    void printConstantWithCast(Constant *CPV, unsigned Opcode);
    bool printConstExprCast(const ConstantExpr *CE, bool Static);
    void printConstantArray(ConstantArray *CPA, bool Static);
    void printConstantVector(ConstantVector *CV, bool Static);
    void flushStruct(void);
    bool isAddressExposed(const Value *V) const {
      //if (const Argument *A = dyn_cast<Argument>(V))
        //return ByValParams.count(A);
      return isa<GlobalVariable>(V) || isDirectAlloca(V);
    }
    static bool isInlinableInst(const Instruction &I) {
      if (isa<CmpInst>(I))
        return true;
      if (I.getType() == Type::getVoidTy(I.getContext()) || !I.hasOneUse() ||
          isa<TerminatorInst>(I) || isa<CallInst>(I) || isa<PHINode>(I) ||
          isa<LoadInst>(I) || isa<VAArgInst>(I) || isa<InsertElementInst>(I) ||
          isa<InsertValueInst>(I))
        return false;
      if (I.hasOneUse()) {
        const Instruction &User = cast<Instruction>(*I.use_back());
        if (isa<ExtractElementInst>(User) || isa<ShuffleVectorInst>(User))
          return false;
      }
      return I.getParent() == cast<Instruction>(I.use_back())->getParent();
    }
    static const AllocaInst *isDirectAlloca(const Value *V) {
      const AllocaInst *AI = dyn_cast<AllocaInst>(V);
      if (!AI) return 0;
      if (AI->isArrayAllocation())
        return 0;   // FIXME: we can also inline fixed size array allocas!
      if (AI->getParent() != &AI->getParent()->getParent()->getEntryBlock())
        return 0;
      return AI;
    }
    friend class InstVisitor<CWriter>;
    void visitReturnInst(ReturnInst &I);
    void visitBranchInst(BranchInst &I);
    void visitIndirectBrInst(IndirectBrInst &I);
    void visitUnreachableInst(UnreachableInst &I);
    void visitPHINode(PHINode &I);
    void visitBinaryOperator(Instruction &I);
    void visitICmpInst(ICmpInst &I);
    void visitCastInst (CastInst &I);
    void visitCallInst (CallInst &I);
    bool visitBuiltinCall(CallInst &I, Intrinsic::ID ID, bool &WroteCallee);
    void visitAllocaInst(AllocaInst &I);
    void visitLoadInst  (LoadInst   &I);
    void visitStoreInst (StoreInst  &I);
    void visitGetElementPtrInst(GetElementPtrInst &I);
    void visitInstruction(Instruction &I) {
      errs() << "C Writer does not know about " << I;
      llvm_unreachable(0);
    }
    bool isGotoCodeNecessary(BasicBlock *From, BasicBlock *To);
    void printPHICopiesForSuccessor(BasicBlock *CurBlock, BasicBlock *Successor, unsigned Indent);
    void printGEPExpression(Value *Ptr, gep_type_iterator I, gep_type_iterator E, bool Static);
    std::string GetValueName(const Value *Operand);
};

class RemoveAllocaPass : public BasicBlockPass {
  public:
    static char ID;
    RemoveAllocaPass() : BasicBlockPass(ID) {}
    bool runOnBasicBlock(BasicBlock &BB);
};

class CallProcessPass : public BasicBlockPass {
  public:
    static char ID;
    CallProcessPass() : BasicBlockPass(ID) {}
    bool runOnBasicBlock(BasicBlock &BB);
};

class GeneratePass : public ModulePass {
  public:
    static char ID;
    GeneratePass() : ModulePass(ID) {}
    bool runOnModule(Module &M);
};

extern ExecutionEngine *EE;
extern int trace_translate;
extern int trace_full;
extern const char *globalName;

extern std::list<VTABLE_WORK> vtablework;
extern SLOTARRAY_TYPE slotarray[MAX_SLOTARRAY];
extern OPERAND_ITEM_TYPE operand_list[MAX_OPERAND_LIST];
extern int operand_list_index;

const MDNode *getNode(const Value *val);
void dumpType(DIType litem, CLASS_META *classp);
void dumpTref(const MDNode *Node, CLASS_META *classp);
void dumpType(DIType litem, CLASS_META *classp);
void process_metadata(NamedMDNode *CU_Nodes);
CLASS_META *lookup_class(const char *cp);
int lookup_method(const char *classname, std::string methodname);
int lookup_field(const char *classname, std::string methodname);

void prepareOperand(const Value *Operand);
const char *getparam(int arg);
int getLocalSlot(const Value *V);

const char *calculateGuardUpdate(Function ***parent_thisp, Instruction &I);
const char *generateVerilog(Function ***thisp, Instruction &I);
bool callProcess_runOnInstruction(Module *Mod, Instruction *II);

int validate_address(int arg, void *p);
const char *map_address(void *arg, std::string name);
void constructAddressMap(NamedMDNode *CU_Nodes);
void adjustModuleSizes(Module *Mod);
