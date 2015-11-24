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
// Portions of this program were derived from source with the license:
//     This file is distributed under the University of Illinois Open Source
//     License. See LICENSE.TXT for details.
#include <stdio.h>
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

using namespace llvm;

#include "declarations.h"

int trace_translate ;//= 1;
static int trace_gep;// = 1;
static std::list<VTABLE_WORK> vtablework;
const Function *EntryFn;
std::string globalName;
std::map<std::string,ClassMethodTable *> classCreate;
std::map<Function *,ClassMethodTable *> functionIndex;
static std::list<const StructType *> structWork;
static std::map<const Type *, int> structMap;
std::list<RULE_PAIR> ruleList;
static int structWork_run;
std::map<std::string, void *> nameMap;
static DenseMap<const Value*, unsigned> AnonValueNumbers;
static unsigned NextAnonValueNumber;
static DenseMap<const StructType*, unsigned> UnnamedStructIDs;
unsigned NextTypeID;
int regen_methods;
int generateRegion;

INTMAP_TYPE predText[] = {
    {FCmpInst::FCMP_FALSE, "false"}, {FCmpInst::FCMP_OEQ, "oeq"},
    {FCmpInst::FCMP_OGT, "ogt"}, {FCmpInst::FCMP_OGE, "oge"},
    {FCmpInst::FCMP_OLT, "olt"}, {FCmpInst::FCMP_OLE, "ole"},
    {FCmpInst::FCMP_ONE, "one"}, {FCmpInst::FCMP_ORD, "ord"},
    {FCmpInst::FCMP_UNO, "uno"}, {FCmpInst::FCMP_UEQ, "ueq"},
    {FCmpInst::FCMP_UGT, "ugt"}, {FCmpInst::FCMP_UGE, "uge"},
    {FCmpInst::FCMP_ULT, "ult"}, {FCmpInst::FCMP_ULE, "ule"},
    {FCmpInst::FCMP_UNE, "une"}, {FCmpInst::FCMP_TRUE, "true"},
    {ICmpInst::ICMP_EQ, "=="}, {ICmpInst::ICMP_NE, "!="},
    {ICmpInst::ICMP_SGT, ">"}, {ICmpInst::ICMP_SGE, ">="},
    {ICmpInst::ICMP_SLT, "<"}, {ICmpInst::ICMP_SLE, "<="},
    {ICmpInst::ICMP_UGT, ">"}, {ICmpInst::ICMP_UGE, ">="},
    {ICmpInst::ICMP_ULT, "<"}, {ICmpInst::ICMP_ULE, "<="}, {}};
INTMAP_TYPE opcodeMap[] = {
    {Instruction::Add, "+"}, {Instruction::FAdd, "+"},
    {Instruction::Sub, "-"}, {Instruction::FSub, "-"},
    {Instruction::Mul, "*"}, {Instruction::FMul, "*"},
    {Instruction::UDiv, "/"}, {Instruction::SDiv, "/"}, {Instruction::FDiv, "/"},
    {Instruction::URem, "%"}, {Instruction::SRem, "%"}, {Instruction::FRem, "%"},
    {Instruction::And, "&"}, {Instruction::Or, "|"}, {Instruction::Xor, "^"},
    {Instruction::Shl, "<<"}, {Instruction::LShr, ">>"}, {Instruction::AShr, " >> "}, {}};

/*
 * Utility functions
 */
const char *intmapLookup(INTMAP_TYPE *map, int value)
{
    while (map->name) {
        if (map->value == value)
            return map->name;
        map++;
    }
    return "unknown";
}

static bool isInlinableInst(const Instruction &I)
{
    if (isa<CmpInst>(I) || isa<LoadInst>(I))
        return true;
    if (I.getType() == Type::getVoidTy(I.getContext()) || !I.hasOneUse()
      || isa<TerminatorInst>(I) || isa<CallInst>(I) || isa<PHINode>(I)
      || isa<VAArgInst>(I) || isa<InsertElementInst>(I)
      || isa<InsertValueInst>(I) || isa<AllocaInst>(I))
        return false;
    if (I.hasOneUse()) {
        const Instruction &User = cast<Instruction>(*I.user_back());
        if (isa<ExtractElementInst>(User) || isa<ShuffleVectorInst>(User))
            return false;
    }
    ERRORIF ( I.getParent() != cast<Instruction>(I.user_back())->getParent());
    return true;
}
static const AllocaInst *isDirectAlloca(const Value *V)
{
    const AllocaInst *AI = dyn_cast<AllocaInst>(V);
    if (!AI || AI->isArrayAllocation()
     || AI->getParent() != &AI->getParent()->getParent()->getEntryBlock())
        return 0;
    return AI;
}
static bool isAddressExposed(const Value *V)
{
    return isa<GlobalVariable>(V) || isDirectAlloca(V);
}
static std::string printString(const char *cp, int len)
{
    std::string cbuffer = "\"";
    char temp[100];
    if (!cp[len-1])
        len--;
    bool LastWasHex = false;
    for (unsigned i = 0, e = len; i != e; ++i) {
        unsigned char C = cp[i];
        if (isprint(C) && (!LastWasHex || !isxdigit(C))) {
            LastWasHex = false;
            if (C == '"' || C == '\\')
                cbuffer += "\\";
            sprintf(temp, "%c", (char)C);
            cbuffer += temp;
        } else {
            LastWasHex = false;
            switch (C) {
            case '\n': cbuffer += "\\n"; break;
            case '\t': cbuffer += "\\t"; break;
            case '\r': cbuffer += "\\r"; break;
            case '\v': cbuffer += "\\v"; break;
            case '\a': cbuffer += "\\a"; break;
            case '\"': cbuffer += "\\\""; break;
            case '\'': cbuffer += "\\\'"; break;
            default:
                cbuffer += "\\x";
                sprintf(temp, "%c", (char)(( C/16  < 10) ? ( C/16 +'0') : ( C/16 -10+'A')));
                cbuffer += temp;
                sprintf(temp, "%c", (char)(((C&15) < 10) ? ((C&15)+'0') : ((C&15)-10+'A')));
                cbuffer += temp;
                LastWasHex = true;
                break;
            }
        }
    }
    return cbuffer + "\"";
}

