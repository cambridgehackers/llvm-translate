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

#include "llvm/IR/Module.h"
#include "llvm/DebugInfo.h"
#include "llvm/ExecutionEngine/Interpreter.h"

//using namespace llvm;

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
    int value;
    const char *name;
} INTMAP_TYPE;

typedef struct {
   int type;
   uint64_t value;
} OPERAND_ITEM_TYPE;

enum {OpTypeNone, OpTypeInt, OpTypeLocalRef, OpTypeExternalFunction, OpTypeString};

extern ExecutionEngine *EE;
extern int trace_translate;
extern int trace_full;
extern const char *globalName;

extern std::list<VTABLE_WORK> vtablework;
extern char vout[MAX_CHAR_BUFFER];
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
const char *intmap_lookup(INTMAP_TYPE *map, int value);

void calculateGuardUpdate(Function ***parent_thisp, Instruction &I);
void generateVerilog(Function ***thisp, Instruction &I);
