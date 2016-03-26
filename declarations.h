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
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

#define MODULE_SEPARATOR "$"
#define MODULE_ARROW (generateRegion == ProcessVerilog ? MODULE_SEPARATOR : "->")
#define MODULE_DOT   (generateRegion == ProcessVerilog ? MODULE_SEPARATOR : ".")

#define MAX_BASIC_BLOCK_FLAGS 0x10
#define MAX_CHAR_BUFFER 1000
#define BOGUS_POINTER ((void *)0x5a5a5a5a5a5a5a5a)

#define ERRORIF(A) { \
      if(A) { \
          printf("[%s:%d]\n", __FUNCTION__, __LINE__); \
          exit(1); \
      }}

#define GIANT_SIZE 1024

typedef struct {
    int value;
    const char *name;
} INTMAP_TYPE;

typedef struct {
    std::string target;
    std::string source;
    const StructType *STy;
} InterfaceConnectType;

typedef struct {
    std::string       name;
    const StructType *STy;
} InterfaceListType;

#define MAX_MEMBER_SIZE 100
class ClassMethodTable {
public:
    const StructType                  *STy;
    std::map<std::string, Function *> method;
    std::map<std::string, int>        shadow;
    std::map<int, Type *>             replaceType;
    std::map<int, uint64_t>           replaceCount;
    std::map<int, bool>               allocateLocally;
    std::map<std::string, Function *> rules;
    std::list<InterfaceConnectType>   interfaceConnect;
    unsigned int                      vtableCount;
    std::string                       instance;
    Function                          *vtable[MAX_MEMBER_SIZE];
    std::map<std::string, Type *>     interfaces;
    std::list<InterfaceListType>      interfaceList;
    ClassMethodTable(): vtableCount(0)//static for now, vtable(NULL)
    {}
};

typedef struct {
    BasicBlock *cond;
    std::string item;
} ReferenceType;
typedef struct {
    std::string target;
    BasicBlock *cond;
    std::string item;
} StoreType;
typedef std::map<std::string,std::list<Value *>> MetaRef;

typedef  struct {
    void *p;
    size_t size;
    Type *type;
    const StructType *STy;
    uint64_t   vecCount;
} MEMORY_REGION;

enum {ProcessNone=0, ProcessVerilog, ProcessCPP};

extern ExecutionEngine *EE;
extern std::map<const StructType *,ClassMethodTable *> classCreate;
extern std::map<Function *, Function *> ruleRDYFunction;
extern MetaRef readList, writeList, invokeList;
extern std::list<ReferenceType> functionList;
extern std::map<std::string, std::string> declareList;
extern std::list<StoreType> storeList;
extern std::map<const Function *, std::string> pushSeen;
extern std::list<MEMORY_REGION> memoryRegion;
extern std::list<Function *> fixupFuncList;
extern int trace_pair;
extern Module *globalMod;

int validateAddress(int arg, void *p);
void constructAddressMap(Module *Mod);
std::string fieldName(const StructType *STy, uint64_t ind);
std::string printType(Type *Ty, bool isSigned, std::string NameSoFar, std::string prefix, std::string postfix, bool ptr);
std::string printOperand(Value *Operand, bool Indirect);
std::string getStructName(const StructType *STy);
std::string CBEMangle(const std::string &S);
void processFunction(Function *func);
std::string verilogArrRange(Type *Ty);
void memdump(unsigned char *p, int len, const char *title);
void memdumpl(unsigned char *p, int len, const char *title);
bool GenerateRunOnModule(Module *Mod, std::string OutDirectory);
const Metadata *fetchType(const Metadata *arg);
std::string ucName(std::string inname);
Function *lookup_function(std::string className, std::string methodName);
Instruction *cloneTree(const Instruction *I, Instruction *insertPoint);
void prepareClone(Instruction *TI, const Instruction *I);
std::string printString(std::string arg);
std::string getMethodName(std::string name);
bool endswith(std::string str, std::string suffix);
void generateClassDef(const StructType *STy, std::string oDir);
void generateModuleDef(const StructType *STy, std::string oDir);
void generateModuleSignature(FILE *OStr, const StructType *STy, std::string instance);
const StructType *findThisArgument(Function *func);
std::string lookupMethodName(const ClassMethodTable *table, int ind);
void preprocessModule(Module *Mod);
std::string GetValueName(const Value *Operand);
int inheritsModule(const StructType *STy, const char *name);
void muxEnable(BasicBlock *bb, std::string signal);
void muxValue(BasicBlock *bb, std::string signal, std::string value);
Value *getCondition(BasicBlock *bb, int invert);
int64_t getGEPOffset(VectorType **LastIndexIsVector, gep_type_iterator I, gep_type_iterator E);
void prepareReplace(const Value *olda, Value *newa);
void setCondition(BasicBlock *bb, int invert, Value *val);
void setAssign(std::string target, std::string value);
void recursiveDelete(Value *V);
int checkDerived(const Type *A, const Type *B);
void setSeen(Function *func, std::string mName);
void dumpMemoryRegions(int arg);
void pushPair(Function *enaFunc, std::string enaName, Function *rdyFunc, std::string rdyName);
void generateContainedStructs(const Type *Ty, std::string ODir);
void appendList(MetaRef &list, BasicBlock *cond, std::string item);
void gatherMeta(std::string mname, std::list<std::string> &metaList);