/*
 * Name functions
 */
std::string CBEMangle(const std::string &S)
{
    std::string Result;
    for (unsigned i = 0, e = S.size(); i != e; ++i)
        if (isalnum(S[i]) || S[i] == '_')
            Result += S[i];
        else {
            Result += '_';
            Result += 'A'+(S[i]&15);
            Result += 'A'+((S[i]>>4)&15);
            Result += '_';
        }
    return Result;
}
std::string getStructName(const StructType *STy)
{
    assert(STy);
    //if (!structWork_run)
    if (!structMap[STy])
        structWork.push_back(STy);
    if (!STy->isLiteral() && !STy->getName().empty())
        return CBEMangle("l_"+STy->getName().str());
    else {
        if (!UnnamedStructIDs[STy])
            UnnamedStructIDs[STy] = NextTypeID++;
        return "l_unnamed_" + utostr(UnnamedStructIDs[STy]);
    }
}

const StructType *findThisArgument(Function *func)
{
    const PointerType *PTy;
    const StructType *STy = NULL;
    if (func && func->arg_begin() != func->arg_end()
     && func->arg_begin()->getName() == "this"
     && (PTy = dyn_cast<PointerType>(func->arg_begin()->getType())))
        STy = dyn_cast<StructType>(PTy->getPointerElementType());
    return STy;
}

const StructType *findThisArgumentType(const PointerType *PTy)
{
    const StructType *STy = NULL;
    const FunctionType *func;
    if (PTy && (func = dyn_cast<FunctionType>(PTy->getElementType()))
     && func->getNumParams() > 0
     && (PTy = dyn_cast<PointerType>(func->getParamType(0))))
        STy = dyn_cast<StructType>(PTy->getPointerElementType());
    return STy;
}
std::string GetValueName(const Value *Operand)
{
    const GlobalAlias *GA = dyn_cast<GlobalAlias>(Operand);
    const Value *V;
    if (GA && (V = GA->getAliasee()))
        Operand = V;
    if (const GlobalValue *GV = dyn_cast<GlobalValue>(Operand))
        return CBEMangle(GV->getName().str());
    std::string Name = Operand->getName();
    if (Name.empty()) { // Assign unique names to local temporaries.
        unsigned &No = AnonValueNumbers[Operand];
        if (No == 0)
            No = ++NextAnonValueNumber;
        Name = "tmp__" + utostr(No);
    }
    std::string VarName;
    VarName.reserve(Name.capacity());
    for (std::string::iterator charp = Name.begin(), E = Name.end(); charp != E; ++charp) {
        char ch = *charp;
        if (isalnum(ch) || ch == '_')
            VarName += ch;
        else {
            char buffer[5];
            sprintf(buffer, "_%x_", ch);
            VarName += buffer;
        }
    }
    if (generateRegion == 1 && VarName != "this")
        return globalName + "_" + VarName;
    if (regen_methods)
        return VarName;
    return "V" + VarName;
}

/*
 * Output types
 */
