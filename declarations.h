
#define SEPARATOR ":"

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
    DICompositeType CTy;
    char *addr;
    std::string aname;
    MAPTYPE_WORK(int a, DICompositeType b, char *c, std::string d) {
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

enum {OpTypeNone, OpTypeInt, OpTypeLocalRef, OpTypeExternalFunction, OpTypeString};

static INTMAP_TYPE predText[] = {
    {FCmpInst::FCMP_FALSE, "false"}, {FCmpInst::FCMP_OEQ, "oeq"},
    {FCmpInst::FCMP_OGT, "ogt"}, {FCmpInst::FCMP_OGE, "oge"},
    {FCmpInst::FCMP_OLT, "olt"}, {FCmpInst::FCMP_OLE, "ole"},
    {FCmpInst::FCMP_ONE, "one"}, {FCmpInst::FCMP_ORD, "ord"},
    {FCmpInst::FCMP_UNO, "uno"}, {FCmpInst::FCMP_UEQ, "ueq"},
    {FCmpInst::FCMP_UGT, "ugt"}, {FCmpInst::FCMP_UGE, "uge"},
    {FCmpInst::FCMP_ULT, "ult"}, {FCmpInst::FCMP_ULE, "ule"},
    {FCmpInst::FCMP_UNE, "une"}, {FCmpInst::FCMP_TRUE, "true"},
    {ICmpInst::ICMP_EQ, "eq"}, {ICmpInst::ICMP_NE, "ne"},
    {ICmpInst::ICMP_SGT, "sgt"}, {ICmpInst::ICMP_SGE, "sge"},
    {ICmpInst::ICMP_SLT, "slt"}, {ICmpInst::ICMP_SLE, "sle"},
    {ICmpInst::ICMP_UGT, "ugt"}, {ICmpInst::ICMP_UGE, "uge"},
    {ICmpInst::ICMP_ULT, "ult"}, {ICmpInst::ICMP_ULE, "ule"}, {}};
static INTMAP_TYPE opcodeMap[] = {
    {Instruction::Add, "+"}, {Instruction::FAdd, "+"},
    {Instruction::Sub, "-"}, {Instruction::FSub, "-"},
    {Instruction::Mul, "*"}, {Instruction::FMul, "*"},
    {Instruction::UDiv, "/"}, {Instruction::SDiv, "/"}, {Instruction::FDiv, "/"},
    {Instruction::URem, "%"}, {Instruction::SRem, "%"}, {Instruction::FRem, "%"},
    {Instruction::And, "&"}, {Instruction::Or, "|"}, {Instruction::Xor, "^"}, {}};
