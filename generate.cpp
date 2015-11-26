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
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

#include "declarations.h"

int trace_translate ;//= 1;
static int trace_gep;// = 1;
static int trace_hoist = 1;
static std::list<VTABLE_WORK> vtablework;
const Function *EntryFn;
std::string globalName;
std::map<std::string,ClassMethodTable *> classCreate;
std::map<Function *,ClassMethodTable *> functionIndex;
static std::list<const StructType *> structWork;
static std::map<const Type *, int> structMap;
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
    if (generateRegion == ProcessVerilog && VarName != "this")
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
        for (auto I = FTy->param_begin(), E = FTy->param_end(); I != E; ++I) {
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
        for (auto I = FTy->param_begin(), E = FTy->param_end(); I != E; ++I) {
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
    if (generateRegion == ProcessVerilog)
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
        for (auto I = FT->param_begin(), E = FT->param_end(); I != E; ++I) {
            if (!skip) {
                tstr += printType(*I, /*isSigned=*/false, "", sep, "");
                sep = ", ";
            }
            skip = 0;
        }
    } else if (!F->arg_empty()) {
        for (auto I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
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
    std::string sep = " ";
    std::string amper = "&";
    PointerType *PTy;
    const StructType *STy;
    const ConstantInt *CI;
    ConstantDataArray *CPA;
    void *tval = NULL;
    uint64_t Total = 0;
    VectorType *LastIndexIsVector = 0;
    GlobalVariable *globalVar = dyn_cast<GlobalVariable>(Ptr);
    const DataLayout *TD = EE->getDataLayout();
    Constant *FirstOp = dyn_cast<Constant>(I.getOperand());
    bool expose = isAddressExposed(Ptr);
    std::string referstr = fetchOperand(thisp, Ptr, false);
    if (referstr[0] == '(' && referstr[referstr.length()-1] == ')')
       referstr = referstr.substr(1, referstr.length() - 2).c_str();

    for (auto TmpI = I; TmpI != E; ++TmpI) {
        LastIndexIsVector = dyn_cast<VectorType>(*TmpI);
        if (StructType *STy = dyn_cast<StructType>(*TmpI)) {
            const StructLayout *SLO = TD->getStructLayout(STy);
            CI = cast<ConstantInt>(TmpI.getOperand());
            Total += SLO->getElementOffset(CI->getZExtValue());
        } else {
            ERRORIF(isa<GlobalValue>(TmpI.getOperand()) || !(CI = dyn_cast<ConstantInt>(TmpI.getOperand())));
            Total += TD->getTypeAllocSize(cast<SequentialType>(*TmpI)->getElementType()) * CI->getZExtValue();
        }
    }
    if (LastIndexIsVector)
        cbuffer += printType(PointerType::getUnqual(LastIndexIsVector->getElementType()), false, "", "((", ")(");
    if (trace_gep)
        printf("[%s:%d] const %s Total %ld\n", __FUNCTION__, __LINE__, referstr.c_str(), (unsigned long)Total);
    if (I == E)
        return referstr;
    if ((tval = mapLookup(referstr.c_str())) || (tval = nameMap[referstr]))
        goto tvallab;
    if ((PTy = dyn_cast<PointerType>(Ptr->getType()))
     && (PTy = dyn_cast<PointerType>(PTy->getElementType()))
     && (STy = findThisArgumentType(PTy))
     && (referstr == "*(this)" || referstr == "*(Vthis)"
        || referstr.length() < 2 || referstr.substr(0,2) != "0x")) {
        std::string lname = lookupMethod(STy, Total/sizeof(void *));
        std::string name;
        const char *className, *methodName;
        if (getClassName(lname.c_str(), &className, &methodName))
            name = methodName;
        //if (trace_gep)
            printf("%s: STy %s thisp %p referstr %s name %s lname %s\n", __FUNCTION__,
                STy->getName().str().c_str(), thisp, referstr.c_str(), name.c_str(), lname.c_str());
        referstr = "(" + referstr + ").";
        if (referstr == "(*(this))." || referstr == "(*(Vthis)).")
            referstr = "";
        referstr += CBEMangle(generateRegion == ProcessHoist ? lname : name);
        I = E; // skip post processing
    }
    else if (FirstOp && FirstOp->isNullValue()) {
        ++I;  // Skip the zero index.
        if (I != E && ((expose && (*I)->isArrayTy())
                    || (!expose && (STy = dyn_cast<StructType>(*I))))
         && (CI = dyn_cast<ConstantInt>(I.getOperand()))) {
            uint64_t val = CI->getZExtValue();
            ++I;     // we processed this index
            if (expose) {
                if (globalVar && !globalVar->getInitializer()->isNullValue()
                 && (CPA = dyn_cast<ConstantDataArray>(globalVar->getInitializer()))) {
                    ERRORIF(val || !CPA->isString());
                    std::string value = CPA->getAsString();
                    referstr = printString(value.c_str(), value.length());
                }
                if (val)
                    referstr += '+' + utostr(val);
                amper = "";
            }
            else {
                referstr += "->";
                if (referstr == "this->")
                    referstr = "";
                referstr += fieldName(STy, val);
            }
            if (trace_gep)
                printf("[%s:%d] expose %d referstr %s\n", __FUNCTION__, __LINE__, expose, referstr.c_str());
        }
    }
    cbuffer += amper + referstr;
    for (; I != E; ++I) {
        if (StructType *STy = dyn_cast<StructType>(*I))
            cbuffer += "." + fieldName(STy, cast<ConstantInt>(I.getOperand())->getZExtValue());
        else if ((*I)->isArrayTy() || !(*I)->isVectorTy())
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
        std::string p = processInstruction(thisp, *I);
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

std::string printCall(Function ***thisp, Instruction &I)
{
    std::string vout;
    int RDYName = -1, ENAName = -1;
    Function *parentRDYName = NULL, *parentENAName = NULL;
    std::string fname;
    const StructType *STy;
    const char *className, *methodName;
    CallInst &ICL = static_cast<CallInst&>(I);
    unsigned ArgNo = 0;
    const char *sep = "";
    Function *func = ICL.getCalledFunction();
    ERRORIF(func && (Intrinsic::ID)func->getIntrinsicID());
    ERRORIF (ICL.hasStructRetAttr() || ICL.hasByValArgument() || ICL.isTailCall());
    Value *Callee = ICL.getCalledValue();
    CallSite CS(&I);
    CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
    std::string cthisp = fetchOperand(thisp, *AI, false);
    Function ***called_thisp = (Function ***)mapLookup(cthisp.c_str());
    std::string pcalledFunction = printOperand(thisp, Callee, false);
    if (!strncmp(pcalledFunction.c_str(), "&0x", 3) && !func) {
        if (void *tval = mapLookup(pcalledFunction.c_str()+1)) {
            func = static_cast<Function *>(tval);
            pcalledFunction = func->getName();
            //printf("[%s:%d] tval %p pnew %s\n", __FUNCTION__, __LINE__, tval, pcalledFunction.c_str());
        }
    }
    pushWork(func, called_thisp, 0);
    int skip = regen_methods;
    int hasRet = !func || (func->getReturnType() != Type::getVoidTy(func->getContext()));
    std::string prefix;
    PointerType  *PTy = (func) ? cast<PointerType>(func->getType()) : cast<PointerType>(Callee->getType());
    FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
    unsigned len = FTy->getNumParams();
    ERRORIF(FTy->isVarArg() && !len);
    void *pact = mapLookup(pcalledFunction.c_str());
    ConstantExpr *CE = dyn_cast<ConstantExpr>(Callee);
    ERRORIF (CE && CE->isCast() && (dyn_cast<Function>(CE->getOperand(0))));

    if (generateRegion == ProcessHoist) {
    if (getClassName(globalName.c_str(), &className, &methodName)) {
        parentRDYName = lookup_function((std::string("class.") + className).c_str(), std::string(methodName) + "__RDY");
        //parentENAName = lookup_method(temp, "ENA");
    }
    if (trace_hoist)
        printf("HOIST: CALLER %s[%s, %s] pRDY %p pENA %p thisp %p func %p pcalledFunction '%s' = %p\n", globalName.c_str(), className, methodName, parentRDYName, parentENAName, thisp, func, pcalledFunction.c_str(), pact);
    if (!func)
        func = static_cast<Function *>(pact);
    if (!func) {
        printf("%s not an instantiable call!!!! %s\n", __FUNCTION__, pcalledFunction.c_str());
        return "";
    }
    fname = func->getName();
    if (trace_hoist)
        printf("HOIST:    CALL %p typeid %d fname %s\n", func, I.getType()->getTypeID(), fname.c_str());
    if (func->isDeclaration() && !strncmp(fname.c_str(), "_Z14PIPELINEMARKER", 18)) {
        cloneVmap.clear();
        /* for now, just remove the Call.  Later we will push processing of I.getOperand(0) into another block */
        Function *F = I.getParent()->getParent();
        Module *Mod = F->getParent();
        std::string Fname = F->getName().str();
        std::string otherName = Fname.substr(0, Fname.length() - 8) + "2" + "3ENAEv";
        Function *otherBody = Mod->getFunction(otherName);
        TerminatorInst *TI = otherBody->begin()->getTerminator();
        prepareClone(TI, &I);
        Instruction *IT = dyn_cast<Instruction>(I.getOperand(1));
        Instruction *IC = dyn_cast<Instruction>(I.getOperand(0));
        Instruction *newIC = cloneTree(IC, TI);
        Instruction *newIT = cloneTree(IT, TI);
        printf("[%s:%d] other %s %p\n", __FUNCTION__, __LINE__, otherName.c_str(), otherBody);
        IRBuilder<> builder(TI->getParent());
        builder.SetInsertPoint(TI);
        builder.CreateStore(newIC, newIT);
        IRBuilder<> oldbuilder(I.getParent());
        oldbuilder.SetInsertPoint(&I);
        Value *newLoad = oldbuilder.CreateLoad(IT);
        I.replaceAllUsesWith(newLoad);
        I.eraseFromParent();
        return "";
    }
    if ((STy = findThisArgument(func))
     && getClassName(fname.c_str(), &className, &methodName)) {
        std::string tname = STy->getName();
        char tempname[1000];
        strcpy(tempname, methodName);
        strcat(tempname, "__RDY");
        RDYName = lookup_method(tname.c_str(), tempname);
        if (trace_hoist)
            printf("HOIST:    RDYName %d RDYLOOK %s %s class %s ENAName %d\n", RDYName, methodName, tempname, tname.c_str(), ENAName);
    }
    if (RDYName >= 0 && parentRDYName) {
        TerminatorInst *TI = parentRDYName->begin()->getTerminator();
        Instruction *newI = copyFunction(TI, &I, RDYName, Type::getInt1Ty(TI->getContext()));
        if (CallInst *nc = dyn_cast<CallInst>(newI))
            nc->addAttribute(AttributeSet::ReturnIndex, Attribute::ZExt);
        Value *cond = TI->getOperand(0);
        const ConstantInt *CI = dyn_cast<ConstantInt>(cond);
        if (CI && CI->getType()->isIntegerTy(1) && CI->getZExtValue())
            TI->setOperand(0, newI);
        else {
            // 'And' return value into condition
            Instruction *newBool = BinaryOperator::Create(Instruction::And, newI, newI, "newand", TI);
            cond->replaceAllUsesWith(newBool);
            // we must set this after the 'replaceAllUsesWith'
            newBool->setOperand(0, cond);
        }
    }
    if (parentENAName) {
        TerminatorInst *TI = parentENAName->begin()->getTerminator();
        if (ENAName >= 0)
            copyFunction(TI, &I, ENAName, Type::getVoidTy(TI->getContext()));
        else if (I.use_empty()) {
            copyFunction(TI, &I, 0, NULL); // Move this call to the 'ENA()' method
            I.eraseFromParent(); // delete "Call" instruction
        }
    }
    if (cthisp == "Vthis") {
        printf("HOIST:    single!!!! %s\n", func->getName().str().c_str());
        fprintf(stderr, "[%s:%d] thisp %p func %p pcalledFunction %s\n", __FUNCTION__, __LINE__, thisp, func, pcalledFunction.c_str());
        ICL.dump();
        I.dump();
        I.getOperand(0)->dump();
        InlineFunctionInfo IFI;
        InlineFunction(&ICL, IFI, false);
    }
    }
    else if (generateRegion == ProcessVerilog) {
    if (ClassMethodTable *CMT = functionIndex[func]) {
        pcalledFunction = printOperand(thisp, *AI, false);
        if (pcalledFunction[0] == '&')
            pcalledFunction = pcalledFunction.substr(1);
        std::string pnew = pcalledFunction;
        referencedItems[pnew] = func->getType();
        prefix = pcalledFunction + CMT->method[func];
        vout += prefix;
        if (!hasRet)
            vout += "__ENA = 1";
        skip = 1;
    }
    else {
        vout += pcalledFunction;
        if (regen_methods)
            return vout;
    }
    if (prefix == "")
        vout += "(";
    Function::const_arg_iterator FAI = func->arg_begin();
    for (; AI != AE; ++AI, ++ArgNo, FAI++) {
        if (!skip) {
            vout += sep;
            std::string p = printOperand(thisp, *AI, false);
            if (prefix != "")
                vout += (";\n            " + prefix + "_" + FAI->getName().str() + " = ");
            else {
                sep = ", ";
            }
            vout += p;
        }
        skip = 0;
    }
    }
    else {
    if (ClassMethodTable *CMT = functionIndex[func]) {
        std::string pfirst = printOperand(thisp, *AI, false);
        if (pfirst[0] == '&')
            pfirst = pfirst.substr(1);
        vout += pfirst + "." + CMT->method[func];
        skip = 1;
    }
    else
        vout += pcalledFunction;
    vout += "(";
    if (len && FTy->getParamType(0)->getTypeID() != Type::PointerTyID) {
        printf("[%s:%d] clear skip\n", __FUNCTION__, __LINE__);
        skip = 0;
    }
    for (; AI != AE; ++AI, ++ArgNo) {
        if (!skip) {
            vout += sep + printOperand(thisp, *AI, false);
            sep = ", ";
        }
        skip = 0;
    }
    }
    if (prefix == "")
        vout += ")";
    return vout;
}

/*
 * Output instructions
 */
std::string processInstruction(Function ***thisp, Instruction &I)
{
    std::string vout;
    int opcode = I.getOpcode();
//printf("[%s:%d] op %s thisp %p\n", __FUNCTION__, __LINE__, I.getOpcodeName(), thisp);
    switch(opcode) {
    case Instruction::GetElementPtr: {
        GetElementPtrInst &IG = static_cast<GetElementPtrInst&>(I);
        return printGEPExpression(thisp, IG.getPointerOperand(), gep_type_begin(IG), gep_type_end(IG));
        }
    case Instruction::Load: {
        LoadInst &IL = static_cast<LoadInst&>(I);
        ERRORIF (IL.isVolatile());
        return fetchOperand(thisp, I.getOperand(0), true);
        }
    case Instruction::Alloca: // ignore
        if (generateRegion == ProcessCPP) {
            if (const AllocaInst *AI = isDirectAlloca(&I))
              return printType(AI->getAllocatedType(), false, GetValueName(AI), "    ", ";    /* Address-exposed local */\n");
        }
        break;
    // Terminators
    case Instruction::Ret:
        if (generateRegion == ProcessHoist)
            break;
        if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
            if (generateRegion == ProcessVerilog)
                vout += "    " + globalName + " = ";
            else
                vout += "return ";
            if (I.getNumOperands())
                vout += printOperand(thisp, I.getOperand(0), false);
        }
        break;
    case Instruction::Unreachable:
        break;

    // Standard binary operators...
    case Instruction::Add: case Instruction::FAdd:
    case Instruction::Sub: case Instruction::FSub:
    case Instruction::Mul: case Instruction::FMul:
    case Instruction::UDiv: case Instruction::SDiv: case Instruction::FDiv:
    case Instruction::URem: case Instruction::SRem: case Instruction::FRem:
    case Instruction::Shl: case Instruction::LShr: case Instruction::AShr:
    // Logical operators...
    case Instruction::And: case Instruction::Or: case Instruction::Xor:
        assert(!I.getType()->isPointerTy());
        if (BinaryOperator::isNeg(&I))
            vout += "-(" + printOperand(thisp, BinaryOperator::getNegArgument(cast<BinaryOperator>(&I)), false) + ")";
        else if (BinaryOperator::isFNeg(&I))
            vout += "-(" + printOperand(thisp, BinaryOperator::getFNegArgument(cast<BinaryOperator>(&I)), false) + ")";
        else if (I.getOpcode() == Instruction::FRem) {
            if (I.getType() == Type::getFloatTy(I.getContext()))
                vout += "fmodf(";
            else if (I.getType() == Type::getDoubleTy(I.getContext()))
                vout += "fmod(";
            else  // all 3 flavors of long double
                vout += "fmodl(";
            vout += printOperand(thisp, I.getOperand(0), false) + ", "
                 + printOperand(thisp, I.getOperand(1), false) + ")";
        } else
            vout += printOperand(thisp, I.getOperand(0), false)
                 + " " + intmapLookup(opcodeMap, I.getOpcode()) + " "
                 + printOperand(thisp, I.getOperand(1), false);
        break;

    // Memory instructions...
    case Instruction::Store: {
        StoreInst &IS = static_cast<StoreInst&>(I);
        ERRORIF (IS.isVolatile());
        std::string pdest = printOperand(thisp, IS.getPointerOperand(), true);
        Value *Operand = I.getOperand(0);
        Constant *BitMask = 0;
        IntegerType* ITy = dyn_cast<IntegerType>(Operand->getType());
        if (ITy && !ITy->isPowerOf2ByteWidth())
            BitMask = ConstantInt::get(ITy, ITy->getBitMask());
        std::string sval = printOperand(thisp, Operand, false);
        if (!strncmp(pdest.c_str(), "*((0x", 5)) {
            char *endptr = NULL;
            void *pint = (void *)strtol(pdest.c_str()+5, &endptr, 16);
            std::string pname = mapAddress(pint);
            if (strncmp(pname.c_str(), "0x", 2) && !strcmp(endptr, "))"))
                pdest = pname;
        }
        if (pdest.length() > 2 && pdest[0] == '(' && pdest[pdest.length()-1] == ')')
            pdest = pdest.substr(1, pdest.length() -2);
        if (generateRegion == ProcessHoist)
            break;
        vout += pdest + ((generateRegion == ProcessVerilog) ? " <= " : " = ");
        if (BitMask)
            vout += "((";
        if (void *valp = nameMap[sval]) {
            //printf("[%s:%d] storeval %s found %p\n", __FUNCTION__, __LINE__, sval.c_str(), valp);
            sval = mapAddress(valp);
        }
        vout += sval;
        if (BitMask)
            vout += ") & " + printOperand(thisp, BitMask, false) + ")";
        break;
        }

    // Convert instructions...
    case Instruction::SExt:
    case Instruction::FPTrunc: case Instruction::FPExt:
    case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::UIToFP: case Instruction::SIToFP:
    case Instruction::IntToPtr: case Instruction::PtrToInt:
    case Instruction::AddrSpaceCast:
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::BitCast:
        vout += fetchOperand(thisp, I.getOperand(0), false);
        break;

    // Other instructions...
    case Instruction::ICmp: case Instruction::FCmp: {
        ICmpInst &CI = static_cast<ICmpInst&>(I);
        vout += printOperand(thisp, I.getOperand(0), false)
             + " " + intmapLookup(predText, CI.getPredicate()) + " "
             + printOperand(thisp, I.getOperand(1), false);
        break;
        }
    case Instruction::Call:
        vout += printCall(thisp, I);
        break;
#if 0
    case Instruction::Br:
        {
        if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
          char temp[MAX_CHAR_BUFFER];
          const BranchInst &BI(cast<BranchInst>(I));
          prepareOperand(BI.getCondition());
          int cond_item = getLocalSlot(BI.getCondition());
          sprintf(temp, "%s" SEPARATOR "%s_cond", globalName.c_str(), I.getParent()->getName().str().c_str());
          sprintf(vout, "%s = %s\n", temp, slotarray[cond_item].name);
          prepareOperand(BI.getSuccessor(0));
          prepareOperand(BI.getSuccessor(1));
        } else if (isa<IndirectBrInst>(I)) {
          for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i) {
            prepareOperand(I.getOperand(i));
          }
        }
        }
        break;
    //case Instruction::Switch:
        //const SwitchInst& SI(cast<SwitchInst>(I));
        //prepareOperand(SI.getCondition());
        //prepareOperand(SI.getDefaultDest());
        //for (SwitchInst::ConstCaseIt i = SI.case_begin(), e = SI.case_end(); i != e; ++i) {
          //prepareOperand(i.getCaseValue());
          //prepareOperand(i.getCaseSuccessor());
        //}
    case Instruction::PHI:
        {
        char temp[MAX_CHAR_BUFFER];
        const PHINode *PN = dyn_cast<PHINode>(&I);
        if (!PN) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        I.getType()->dump();
        sprintf(temp, "%s" SEPARATOR "%s_phival", globalName.c_str(), I.getParent()->getName().str().c_str());
        vout += temp + " = ";
        for (unsigned op = 0, Eop = PN->getNumIncomingValues(); op < Eop; ++op) {
            int valuein = getLocalSlot(PN->getIncomingValue(op));
            prepareOperand(PN->getIncomingValue(op));
            prepareOperand(PN->getIncomingBlock(op));
            TerminatorInst *TI = PN->getIncomingBlock(op)->getTerminator();
            //printf("[%s:%d] terminator\n", __FUNCTION__, __LINE__);
            //TI->dump();
            const BranchInst *BI = dyn_cast<BranchInst>(TI);
            const char *trailch = "";
            if (isa<BranchInst>(TI) && cast<BranchInst>(TI)->isConditional()) {
              prepareOperand(BI->getCondition());
              int cond_item = getLocalSlot(BI->getCondition());
              sprintf(temp, "%s ?", slotarray[cond_item].name);
              trailch = ":";
              //prepareOperand(BI->getSuccessor(0));
              //prepareOperand(BI->getSuccessor(1));
              vout += temp;
            }
            if (slotarray[valuein].name)
                sprintf(temp, "%s %s", slotarray[valuein].name, trailch);
            else
                sprintf(temp, "%lld %s", (long long)slotarray[valuein].offset, trailch);
            vout += temp;
        }
        }
        break;
#endif
    default:
        printf("COther opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
        break;
    }
    return vout;
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
    if (generateRegion == ProcessVerilog && !strncmp(&globalName.c_str()[globalName.length() - 6], "3ENAEv", 9)) {
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
    for (auto AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
        int slotindex = getLocalSlot(AI);
        ERRORIF (AI->hasByValAttr());
        (AI->getName().str().c_str());
    }
#endif
    /* Generate Verilog for all instructions.  Record function calls for post processing */
    if (generateRegion == ProcessCPP) {
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
    nameMap["Vthis"] = work.thisp;
    for (Function::iterator BB = func->begin(), E = func->end(); BB != E; ++BB) {
        if (trace_translate && BB->hasName())         // Print out the label if it exists...
            printf("LLLLL: %s\n", BB->getName().str().c_str());
        for (BasicBlock::iterator ins = BB->begin(), ins_end = BB->end(); ins != ins_end;) {

            BasicBlock::iterator next_ins = std::next(BasicBlock::iterator(ins));
            if (!isInlinableInst(*ins)) {
                if (trace_translate && generateRegion == ProcessCPP)
                    printf("/*before %p opcode %d.=%s*/\n", &*ins, ins->getOpcode(), ins->getOpcodeName());
                std::string vout = processInstruction(work.thisp, *ins);
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
                    if (generateRegion == ProcessCPP) {
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
    if (generateRegion == ProcessCPP)
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
    //structWork.clear();
    // Walk the rule lists for all modules, generating work items
    for (RULE_INFO *info : ruleInfo) {
        printf("RULE_INFO: rule %s thisp %p, RDY %p ENA %p\n", info->name, info->thisp, info->RDY, info->ENA);
        pushWork(info->RDY, (Function ***)info->thisp, 1);
        pushWork(info->ENA, (Function ***)info->thisp, 1);
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
        for (auto I = Ty->subtype_begin(), E = Ty->subtype_end(); I != E; ++I)
            printContainedStructs(*I, OStr, ODir);
        if (STy) {
            if (STy->getName() == "class.Module")
                return;  // just a dummy class
            for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
                printContainedStructs(*I, OStr, ODir);
            if (generateRegion == ProcessVerilog)
                generateModuleDef(STy, ODir);
            else if (generateRegion == ProcessCPP)
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
    generateRegion = ProcessHoist;
    processRules(OutNull, OutNull, OutNull);

    // Generate verilog for all rules
    generateRegion = ProcessVerilog;
    fprintf(OutVMain, "module top(input CLK, input nRST);\n  always @( posedge CLK) begin\n    if (!nRST) then begin\n    end\n    else begin\n");
    processRules(OutVMain, OutNull, OutNull);
    fprintf(OutVMain, "    end; // nRST\n  end; // always @ (posedge CLK)\nendmodule \n\n");
    generateStructs(NULL, OutDirectory);
    generateVerilogHeader(*Mod, OutVInstance, OutNull);

    // Generate cpp code for all rules
    generateRegion = ProcessCPP;
    generateCppData(Out, *Mod);
    processRules(Out, OutNull, OutHeader);
    UnnamedStructIDs.clear();
    generateStructs(Out, ""); // generate class method bodies
    generateRegion = ProcessClass;
    generateStructs(OutHeader, ""); // generate class definitions
    return false;
}