static std::string printTypeCpp(const Type *Ty, bool isSigned, std::string NameSoFar, std::string prefix, std::string postfix, bool ptr)
{
    std::string sep = "", cbuffer = prefix, sp = (isSigned?"signed":"unsigned");

    switch (Ty->getTypeID()) {
    case Type::VoidTyID:
        cbuffer += "void " + NameSoFar;
        break;
    case Type::IntegerTyID: {
        unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
        assert(NumBits <= 128 && "Bit widths > 128 not implemented yet");
        if (NumBits == 1)
            cbuffer += "bool";
        else if (NumBits <= 8) {
            if (ptr)
                cbuffer += sp + " char";
            else
                cbuffer += "bool";
        }
        else if (NumBits <= 16)
            cbuffer += sp + " short";
        else if (NumBits <= 32)
            cbuffer += sp + " int";
        else if (NumBits <= 64)
            cbuffer += sp + " long long";
        }
        cbuffer += " " + NameSoFar;
        break;
    case Type::FunctionTyID: {
        const FunctionType *FTy = cast<FunctionType>(Ty);
        std::string tstr = " (" + NameSoFar + ") (";
        for (FunctionType::param_iterator I = FTy->param_begin(), E = FTy->param_end(); I != E; ++I) {
            tstr += printTypeCpp(*I, /*isSigned=*/false, "", sep, "", false);
            sep = ", ";
        }
        if (FTy->isVarArg()) {
            if (!FTy->getNumParams())
                tstr += " int"; //dummy argument for empty vaarg functs
            tstr += ", ...";
        } else if (!FTy->getNumParams())
            tstr += "void";
        cbuffer += printTypeCpp(FTy->getReturnType(), /*isSigned=*/false, tstr + ')', "", "", false);
        break;
        }
    case Type::StructTyID:
        cbuffer += "class " + getStructName(cast<StructType>(Ty)) + " " + NameSoFar;
        break;
    case Type::ArrayTyID: {
        const ArrayType *ATy = cast<ArrayType>(Ty);
        unsigned len = ATy->getNumElements();
        if (len == 0) len = 1;
        cbuffer += printTypeCpp(ATy->getElementType(), false, "", "", "", false) + NameSoFar + "[" + utostr(len) + "]";
        break;
        }
    case Type::PointerTyID: {
        const PointerType *PTy = cast<PointerType>(Ty);
        std::string ptrName = "*" + NameSoFar;
        if (PTy->getElementType()->isArrayTy() || PTy->getElementType()->isVectorTy())
            ptrName = "(" + ptrName + ")";
        cbuffer += printTypeCpp(PTy->getElementType(), false, ptrName, "", "", true);
        break;
        }
    default:
        llvm_unreachable("Unhandled case in printTypeCpp!");
    }
    cbuffer += postfix;
    return cbuffer;
}

static std::string printTypeVerilog(const Type *Ty, bool isSigned, std::string NameSoFar, std::string prefix, std::string postfix, bool ptr)
{
    std::string sep = "", cbuffer = prefix, sp = (isSigned?"VERILOGsigned":"VERILOGunsigned");

    switch (Ty->getTypeID()) {
    case Type::VoidTyID:
        cbuffer += "VERILOG_void " + NameSoFar;
        break;
    case Type::IntegerTyID: {
        unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
        assert(NumBits <= 128 && "Bit widths > 128 not implemented yet");
        if (NumBits == 1)
            cbuffer += "VERILOG_bool";
        else if (NumBits <= 8) {
            if (ptr)
                cbuffer += sp + " VERILOG_char";
            else
                cbuffer += " reg";
        }
        else if (NumBits <= 16)
            cbuffer += sp + " VERILOG_short";
        else if (NumBits <= 32)
            //cbuffer += sp + " VERILOG_int";
            cbuffer += " reg" + verilogArrRange(Ty);
        else if (NumBits <= 64)
            cbuffer += sp + " VERILOG_long long";
        cbuffer += " " + NameSoFar;
        }
        break;
    case Type::FunctionTyID: {
        const FunctionType *FTy = cast<FunctionType>(Ty);
        std::string tstr = " (" + NameSoFar + ") (";
        for (FunctionType::param_iterator I = FTy->param_begin(), E = FTy->param_end(); I != E; ++I) {
            tstr += printTypeVerilog(*I, /*isSigned=*/false, "", sep, "", false);
            sep = ", ";
        }
        if (FTy->isVarArg()) {
            if (!FTy->getNumParams())
                tstr += " VERILOG_int"; //dummy argument for empty vaarg functs
            tstr += ", ...";
        } else if (!FTy->getNumParams())
            tstr += "VERILOG_void";
        cbuffer += printTypeVerilog(FTy->getReturnType(), /*isSigned=*/false, tstr + ')', "", "", false);
        break;
        }
    case Type::StructTyID:
        //cbuffer += "VERILOG_class " + getStructName(cast<StructType>(Ty)) + " " + NameSoFar;
        return "";
        break;
    case Type::ArrayTyID: {
        const ArrayType *ATy = cast<ArrayType>(Ty);
        unsigned len = ATy->getNumElements();
        if (len == 0) len = 1;
        cbuffer += printTypeVerilog(ATy->getElementType(), false, "", "", "", false) + NameSoFar + "[" + utostr(len) + "]";
        break;
        }
    case Type::PointerTyID: {
        const PointerType *PTy = cast<PointerType>(Ty);
        std::string ptrName = "*" + NameSoFar;
        if (PTy->getElementType()->isArrayTy() || PTy->getElementType()->isVectorTy())
            ptrName = "(" + ptrName + ")";
        cbuffer += printTypeVerilog(PTy->getElementType(), false, ptrName, "", "", true);
        break;
        }
    default:
        llvm_unreachable("Unhandled case in printTypeVerilog!");
    }
    cbuffer += postfix;
    return cbuffer;
}
std::string printType(const Type *Ty, bool isSigned, std::string NameSoFar, std::string prefix, std::string postfix)
{
    if (generateRegion == 1)
        return printTypeVerilog(Ty, isSigned, NameSoFar, prefix, postfix, false);
    else
        return printTypeCpp(Ty, isSigned, NameSoFar, prefix, postfix, false);
}

