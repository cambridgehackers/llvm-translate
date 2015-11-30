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
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"

#define SEPARATOR ":"

#define MAX_BASIC_BLOCK_FLAGS 0x10
#define MAX_CHAR_BUFFER 1000
#define BOGUS_POINTER ((void *)0x5a5a5a5a5a5a5a5a)

#define ERRORIF(A) { \
      if(A) { \
          printf("[%s:%d]\n", __FUNCTION__, __LINE__); \
          exit(1); \
      }}

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
    int value;
    const char *name;
} INTMAP_TYPE;

typedef struct {
    const void *addr;
    const void *type;
} MAPSEEN_TYPE;

struct MAPSEENcomp {
    bool operator() (const MAPSEEN_TYPE& lhs, const MAPSEEN_TYPE& rhs) const {
        if (lhs.addr < rhs.addr)
            return true;
        if (lhs.addr > rhs.addr)
            return false;
        return lhs.type < rhs.type;
    }
};

typedef struct {
    const StructType *STy;
    int               Idx;
} EREPLACE_INFO;

struct EREPLACEcomp {
    bool operator() (const EREPLACE_INFO& lhs, const EREPLACE_INFO& rhs) const {
        if (lhs.STy < rhs.STy)
            return true;
        if (lhs.STy > rhs.STy)
            return false;
        return lhs.Idx < rhs.Idx;
    }
};

class ClassMethodTable {
public:
    std::map<Function *, std::string> method;
    std::list<std::string> rules;
};

typedef struct {
    const char *name;
    void       *thisp;
    Function   *RDY;
    Function   *ENA;
} RULE_INFO;

typedef struct {
    unsigned int maxIndex;
    std::string *methods;
} METHOD_INFO;

typedef void (*GEN_HEADER)(const StructType *STy, FILE *OStr, std::string ODir);

enum {ProcessNone, ProcessHoist, ProcessVerilog, ProcessCPP};

extern ExecutionEngine *EE;
extern int trace_translate;
extern int trace_full;
extern std::string globalName;

extern INTMAP_TYPE predText[];
extern INTMAP_TYPE opcodeMap[];
extern const Function *EntryFn;
extern int regen_methods;
extern unsigned NextTypeID;
extern int generateRegion;
extern Module *globalMod;
extern Function *currentFunction;
extern std::map<std::string, void *> nameMap;
extern std::map<std::string, const Function *> referencedItems;
extern std::map<const StructType *,ClassMethodTable *> classCreate;
extern std::map<const StructType *, METHOD_INFO *> classMethod;
extern std::map<EREPLACE_INFO, const Type *, EREPLACEcomp> replaceType;
extern std::list<RULE_INFO *> ruleInfo;
extern std::map<Function *, Function *> ruleRDYFunction;

int validateAddress(int arg, void *p);
std::string mapAddress(void *arg);
void constructAddressMap(Module *Mod);

const char *intmapLookup(INTMAP_TYPE *map, int value);
std::string fieldName(const StructType *STy, uint64_t ind);
void *mapLookup(std::string name);

void generateCppData(FILE *OStr, Module &Mod);

std::string printType(const Type *Ty, bool isSigned, std::string NameSoFar, std::string prefix, std::string postfix);
std::string printOperand(Function ***thisp, Value *Operand, bool Indirect);
std::string printFunctionSignature(const Function *F, std::string altname, bool Prototype, std::string postfix, int skip);
std::string fetchOperand(Function ***thisp, Value *Operand, bool Indirect);

std::string GetValueName(const Value *Operand);
std::string getStructName(const StructType *STy);
std::string CBEMangle(const std::string &S);
const StructType *findThisArgument(const Function *func);

void processFunction(VTABLE_WORK work, FILE *outputFile, std::string aclassName);
std::string verilogArrRange(const Type *Ty);
void memdump(unsigned char *p, int len, const char *title);
void memdumpl(unsigned char *p, int len, const char *title);
bool call2runOnFunction(Function &F);
bool callMemrunOnFunction(Function &F);
bool GenerateRunOnModule(Module *Mod, std::string OutDirectory);
void mapDwarfType(int derived, const Metadata *aMeta, char *addr, int aoffset, std::string aname);
const Metadata *fetchType(const Metadata *arg);
std::string ucName(std::string inname);
Function *lookup_function(std::string className, std::string methodName);
Instruction *copyFunction(Instruction *TI, const Instruction *I, int methodIndex, Type *returnType);
Instruction *cloneTree(const Instruction *I, Instruction *insertPoint);
void prepareClone(Instruction *TI, const Instruction *I);
std::string printString(std::string arg);
std::string getMethodName(std::string name);
bool endswith(std::string str, std::string suffix);
void generateClassDef(const StructType *STy, FILE *OStr, std::string ODir);
void generateClassBody(const StructType *STy, FILE *OStr, std::string ODir);
void generateModuleDef(const StructType *STy, FILE *OStr, std::string oDir);
void generateModuleSignature(FILE *OStr, const StructType *STy, const char *instance);
