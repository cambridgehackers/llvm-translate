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
#define MAX_CHAR_BUFFER 1000
#define MAX_CLASS_DEFS  200
#define MAX_VTAB_EXTRA 100

#define ERRORIF(A) { \
      if(A) { \
          printf("[%s:%d]\n", __FUNCTION__, __LINE__); \
          exit(1); \
      }}

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
    Function *f;      // Since passes modify instructions, this cannot be 'const'
    Function ***thisp;
    VTABLE_WORK(Function *a, Function ***b) {
       f = a;
       thisp = b;
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
    int value;
    const char *name;
} INTMAP_TYPE;

enum {CastOther, CastUnsigned, CastSigned, CastGEP, CastSExt, CastZExt, CastFPToSI};

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

class GeneratePass : public ModulePass {
    FILE *Out;
    FILE *OutHeader;
    FILE *OutNull;
  public:
    static char ID;
    GeneratePass(FILE *o, FILE *oh, FILE *on) : ModulePass(ID), Out(o), OutHeader(oh), OutNull(on) { }
    bool runOnModule(Module &M);
};
class ClassMethodTable {
public:
    std::string classOrig;
    std::string className;
    std::map<Function *, std::string> method;
    ClassMethodTable(std::string corig, const char *name) : classOrig(corig), className(name) { }
};

extern ExecutionEngine *EE;
extern int trace_translate;
extern int trace_full;
extern const char *globalName;

extern INTMAP_TYPE predText[];
extern INTMAP_TYPE opcodeMap[];
extern FILE *outputFile;
extern const Function *EntryFn;
extern cl::opt<std::string> MArch;
extern cl::list<std::string> MAttrs;
extern unsigned NextAnonValueNumber;
extern std::map<std::string, void *> nameMap;
extern std::map<std::string,ClassMethodTable *> classCreate;
extern std::map<Function *,ClassMethodTable *> functionIndex;
extern int regen_methods;
extern unsigned NextTypeID;
extern DenseMap<const StructType*, unsigned> UnnamedStructIDs;
extern int generateRegion;

const char *intmapLookup(INTMAP_TYPE *map, int value);
const MDNode *getNode(const Value *val);
void dumpType(DIType litem, CLASS_META *classp);
void dumpTref(const MDNode *Node, CLASS_META *classp);
void dumpType(DIType litem, CLASS_META *classp);
void process_metadata(NamedMDNode *CU_Nodes);
CLASS_META *lookup_class(const char *cp);
int lookup_method(const char *classname, std::string methodname);
int lookup_field(const char *classname, std::string methodname);

const char *calculateGuardUpdate(Function ***parent_thisp, Instruction &I);
const char *generateVerilog(Function ***thisp, Instruction &I);
bool callProcess_runOnInstruction(Module *Mod, Instruction *II);

int validateAddress(int arg, void *p);
const char *mapAddress(void *arg, std::string name, const MDNode *type);
void constructAddressMap(NamedMDNode *CU_Nodes);
bool endswith(const char *str, const char *suffix);
void generateCppData(FILE *OStr, Module &Mod);
void generateCppHeader(Module &Mod, FILE *OStr);
const char *processCInstruction(Function ***thisp, Instruction &I);
char *printFunctionSignature(const Function *F, const char *altname, bool Prototype, const char *postfix, int skip);
std::string GetValueName(const Value *Operand);
char *printType(Type *Ty, bool isSigned, std::string NameSoFar, std::string prefix, std::string postfix);
void recursiveDelete(Value *V);
char *writeOperand(Function ***thisp, Value *Operand, bool Indirect);
char *getOperand(Function ***thisp, Value *Operand, bool Indirect);
const char *processInstruction(Function ***thisp, Instruction *ins);
void *mapLookup(std::string name);
void pushWork(Function *func, Function ***thisp);
ClassMethodTable *findClass(std::string tname);
std::string getStructName(const StructType *STy);
std::string CBEMangle(const std::string &S);
CLASS_META *lookup_class_mangle(const char *cp);
void processFunction(VTABLE_WORK &work, const char *newName, FILE *outputFile);
int checkIfRule(Type *aTy);
const char *fieldName(const StructType *STy, uint64_t ind);
int processVar(const GlobalVariable *GV);
char *writeOperandWithCast(Function ***thisp, Value* Operand, unsigned Opcode);
char *writeOperandWithCastICmp(Function ***thisp, Value* Operand, bool shouldCast, bool typeIsSigned);
const char *writeInstructionCast(const Instruction &I);
const char *printCast(unsigned opc, Type *SrcTy, Type *DstTy);
void generateVerilogHeader(Module &Mod, FILE *OStr, FILE *ONull);
void generateModuleDef(const StructType *STy, FILE *OStr);
void generateClassDef(const StructType *STy, FILE *OStr);