std::string printFunctionSignature(const Function *F, std::string altname, bool Prototype, std::string postfix, int skip)
{
    std::string sep = "", statstr = "", tstr;
    FunctionType *FT = cast<FunctionType>(F->getFunctionType());
    ERRORIF (F->hasDLLImportStorageClass() || F->hasDLLExportStorageClass() || F->hasStructRetAttr() || FT->isVarArg());
    if (F->hasLocalLinkage()) statstr = "static ";
    if (altname != "")
        tstr += altname;
    else
        tstr += GetValueName(F);
    tstr += '(';
    if (F->isDeclaration()) {
        for (FunctionType::param_iterator I = FT->param_begin(), E = FT->param_end(); I != E; ++I) {
            if (!skip) {
                tstr += printType(*I, /*isSigned=*/false, "", sep, "");
                sep = ", ";
            }
            skip = 0;
        }
    } else if (!F->arg_empty()) {
        for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
            if (!skip) {
                std::string ArgName = (I->hasName() || !Prototype) ? GetValueName(I) : "";
                tstr += printType(I->getType(), /*isSigned=*/false, ArgName, sep, "");
                sep = ", ";
            }
            skip = 0;
        }
    }
    if (sep == "")
        tstr += "void"; // ret() -> ret(void) in C.
    return printType(F->getReturnType(), /*isSigned=*/false, tstr + ')', statstr, postfix);
}

/*
 * GEP and Load instructions interpreter functions
 * (just execute using the memory areas allocated by the constructors)
 */
