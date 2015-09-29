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
#include <list>
#include <set>
#include "llvm/DebugInfo.h"
#include "llvm/InstVisitor.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/CommandLine.h"

#define SEPARATOR ":"

#define MAX_BASIC_BLOCK_FLAGS 0x10
#define MAX_SLOTARRAY 1000
#define MAX_OPERAND_LIST 200
#define MAX_CHAR_BUFFER 1000
#define MAX_CLASS_DEFS  200
#define MAX_VTAB_EXTRA 100

#define ERRORIF(A) { \
      if(A) { \
          printf("[%s:%d]\n", __FUNCTION__, __LINE__); \
          exit(1); \
      }}

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
class ADDRESSMAP_TYPE {
public:
    const MDNode *type;
    std::string name;
    ADDRESSMAP_TYPE(std::string aname, const MDNode *atype) {
       name = aname;
       type = atype;
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

typedef struct {
    int value;
    const char *name;
} INTMAP_TYPE;

enum {OpTypeNone, OpTypeInt, OpTypeLocalRef, OpTypeExternalFunction, OpTypeString};

class CWriter : public InstVisitor<CWriter> {
  public:
    raw_fd_ostream &Out;
    raw_fd_ostream &OutHeader;
    DenseMap<const Value*, unsigned> AnonValueNumbers;
    unsigned NextAnonValueNumber;
    DenseMap<StructType*, unsigned> UnnamedStructIDs;
    unsigned NextTypeID;
    explicit CWriter(raw_fd_ostream &o, raw_fd_ostream &oh)
      : Out(o), OutHeader(oh), NextAnonValueNumber(0), NextTypeID(1) { }
    bool cwriterModule(Module &M);
  //private :
    void printType(raw_ostream &Out, Type *Ty, bool isSigned, const std::string NameSoFar, const std::string prefix, const std::string postfix);
    std::string getStructName(StructType *STy);
    void writeOperand(Value *Operand, bool Indirect, bool Static = false);
    void writeOperandWithCast(Value* Operand, unsigned Opcode);
    void writeOperandWithCastICmp(Value* Operand, bool shouldCast, bool typeIsSigned);
    bool writeInstructionCast(const Instruction &I);
    void printContainedStructs(Type *Ty);
    void printFunctionSignature(raw_ostream &Out, const Function *F, bool Prototype, const char *postfix);
    void printCast(unsigned opcode, Type *SrcTy, Type *DstTy);
    void printConstant(const char *prefix, Constant *CPV, bool Static);
    void printConstantWithCast(Constant *CPV, unsigned Opcode, const char *postfix);
    bool printConstExprCast(const ConstantExpr *CE);
    void printString(const char *cp, int len);
    void printConstantDataArray(ConstantDataArray *CPA, bool Static);
    void printConstantArray(ConstantArray *CPA, bool Static);
    void printConstantVector(ConstantVector *CV, bool Static);
    bool isAddressExposed(const Value *V) const {
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
      if ( I.getParent() != cast<Instruction>(I.use_back())->getParent()) {
          printf("[%s:%d]\n", __FUNCTION__, __LINE__);
          exit(1);
      }
      return true;
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
    void visitBinaryOperator(Instruction &I);
    void visitICmpInst(ICmpInst &I);
    void visitCastInst (CastInst &I);
    void visitCallInst (CallInst &I);
    void visitLoadInst  (LoadInst   &I);
    void visitStoreInst (StoreInst  &I);
    void visitGetElementPtrInst(GetElementPtrInst &I);
    void visitInstruction(Instruction &I) {
      errs() << "C Writer does not know about " << I;
      llvm_unreachable(0);
    }
    void printGEPExpression(Value *Ptr, gep_type_iterator I, gep_type_iterator E, bool Static);
    std::string GetValueName(const Value *Operand);
    void processCFunction(Function &func);
};

class RemoveAllocaPass : public FunctionPass {
  public:
    static char ID;
    RemoveAllocaPass() : FunctionPass(ID) {}
    bool runOnFunction(Function &F);
};

class CallProcessPass : public BasicBlockPass {
  public:
    static char ID;
    CallProcessPass() : BasicBlockPass(ID) {}
    bool runOnBasicBlock(BasicBlock &BB);
};

class GeneratePass : public ModulePass, public CWriter {
  public:
    static char ID;
    GeneratePass(raw_fd_ostream &o, raw_fd_ostream &oh) : ModulePass(ID), CWriter(o,oh) { }
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
extern INTMAP_TYPE predText[];
extern INTMAP_TYPE opcodeMap[];
extern FILE *outputFile;
extern Function *EntryFn;
extern cl::opt<std::string> MArch;
extern cl::list<std::string> MAttrs;
extern std::list<StructType *> structWork;
extern int structWork_run;

const char *intmapLookup(INTMAP_TYPE *map, int value);
const MDNode *getNode(const Value *val);
void dumpType(DIType litem, CLASS_META *classp);
void dumpTref(const MDNode *Node, CLASS_META *classp);
void dumpType(DIType litem, CLASS_META *classp);
void process_metadata(NamedMDNode *CU_Nodes);
CLASS_META *lookup_class(const char *cp);
int lookup_method(const char *classname, std::string methodname);
int lookup_field(const char *classname, std::string methodname);

void prepareOperand(const Value *Operand);
const char *getParam(int arg);
int getLocalSlot(const Value *V);

const char *calculateGuardUpdate(Function ***parent_thisp, Instruction &I);
const char *generateVerilog(Function ***thisp, Instruction &I);
bool callProcess_runOnInstruction(Module *Mod, Instruction *II);

int validateAddress(int arg, void *p);
const char *mapAddress(void *arg, std::string name, const MDNode *type);
void constructAddressMap(NamedMDNode *CU_Nodes);
void adjustModuleSizes(Module *Mod);
bool endswith(const char *str, const char *suffix);
const char *fieldName(StructType *STy, uint64_t ind);