static std::string printGEPExpression(Function ***thisp, Value *Ptr, gep_type_iterator I, gep_type_iterator E)
{
    std::string cbuffer = "(";
    PointerType *PTy;
    const StructType *STy;
    const DISubprogram *tptr;
    const ConstantInt *CI;
    void *tval = NULL;
    uint64_t Total = 0;
    const DataLayout *TD = EE->getDataLayout();
    Value *FirstOp = I.getOperand();
    bool expose = isAddressExposed(Ptr);
    std::string referstr = fetchOperand(thisp, Ptr, false);
    void *valp = nameMap[referstr];
    if (I == E)
        return referstr;
    VectorType *LastIndexIsVector = 0;
    for (gep_type_iterator TmpI = I; TmpI != E; ++TmpI) {
        LastIndexIsVector = dyn_cast<VectorType>(*TmpI);
        if (StructType *STy = dyn_cast<StructType>(*TmpI)) {
            const StructLayout *SLO = TD->getStructLayout(STy);
            CI = cast<ConstantInt>(TmpI.getOperand());
            Total += SLO->getElementOffset(CI->getZExtValue());
        } else {
            SequentialType *ST = cast<SequentialType>(*TmpI);
            // Get the index number for the array... which must be long type...
            const Constant *CV = dyn_cast<Constant>(TmpI.getOperand());
            ERRORIF(!CV || isa<GlobalValue>(CV) || !(CI = dyn_cast<ConstantInt>(CV)));
            Total += TD->getTypeAllocSize(ST->getElementType()) * CI->getZExtValue();
        }
    }
    if (LastIndexIsVector)
        cbuffer += printType(PointerType::getUnqual(LastIndexIsVector->getElementType()), false, "", "((", ")(");
    if (trace_gep)
        printf("[%s:%d] const %s Total %ld\n", __FUNCTION__, __LINE__, referstr.c_str(), (unsigned long)Total);
    if ((PTy = dyn_cast<PointerType>(Ptr->getType()))
     && (PTy = dyn_cast<PointerType>(PTy->getElementType()))
     && (STy = findThisArgumentType(PTy))
     && (tptr = lookupMethod(STy, Total/sizeof(void *)))) {
        std::string name = CBEMangle(tptr->getName().str());
        std::string lname = CBEMangle(tptr->getLinkageName().str());
        if (referstr == "(*(this))" || referstr == "(*(Vthis))") {
            if (trace_gep)
                printf("%s: p %s name %s lname %s\n", __FUNCTION__, referstr.c_str(), name.c_str(), lname.c_str());
            cbuffer += "&" + (generateRegion == 0 ? lname : name);
            goto exitlab;
        }
        else {
            if (trace_gep)
                printf("%s: notthis %s name %s\n", __FUNCTION__, referstr.c_str(), name.c_str());
            if (referstr.length() < 2 || referstr.substr(0,2) != "0x") {
                cbuffer += "&" + referstr + "." + name;
                goto exitlab;
            }
        }
    }
    if ((referstr[0] == '(' && referstr[referstr.length()-1] == ')'
       && (tval = mapLookup(referstr.substr(1, referstr.length() - 2).c_str())))
     || (tval = mapLookup(referstr.c_str())))
        goto tvallab;
    if (!isa<Constant>(FirstOp) || !cast<Constant>(FirstOp)->isNullValue()) {
        cbuffer += "&" + referstr;
    } else {
       ++I;  // Skip the zero index.
       if (expose && I != E && (*I)->isArrayTy()
         && (CI = dyn_cast<ConstantInt>(I.getOperand()))) {
           uint64_t val = CI->getZExtValue();
           ++I;     // we processed this index
           GlobalVariable *gv = dyn_cast<GlobalVariable>(Ptr);
           if (gv && !gv->getInitializer()->isNullValue()) {
               Constant* CPV = dyn_cast<Constant>(gv->getInitializer());
               if (dyn_cast<ConstantArray>(CPV)) {
                   ERRORIF (val != 2);
                   val = 0;
               }
               else if (ConstantDataArray *CPA = dyn_cast<ConstantDataArray>(CPV)) {
                   ERRORIF (val);
                   if (CPA->isString()) {
                       StringRef value = CPA->getAsString();
                       cbuffer += printString(value.str().c_str(), value.str().length());
                   } else {
                       std::string sep = " ";
                       cbuffer += "{";
                       for (unsigned i = 0, e = CPA->getNumOperands(); i != e; ++i) {
                           cbuffer += sep + printOperand(thisp, CPA->getOperand(i), false);
                           sep = ", ";
                       }
                       cbuffer += " }";
                   }
                   goto next;
               }
           }
           cbuffer += fetchOperand(thisp, Ptr, true);
           if (trace_gep)
               printf("[%s:%d] cbuf %s\n", __FUNCTION__, __LINE__, cbuffer.c_str());
next:
           if (val) {
               char temp[100];
               sprintf(temp, "+%lld", (long long)val);
               cbuffer += temp;
           }
       }
       else if (expose)
               cbuffer += "&" + fetchOperand(thisp, Ptr, true);
       else if (I != E && (*I)->isStructTy()) {
             std::string fieldp = fieldName(dyn_cast<StructType>(*I), cast<ConstantInt>(I.getOperand())->getZExtValue());
             if (trace_gep)
                 printf("[%s:%d] writeop %s found %p thisp %p fieldp %s\n", __FUNCTION__, __LINE__, referstr.c_str(), valp, thisp, fieldp.c_str());
             if (thisp && referstr == "Vthis") {
                 tval = thisp;
                 goto tvallab;
             }
             if (valp) {
                 char temp[1000];
                 sprintf(temp, "0x%llx", (long long)Total + (long)valp);
                 cbuffer += temp;
                 goto exitlab;
             }
             if (!strncmp(referstr.c_str(), "(0x", 3) && referstr[referstr.length()-1] == ')')
                 referstr = referstr.substr(1,referstr.length()-2);
             if (!strncmp(referstr.c_str(), "0x", 2)) {
                 tval = mapLookup(referstr.c_str());
                 goto tvallab;
             }
             if (!strncmp(referstr.c_str(), "(&", 2) && referstr[referstr.length()-1] == ')') {
                 referstr = referstr.substr(2,referstr.length()-2);
                 tval = mapLookup(referstr.c_str());
                 if (tval)
                     goto tvallab;
                 cbuffer += "&" + referstr + "." + fieldp;
             }
             cbuffer += "&";
             if (referstr != "this")
                 cbuffer += referstr + "->";
             cbuffer += fieldp;
             ++I;  // eat the struct index as well.
           }
        else
             cbuffer += "&(" + fetchOperand(thisp, Ptr, true) + ")";
    }
    for (; I != E; ++I) {
        if ((*I)->isStructTy()) {
            StructType *STy = dyn_cast<StructType>(*I);
            cbuffer += "." + fieldName(STy, cast<ConstantInt>(I.getOperand())->getZExtValue());
        } else if ((*I)->isArrayTy() || !(*I)->isVectorTy())
            cbuffer += "[" + fetchOperand(thisp, I.getOperand(), false) + "]";
        else {
            if (!isa<Constant>(I.getOperand()) || !cast<Constant>(I.getOperand())->isNullValue())
                cbuffer += ")+(" + printOperand(thisp, I.getOperand(), false);
            cbuffer += "))";
        }
    }
    goto exitlab;
tvallab:
    char temp2[1000];
    sprintf(temp2, "0x%llx", (long long)Total + (long)tval);
    cbuffer += temp2;
exitlab:
    cbuffer += ")";
    if (!strncmp(cbuffer.c_str(), "(0x", 3))
        cbuffer = cbuffer.substr(1, cbuffer.length()-2);
    if (trace_gep) {
        printf("[%s:%d] return %s; ", __FUNCTION__, __LINE__, cbuffer.c_str());
        if (!strncmp(cbuffer.c_str(), "0x", 2)) {
            char *endptr;
            void **pint = (void **)strtol(cbuffer.c_str()+2, &endptr, 16);
            printf(" [%p]= %p", pint, *pint);
        }
        printf("\n");
    }
    return cbuffer;
}
std::string fetchOperand(Function ***thisp, Value *Operand, bool Indirect)
{
    std::string cbuffer;
    Instruction *I = dyn_cast<Instruction>(Operand);
    bool isAddressImplicit = isAddressExposed(Operand);
    std::string prefix;
    if (Indirect && isAddressImplicit) {
        isAddressImplicit = false;
        Indirect = false;
    }
    if (Indirect)
        prefix = "*";
    if (isAddressImplicit)
        prefix = "(&";  // Global variables are referenced as their addresses by llvm
    if (I && isInlinableInst(*I)) {
        std::string p = processInstruction(thisp, I);
        if (p[0] == '(' && p[p.length()-1] == ')')
            p = p.substr(1,p.length()-2);
        if (prefix == "*" && p[0] == '&') {
            prefix = "";
            p = p.substr(1);
        }
        if (prefix == "*" && !strncmp(p.c_str(), "0x", 2)) {
            char *endptr;
            void **pint = (void **)strtol(p.c_str()+2, &endptr, 16);
            char temp[100];
            sprintf(temp, "0x%lx", (unsigned long)*pint);
            cbuffer += temp;
        }
        else {
            int addparen = strncmp(p.c_str(), "0x", 2) && (p[0] != '(' || p[p.length()-1] != ')');
            cbuffer += prefix;
            if (addparen)
                cbuffer += "(";
            cbuffer += p;
            if (addparen)
                cbuffer += ")";
        }
    }
    else {
        cbuffer += prefix;
        Constant* CPV = dyn_cast<Constant>(Operand);
        if (!CPV || isa<GlobalValue>(CPV))
            cbuffer += GetValueName(Operand);
        else {
            /* handle expressions */
            ERRORIF(isa<UndefValue>(CPV) && CPV->getType()->isSingleValueType()); /* handle 'undefined' */
            if (ConstantExpr *CE = dyn_cast<ConstantExpr>(CPV)) {
                cbuffer += "(";
                int op = CE->getOpcode();
                assert (op == Instruction::GetElementPtr);
                // used for character string args to printf()
                cbuffer += printGEPExpression(thisp, CE->getOperand(0), gep_type_begin(CPV), gep_type_end(CPV)) +  ")";
            }
            else if (ConstantInt *CI = dyn_cast<ConstantInt>(CPV)) {
                char temp[100];
                Type* Ty = CI->getType();
                temp[0] = 0;
                if (Ty == Type::getInt1Ty(CPV->getContext()))
                    cbuffer += CI->getZExtValue() ? "1" : "0";
                else if (Ty == Type::getInt32Ty(CPV->getContext()) || Ty->getPrimitiveSizeInBits() > 32) {
                    sprintf(temp, "%ld", CI->getZExtValue());
                }
                else if (CI->isMinValue(true))
                    sprintf(temp, "%ld", CI->getZExtValue());//  'u';
                else
                    sprintf(temp, "%ld", CI->getSExtValue());
                cbuffer += temp;
            }
            else
                ERRORIF(1); /* handle structured types */
        }
    }
    if (isAddressImplicit)
        cbuffer += ")";
    return cbuffer;
}
std::string printOperand(Function ***thisp, Value *Operand, bool Indirect)
{
    std::string p = fetchOperand(thisp, Operand, Indirect);
    void *tval = mapLookup(p.c_str());
    if (tval) {
        char temp[1000];
        sprintf(temp, "%s%s", Indirect ? "" : "&", mapAddress(tval).c_str());
        return std::string(temp);
    }
    return p;
}
/*
 * Pass control functions
 */
std::string processInstruction(Function ***thisp, Instruction *ins)
{
    switch (ins->getOpcode()) {
    case Instruction::GetElementPtr: {
        GetElementPtrInst &IG = static_cast<GetElementPtrInst&>(*ins);
        return printGEPExpression(thisp, IG.getPointerOperand(), gep_type_begin(IG), gep_type_end(IG));
        }
    case Instruction::Load: {
        LoadInst &IL = static_cast<LoadInst&>(*ins);
        ERRORIF (IL.isVolatile());
        return fetchOperand(thisp, ins->getOperand(0), true);
        }
    case Instruction::Alloca: // ignore
        if (generateRegion == 2) {
            if (const AllocaInst *AI = isDirectAlloca(&*ins))
              return printType(AI->getAllocatedType(), false, GetValueName(AI), "    ", ";    /* Address-exposed local */\n");
        }
        break;
    }
    if (generateRegion == 2)
        return processCInstruction(thisp, *ins);
    else
        return generateRegion ? generateVerilog(thisp, *ins)
                        : calculateGuardUpdate(thisp, *ins);
}

/*
 * Walk all BasicBlocks for a Function, calling requested processing function
 */
static std::map<const Function *, int> funcSeen;
void processFunction(VTABLE_WORK &work, FILE *outputFile, std::string aclassName)
{
    Function *func = work.f;
    int hasGuard = 0;
    const char *className, *methodName;
    int hasRet = (func->getReturnType() != Type::getVoidTy(func->getContext()));
    std::string fname = func->getName();
    int regenItem = (regen_methods && getClassName(fname.c_str(), &className, &methodName));

    NextAnonValueNumber = 0;
    nameMap.clear();
    if (fname.length() > 5 && fname.substr(0,5) == "_ZNSt") {
        printf("SKIPPING %s\n", fname.c_str());
        return;
    }
    if (regenItem)
        globalName = methodName;
    else {
        globalName = fname;
        fprintf(outputFile, "//processing %s\n", globalName.c_str());
    }
    //printf("PROCESSING %s %d\n", globalName.c_str(), regenItem);
    if (generateRegion == 1 && !strncmp(&globalName.c_str()[globalName.length() - 6], "3ENAEv", 9)) {
        hasGuard = 1;
        fprintf(outputFile, "    if (%s__ENA) begin\n", globalName.c_str());
    }
    if (trace_translate) {
        printf("FULL_AFTER_OPT: %s\n", func->getName().str().c_str());
        func->dump();
        printf("TRANSLATE:\n");
    }
#if 0
    /* connect up argument formal param names with actual values */
    for (Function::const_arg_iterator AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
        int slotindex = getLocalSlot(AI);
        ERRORIF (AI->hasByValAttr());
        (AI->getName().str().c_str());
    }
#endif
    /* Generate Verilog for all instructions.  Record function calls for post processing */
    if (generateRegion == 2) {
        int status;
        const char *demang = abi::__cxa_demangle(globalName.c_str(), 0, 0, &status);
        if (!regenItem && ((demang && strstr(demang, "::~"))
         || func->isDeclaration() || globalName == "_Z16run_main_programv" || globalName == "main"
         || globalName == "__dtor_echoTest" || funcSeen[func]))
            return;
        funcSeen[func] = 1;
        std::string mname = globalName;
        if (regenItem)
            mname = std::string(aclassName) + "::" + mname;
        fprintf(outputFile, "%s", printFunctionSignature(func, mname, false, " {\n", work.skip || regenItem).c_str());
    }
    //manually done (only for methods) nameMap["Vthis"] = work.thisp;
    for (Function::iterator BB = func->begin(), E = func->end(); BB != E; ++BB) {
        if (trace_translate && BB->hasName())         // Print out the label if it exists...
            printf("LLLLL: %s\n", BB->getName().str().c_str());
        for (BasicBlock::iterator ins = BB->begin(), ins_end = BB->end(); ins != ins_end;) {

            BasicBlock::iterator next_ins = std::next(BasicBlock::iterator(ins));
            if (!isInlinableInst(*ins)) {
                if (trace_translate && generateRegion == 2)
                    printf("/*before %p opcode %d.=%s*/\n", &*ins, ins->getOpcode(), ins->getOpcodeName());
                std::string vout = processInstruction(work.thisp, ins);
                if (vout != "") {
                    if (vout[0] == '&' && !isDirectAlloca(&*ins) && ins->getType() != Type::getVoidTy(BB->getContext())
                     && ins->use_begin() != ins->use_end()) {
                        std::string name = GetValueName(&*ins);
                        void *tval = mapLookup(vout.c_str()+1);
                        if (trace_translate)
                            printf("%s: settingnameMap [%s]=%p\n", __FUNCTION__, name.c_str(), tval);
                        if (tval)
                            nameMap[name] = tval;
                    }
                    fprintf(outputFile, "    ");
                    if (generateRegion == 2) {
                        if (!isDirectAlloca(&*ins) && ins->getType() != Type::getVoidTy(BB->getContext())
                         && ins->use_begin() != ins->use_end())
                            fprintf(outputFile, "%s", printType(ins->getType(), false, GetValueName(&*ins), "", " = ").c_str());
                        fprintf(outputFile, "    ");
                    }
                    else {
                        if (!hasRet)
                            fprintf(outputFile, "    ");
                        if (!isDirectAlloca(&*ins) && ins->getType() != Type::getVoidTy(BB->getContext())
                         && ins->use_begin() != ins->use_end())
                            fprintf(outputFile, "%s = ", GetValueName(&*ins).c_str());
                    }
                    fprintf(outputFile, "%s;\n", vout.c_str());
                }
            }
            if (trace_translate)
                printf("\n");
            ins = next_ins;
        }
    }
    if (hasGuard)
        fprintf(outputFile, "    end; // if (%s__ENA) \n", globalName.c_str());
    if (generateRegion == 2)
        fprintf(outputFile, "}\n");
    else if (!regenItem)
        fprintf(outputFile, "\n");
}

void pushWork(Function *func, Function ***thisp, int skip)
{
    const char *className, *methodName;
    const StructType *STy;

    if (!func)
        return;
    if (getClassName(func->getName().str().c_str(), &className, &methodName)
     && (STy = findThisArgument(func))) {
        std::string sname = getStructName(STy);
        if (!classCreate[sname])
            classCreate[sname] = new ClassMethodTable(sname, className);
        classCreate[sname]->method[func] = methodName;
        functionIndex[func] = classCreate[sname];
    }
    vtablework.push_back(VTABLE_WORK(func, thisp, skip));
}

/*
 * Symbolically run through all rules, running either preprocessing or
 * generating verilog.
 */
static void processRules(FILE *outputFile, FILE *outputNull, FILE *headerFile)
{
    ruleList.clear();
    //structWork.clear();
    // Walk the rule lists for all modules, generating work items
    for (RULE_INFO *info : ruleInfo) {
        printf("RULE_INFO: rule %s thisp %p, RDY %p ENA %p\n", info->name, info->thisp, info->RDY, info->ENA);
        RULE_PAIR p = {info->RDY, info->ENA};
        pushWork(p.RDY, (Function ***)info->thisp, 1);
        pushWork(p.ENA, (Function ***)info->thisp, 1);
        ruleList.push_back(p);
    }

    // Walk list of work items, generating code
    while (vtablework.begin() != vtablework.end()) {
        Function *F = vtablework.begin()->f;
        processFunction(*vtablework.begin(), (functionIndex[F] ? outputNull : outputFile), "");
        vtablework.pop_front();
    }
}

static void printContainedStructs(const Type *Ty, FILE *OStr, std::string ODir)
{
    if (const PointerType *PTy = dyn_cast<PointerType>(Ty)) {
        const StructType *subSTy = dyn_cast<StructType>(PTy->getElementType());
        if (subSTy && structMap[subSTy]) /* Not recursion!  These are generated afterword, if we didn't generate before */
            structWork.push_back(subSTy);
    }
    else if (!structMap[Ty]) {
        const StructType *STy = dyn_cast<StructType>(Ty);
        std::string name;
        structMap[Ty] = 1;
        for (Type::subtype_iterator I = Ty->subtype_begin(), E = Ty->subtype_end(); I != E; ++I)
            printContainedStructs(*I, OStr, ODir);
        if (STy) {
            if (STy->getName() == "class.Module")
                return;  // just a dummy class
            for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
                printContainedStructs(*I, OStr, ODir);
            if (generateRegion == 1)
                generateModuleDef(STy, ODir);
            else if (generateRegion == 2)
                generateClassBody(STy, OStr);
            else
                generateClassDef(STy, OStr);
        }
    }
}
static void generateStructs(FILE *OStr, std::string oDir)
{
    structWork_run = 1;
    structMap.clear();
    auto current = structWork.begin();
    while (current != structWork.end()) {
        printContainedStructs(*current, OStr, oDir);
        current = std::next(current);
    }
    structWork_run = 0;
}

bool GenerateRunOnModule(Module *Mod, std::string OutDirectory)
{
    FILE *Out = fopen((OutDirectory + "/output.cpp").c_str(), "w");
    FILE *OutHeader = fopen((OutDirectory + "/output.h").c_str(), "w");
    FILE *OutNull = fopen("/dev/null", "w");
    FILE *OutVInstance = fopen((OutDirectory + "/vinst.v").c_str(), "w");
    FILE *OutVMain = fopen((OutDirectory + "/main.v").c_str(), "w");

printf("[%s:%d] globalMod %p\n", __FUNCTION__, __LINE__, globalMod);
    const char *delete_names[] = { "llvm.dbg.declare", "llvm.dbg.value", "atexit", NULL};
    const char **p = delete_names;
    while(*p) {
        if (Function *Declare = Mod->getFunction(*p)) {
            while (!Declare->use_empty()) {
                CallInst *CI = cast<CallInst>(Declare->user_back());
                CI->eraseFromParent();
            }
            Declare->eraseFromParent();
        }
        p++;
    }
    EntryFn = Mod->getFunction("main");
    if (!EntryFn) {
        printf("'main' function not found in module.\n");
        exit(1);
    }
    for (Module::iterator FB = Mod->begin(), FE = Mod->end(); FB != FE; ++FB)
        callMemrunOnFunction(*FB);

    // run Constructors
    EE->runStaticConstructorsDestructors(false);

    // Construct the address -> symbolic name map using dwarf debug info
    for (Module::iterator FB = Mod->begin(), FE = Mod->end(); FB != FE; ++FB)
        addressrunOnFunction(*FB);
    constructAddressMap(Mod);
    //dump_class_data();

    for (Module::iterator FB = Mod->begin(), FE = Mod->end(); FB != FE; ++FB)
        call2runOnFunction(*FB);

    // Preprocess the body rules, creating shadow variables and moving items to RDY() and ENA()
    generateRegion = 0;
    processRules(OutNull, OutNull, OutNull);

    // Generate verilog for all rules
    generateRegion = 1;
    fprintf(OutVMain, "module top(input CLK, input nRST);\n  always @( posedge CLK) begin\n    if (!nRST) then begin\n    end\n    else begin\n");
    processRules(OutVMain, OutNull, OutNull);
    fprintf(OutVMain, "    end; // nRST\n  end; // always @ (posedge CLK)\nendmodule \n\n");
    generateStructs(NULL, OutDirectory);
    generateVerilogHeader(*Mod, OutVInstance, OutNull);

    // Generate cpp code for all rules
    generateRegion = 2;
    generateCppData(Out, *Mod);
    processRules(Out, OutNull, OutHeader);
    UnnamedStructIDs.clear();
    generateStructs(Out, ""); // generate class method bodies
    generateRegion = 3;
    generateStructs(OutHeader, ""); // generate class definitions
    return false;
}
