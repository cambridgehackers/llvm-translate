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
#include "llvm/IR/CallSite.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

using namespace llvm;

#include "declarations.h"

static int trace_call;//=1;
int trace_translate;//=1;
static int trace_gep;//=1;
std::map<Function *, Function *> ruleRDYFunction;
std::map<const StructType *,ClassMethodTable *> classCreate;
static unsigned NextTypeID;
static int generateRegion = ProcessNone;
static Function *currentFunction;

std::list<Function *> vtableWork;
static std::map<const Type *, int> structMap;
static DenseMap<const Value*, unsigned> AnonValueNumbers;
static unsigned NextAnonValueNumber;
static DenseMap<const StructType*, unsigned> UnnamedStructIDs;
static std::string processInstruction(Instruction &I);
std::list<std::string> readList, writeList, invokeList, functionList;
std::map<std::string, std::string> storeList;

static INTMAP_TYPE predText[] = {
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
static INTMAP_TYPE opcodeMap[] = {
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
    if (isa<CallInst>(I) && generateRegion == ProcessCPP)
        return false; // needed to force guardedValue reads before Action calls
    if (isa<CmpInst>(I) || isa<LoadInst>(I))
        return true;
    if (I.getType() == Type::getVoidTy(I.getContext()) || !I.hasOneUse()
      || isa<TerminatorInst>(I) || isa<PHINode>(I)
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

int inheritsModule(const StructType *STy, const char *name)
{
    if (STy && STy->hasName()) {
        std::string sname = STy->getName();
        if (sname == name)
            return 1;
        int Idx = 0;
        for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
            std::string fname = fieldName(STy, Idx);
            if (fname == "" && inheritsModule(dyn_cast<StructType>(*I), name))
                return 1;
        }
    }
    return 0;
}

/*
 * Name functions
 */
std::string getStructName(const StructType *STy)
{
    assert(STy);
    if (!classCreate[STy])
        classCreate[STy] = new ClassMethodTable;
    if (!STy->isLiteral() && !STy->getName().empty())
        return CBEMangle("l_"+STy->getName().str());
    if (!UnnamedStructIDs[STy])
        UnnamedStructIDs[STy] = NextTypeID++;
    return "l_unnamed_" + utostr(UnnamedStructIDs[STy]);
}

std::string GetValueName(const Value *Operand)
{
    const GlobalAlias *GA = dyn_cast<GlobalAlias>(Operand);
    const Value *V;
    if (GA && (V = GA->getAliasee()))
        Operand = V;
    if (const GlobalValue *GV = dyn_cast<GlobalValue>(Operand))
        return CBEMangle(GV->getName());
    std::string Name = Operand->getName();
    if (Name.empty()) { // Assign unique names to local temporaries.
        unsigned &No = AnonValueNumbers[Operand];
        if (No == 0)
            No = ++NextAnonValueNumber;
        Name = "tmp__" + utostr(No);
    }
    std::string VarName;
    for (auto charp = Name.begin(), E = Name.end(); charp != E; ++charp) {
        char ch = *charp;
        if (isalnum(ch) || ch == '_')
            VarName += ch;
        else {
            char buffer[5];
            sprintf(buffer, "_%x_", ch);
            VarName += buffer;
        }
    }
    return VarName;
}

const StructType *findThisArgumentType(const PointerType *PTy)
{
    if (PTy)
    if (const FunctionType *func = dyn_cast<FunctionType>(PTy->getElementType()))
    if (func->getNumParams() > 0)
    if ((PTy = dyn_cast<PointerType>(func->getParamType(0))))
    if (const StructType *STy = dyn_cast<StructType>(PTy->getElementType())) {
        getStructName(STy);
        return STy;
    }
    return NULL;
}

/*
 * Output types
 */
std::string printType(const Type *Ty, bool isSigned, std::string NameSoFar, std::string prefix, std::string postfix, bool ptr)
{
    std::string sep = "", cbuffer = prefix, sp = (isSigned?"signed":"unsigned");
    switch (Ty->getTypeID()) {
    case Type::VoidTyID:
        if (generateRegion == ProcessVerilog)
            cbuffer += "VERILOG_void " + NameSoFar;
        else
            cbuffer += "void " + NameSoFar;
        break;
    case Type::IntegerTyID: {
        unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
        assert(NumBits <= 128 && "Bit widths > 128 not implemented yet");
        if (generateRegion == ProcessVerilog) {
        if (NumBits == 1)
            cbuffer += "VERILOG_bool";
        else if (NumBits <= 8) {
            if (ptr)
                cbuffer += sp + " VERILOG_char";
            else
                cbuffer += "reg";
        }
        else if (NumBits <= 16)
            cbuffer += sp + " VERILOG_short";
        else if (NumBits <= 32)
            //cbuffer += sp + " VERILOG_int";
            cbuffer += "reg" + verilogArrRange(Ty);
        else if (NumBits <= 64)
            cbuffer += sp + " VERILOG_long long";
        }
        else {
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
        }
    case Type::FunctionTyID: {
        const FunctionType *FTy = cast<FunctionType>(Ty);
        std::string tstr = " (" + NameSoFar + ") (";
        for (auto I = FTy->param_begin(), E = FTy->param_end(); I != E; ++I) {
            tstr += printType(*I, /*isSigned=*/false, "", sep, "", false);
            sep = ", ";
        }
        if (generateRegion == ProcessVerilog) {
        if (FTy->isVarArg()) {
            if (!FTy->getNumParams())
                tstr += " VERILOG_int"; //dummy argument for empty vaarg functs
            tstr += ", ...";
        } else if (!FTy->getNumParams())
            tstr += "VERILOG_void";
        }
        else {
        if (FTy->isVarArg()) {
            if (!FTy->getNumParams())
                tstr += " int"; //dummy argument for empty vaarg functs
            tstr += ", ...";
        } else if (!FTy->getNumParams())
            tstr += "void";
        }
        cbuffer += printType(FTy->getReturnType(), /*isSigned=*/false, tstr + ')', "", "", false);
        break;
        }
    case Type::StructTyID:
        if (generateRegion == ProcessVerilog)
            cbuffer += "VERILOG_class " + getStructName(cast<StructType>(Ty)) + " " + NameSoFar;
        else
            cbuffer += "class " + getStructName(cast<StructType>(Ty)) + " " + NameSoFar;
        break;
    case Type::ArrayTyID: {
        const ArrayType *ATy = cast<ArrayType>(Ty);
        unsigned len = ATy->getNumElements();
        if (len == 0) len = 1;
        if (generateRegion == ProcessVerilog) {
        }
        cbuffer += printType(ATy->getElementType(), false, "", "", "", false) + NameSoFar + "[" + utostr(len) + "]";
        break;
        }
    case Type::PointerTyID: {
        const PointerType *PTy = cast<PointerType>(Ty);
        std::string ptrName = "*" + NameSoFar;
        if (PTy->getElementType()->isArrayTy() || PTy->getElementType()->isVectorTy())
            ptrName = "(" + ptrName + ")";
        cbuffer += printType(PTy->getElementType(), false, ptrName, "", "", true);
        if (generateRegion == ProcessVerilog) {
        }
        break;
        }
    default:
        llvm_unreachable("Unhandled case in printType!");
    }
    cbuffer += postfix;
    return cbuffer;
}

/*
 * GEP and Load instructions interpreter functions
 * (just execute using the memory areas allocated by the constructors)
 */
static std::string printGEPExpression(Value *Ptr, gep_type_iterator I, gep_type_iterator E)
{
    std::string cbuffer, sep = " ", amper = "&";
    PointerType *PTy;
    ConstantDataArray *CPA;
    uint64_t Total = 0;
    VectorType *LastIndexIsVector = 0;
    const DataLayout *TD = EE->getDataLayout();
    Constant *FirstOp = dyn_cast<Constant>(I.getOperand());
    bool expose = isAddressExposed(Ptr);
    std::string referstr = printOperand(Ptr, false);

    for (auto TmpI = I; TmpI != E; ++TmpI) {
        LastIndexIsVector = dyn_cast<VectorType>(*TmpI);
        const ConstantInt *CI = cast<ConstantInt>(TmpI.getOperand());
        if (StructType *STy = dyn_cast<StructType>(*TmpI))
            Total += TD->getStructLayout(STy)->getElementOffset(CI->getZExtValue());
        else {
            ERRORIF(isa<GlobalValue>(TmpI.getOperand()));
            Total += TD->getTypeAllocSize(cast<SequentialType>(*TmpI)->getElementType()) * CI->getZExtValue();
        }
    }
    if (LastIndexIsVector)
        cbuffer += printType(PointerType::getUnqual(LastIndexIsVector->getElementType()), false, "", "((", ")(", false);
    if (trace_gep)
        printf("[%s:%d] referstr %s Total %ld\n", __FUNCTION__, __LINE__, referstr.c_str(), (unsigned long)Total);
    if (I == E)
        return referstr;
    ClassMethodTable *table;
    if ((PTy = dyn_cast<PointerType>(Ptr->getType()))
     && (PTy = dyn_cast<PointerType>(PTy->getElementType()))
     && (table = classCreate[findThisArgumentType(PTy)])) {
        // Lookup method index in vtable
        referstr = lookupMethodName(table, Total/sizeof(void *));
        if (trace_gep)
            printf("%s: Method invocation referstr %s\n", __FUNCTION__, referstr.c_str());
        I = E; // skip post processing
    }
    else if (FirstOp && FirstOp->isNullValue()) {
        ++I;  // Skip the zero index.
        if (I != E && (*I)->isArrayTy())
            if (const ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand())) {
                uint64_t val = CI->getZExtValue();
                if (GlobalVariable *globalVar = dyn_cast<GlobalVariable>(Ptr))
                if (globalVar && !globalVar->getInitializer()->isNullValue()
                 && (CPA = dyn_cast<ConstantDataArray>(globalVar->getInitializer()))) {
                    ERRORIF(val || !CPA->isString());
                    referstr = printString(CPA->getAsString());
                }
                if (val)
                    referstr += '+' + utostr(val);
                amper = "";
                if (trace_gep)
                    printf("[%s:%d] expose %d referstr %s\n", __FUNCTION__, __LINE__, expose, referstr.c_str());
                ++I;     // we processed this index
            }
    }
    cbuffer += amper;
    for (; I != E; ++I) {
        if (StructType *STy = dyn_cast<StructType>(*I)) {
            uint64_t foffset = cast<ConstantInt>(I.getOperand())->getZExtValue();
            std::string dot = MODULE_DOT;
            std::string arrow = MODULE_ARROW;
            std::string fname = fieldName(STy, foffset);
            if (inheritsModule(dyn_cast<StructType>(STy->element_begin()[foffset]), "class.InterfaceClass"))
                fname += '_';
            if (trace_gep)
                printf("[%s:%d] expose %d referstr %s cbuffer %s STy %s fname %s\n", __FUNCTION__, __LINE__, expose, referstr.c_str(), cbuffer.c_str(), STy->getName().str().c_str(), fname.c_str());
            if (!expose && referstr[0] == '&') {
                expose = true;
                referstr = referstr.substr(1);
            }
            if (expose)
                referstr += dot;
            else if (referstr == "this")
                referstr = "";
            else
                referstr += arrow;
            cbuffer += referstr + fname;
        }
        else {
            if (trace_gep)
                printf("[%s:%d] expose %d referstr %s cbuffer %s\n", __FUNCTION__, __LINE__, expose, referstr.c_str(), cbuffer.c_str());
            cbuffer += referstr;
            if ((*I)->isArrayTy() || !(*I)->isVectorTy())
                cbuffer += "[" + printOperand(I.getOperand(), false) + "]";
            else {
                if (!isa<Constant>(I.getOperand()) || !cast<Constant>(I.getOperand())->isNullValue())
                    cbuffer += ")+(" + printOperand(I.getOperand(), false);
                cbuffer += "))";
            }
        }
        referstr = "";
    }
    cbuffer += referstr;
    if (trace_gep)
        printf("%s: return %s\n", __FUNCTION__, cbuffer.c_str());
    return cbuffer;
}

std::string printOperand(Value *Operand, bool Indirect)
{
    std::string cbuffer;
    if (!Operand)
        return "";
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
        prefix = "&";  // Global variables are referenced as their addresses by llvm
    if (I && isInlinableInst(*I)) {
        std::string p = processInstruction(*I);
        if (prefix == "")
            cbuffer += p;
        else if (prefix == "*" && p[0] == '&') {
            prefix = "";
            p = p.substr(1);
            cbuffer += p;
        }
        else
            cbuffer += prefix + "(" + p + ")";
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
                cbuffer += printGEPExpression(CE->getOperand(0), gep_type_begin(CPV), gep_type_end(CPV)) +  ")";
            }
            else if (ConstantInt *CI = dyn_cast<ConstantInt>(CPV)) {
                char temp[100];
                Type* Ty = CI->getType();
                temp[0] = 0;
                if (Ty == Type::getInt1Ty(CPV->getContext()))
                    cbuffer += CI->getZExtValue() ? "1" : "0";
                else if (Ty == Type::getInt32Ty(CPV->getContext()) || Ty->getPrimitiveSizeInBits() > 32)
                    sprintf(temp, "%ld", CI->getZExtValue());
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
    return cbuffer;
}

static std::string printCall(Instruction &I)
{
    Function *callingFunction = I.getParent()->getParent();
    std::string vout, sep = "";
    int skip = 1;
    CallInst &ICL = static_cast<CallInst&>(I);
    Function *func = ICL.getCalledFunction();
    CallSite CS(&I);
    CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
    std::string pcalledFunction = printOperand(*AI, false);
    std::string prefix = MODULE_ARROW;

    if (!func)
        func = EE->FindFunctionNamed(pcalledFunction.c_str());
    if (trace_call)
        printf("CALL: CALLER %s func %s pcalledFunction '%s'\n", callingFunction->getName().str().c_str(), func->getName().str().c_str(), pcalledFunction.c_str());
    if (!func) {
        printf("%s: not an instantiable call!!!! %s\n", __FUNCTION__, pcalledFunction.c_str());
        exit(-1);
    }
    Function::const_arg_iterator FAI = func->arg_begin();
    bool thisInterface = inheritsModule(findThisArgumentType(func->getType()), "class.InterfaceClass");
    if (pcalledFunction[0] == '&') {
        pcalledFunction = pcalledFunction.substr(1);
        prefix = thisInterface ? "" : ".";
    }
    if (generateRegion == ProcessVerilog)
        prefix = pcalledFunction + prefix;
    std::string mname = prefix + getMethodName(func->getName());
    if (generateRegion == ProcessVerilog) {
        if (func->getReturnType() == Type::getVoidTy(func->getContext()))
            muxEnable(mname + "__ENA");
        else
            vout += mname;
        invokeList.push_back(mname);
    }
    else
        vout += pcalledFunction + mname + "(";
    for (; AI != AE; ++AI, FAI++) {
        if (!skip) {
            std::string parg = printOperand(*AI, false);
            if (generateRegion == ProcessVerilog)
                muxValue(mname + "_" + FAI->getName().str(), parg);
            else
                vout += sep + parg;
            sep = ", ";
        }
        skip = 0;
    }
    if (generateRegion != ProcessVerilog)
        vout += ")";
    return vout;
}

std::string parenOperand(Value *Operand)
{
    std::string temp = printOperand(Operand, false);
    for (auto ch: temp)
        if (!isalnum(ch) && ch != '$' && ch != '_')
            return "(" + temp + ")";
    return temp;
}

/*
 * Output instructions
 */
static std::string processInstruction(Instruction &I)
{
    std::string vout;
    int opcode = I.getOpcode();
//printf("[%s:%d] op %s\n", __FUNCTION__, __LINE__, I.getOpcodeName());
    switch(opcode) {
    case Instruction::Call:
        vout += printCall(I);
        break;
    case Instruction::GetElementPtr: {
        GetElementPtrInst &IG = static_cast<GetElementPtrInst&>(I);
        return printGEPExpression(IG.getPointerOperand(), gep_type_begin(IG), gep_type_end(IG));
        }
    case Instruction::Load: {
        LoadInst &IL = static_cast<LoadInst&>(I);
        ERRORIF (IL.isVolatile());
        std::string p = printOperand(I.getOperand(0), true);
        if (I.getType()->getTypeID() != Type::PointerTyID)
            readList.push_back(p);
        return p;
        }

    // Memory instructions...
    case Instruction::Store: {
        StoreInst &IS = static_cast<StoreInst&>(I);
        ERRORIF (IS.isVolatile());
        std::string pdest = printOperand(IS.getPointerOperand(), true);
        Value *Operand = I.getOperand(0);
        Constant *BitMask = 0;
        IntegerType* ITy = dyn_cast<IntegerType>(Operand->getType());
        if (ITy && !ITy->isPowerOf2ByteWidth())
            BitMask = ConstantInt::get(ITy, ITy->getBitMask());
        std::string sval = printOperand(Operand, false);
        writeList.push_back(pdest);
        if (BitMask)
            sval = "((" + sval + ") & " + parenOperand(BitMask) + ")";
        storeList[pdest] = sval;
        return "";
        }

    // Terminators
    case Instruction::Ret:
        if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
            if (generateRegion == ProcessCPP)
                vout += "return ";
            if (I.getNumOperands())
                vout += printOperand(I.getOperand(0), false);
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
            vout += "-(" + printOperand(BinaryOperator::getNegArgument(cast<BinaryOperator>(&I)), false) + ")";
        else if (BinaryOperator::isFNeg(&I))
            vout += "-(" + printOperand(BinaryOperator::getFNegArgument(cast<BinaryOperator>(&I)), false) + ")";
        else if (I.getOpcode() == Instruction::FRem) {
            if (I.getType() == Type::getFloatTy(I.getContext()))
                vout += "fmodf(";
            else if (I.getType() == Type::getDoubleTy(I.getContext()))
                vout += "fmod(";
            else  // all 3 flavors of long double
                vout += "fmodl(";
            vout += printOperand(I.getOperand(0), false) + ", "
                 + printOperand(I.getOperand(1), false) + ")";
        } else
            vout += parenOperand(I.getOperand(0))
                 + " " + intmapLookup(opcodeMap, I.getOpcode()) + " "
                 + parenOperand(I.getOperand(1));
        break;

    // Convert instructions...
    case Instruction::SExt:
    case Instruction::FPTrunc: case Instruction::FPExt:
    case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::UIToFP: case Instruction::SIToFP:
    case Instruction::IntToPtr: case Instruction::PtrToInt:
    case Instruction::AddrSpaceCast:
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::BitCast:
        vout += printOperand(I.getOperand(0), false);
        break;

    // Other instructions...
    case Instruction::ICmp: case Instruction::FCmp: {
        ICmpInst &CI = static_cast<ICmpInst&>(I);
        vout += parenOperand(I.getOperand(0))
             + " " + intmapLookup(predText, CI.getPredicate()) + " "
             + parenOperand(I.getOperand(1));
        break;
        }
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
        printf("Other opcode %d.=%s\n", opcode, I.getOpcodeName());
        I.getParent()->getParent()->dump();
        exit(1);
        break;
    }
    return vout;
}

/*
 * Walk all BasicBlocks for a Function, calling requested processing function
 */
void processFunction(Function *func)
{
    std::string fname = func->getName();
    currentFunction = func;

    NextAnonValueNumber = 0;
    readList.clear();
    writeList.clear();
    invokeList.clear();
    storeList.clear();
    functionList.clear();
    if (trace_call)
        printf("PROCESSING %s\n", fname.c_str());
    /* Generate cpp/Verilog for all instructions.  Record function calls for post processing */
    for (auto BI = func->begin(), BE = func->end(); BI != BE; ++BI) {
        if (trace_translate && BI->hasName())         // Print out the label if it exists...
            printf("LLLLL: %s\n", BI->getName().str().c_str());
        for (auto II = BI->begin(), IE = BI->end(); II != IE;) {
            auto INEXT = std::next(BasicBlock::iterator(II));
            if (INEXT == IE || !isInlinableInst(*II)) {
                std::string vout = processInstruction(*II);
                if (vout != "") {
                    if (!isDirectAlloca(&*II) && II->use_begin() != II->use_end()
                         && II->getType() != Type::getVoidTy(BI->getContext())) {
                        std::string resname = GetValueName(&*II);
                        if (generateRegion == ProcessCPP)
                            resname = printType(II->getType(), false, resname, "", "", false);
                        storeList[resname] = vout;
                    }
                    else
                        functionList.push_back(vout);
                }
            }
            II = INEXT;
        }
    }
}

static void printContainedStructs(const Type *Ty, FILE *OStr, std::string ODir, GEN_HEADER cb)
{
    if (!Ty)
        return;
    if (const PointerType *PTy = dyn_cast<PointerType>(Ty)) {
        if (const StructType *subSTy = dyn_cast<StructType>(PTy->getElementType()))
            printContainedStructs(subSTy, OStr, ODir, cb);
    }
    else if (!structMap[Ty]) {
        structMap[Ty] = 1;
        for (auto I = Ty->subtype_begin(), E = Ty->subtype_end(); I != E; ++I)
            printContainedStructs(*I, OStr, ODir, cb);
        if (const StructType *STy = dyn_cast<StructType>(Ty))
        if (STy->hasName()) {
            if (!strncmp(STy->getName().str().c_str(), "class.std::", 11)
             || !strncmp(STy->getName().str().c_str(), "struct.std::", 12))
                return;   // don't generate anything for std classes
            ClassMethodTable *table = classCreate[STy];
            int Idx = 0;
            for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
                printContainedStructs(*I, OStr, ODir, cb);
            }
            Idx = 0;
            for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
                const Type *element = *I;
                if (table && table->replaceType[Idx])
                    element = table->replaceType[Idx];
                printContainedStructs(element, OStr, ODir, cb);
            }
            if (table)
                cb(STy, OStr, ODir);
        }
    }
}
static void generateStructs(FILE *OStr, std::string oDir, GEN_HEADER cb)
{
    structMap.clear();
    for (auto current : classCreate)
        printContainedStructs(current.first, OStr, oDir, cb);
}

bool GenerateRunOnModule(Module *Mod, std::string OutDirectory)
{
    // Construct the vtable map for classes, cleanup IR, build vtableWork function list
    preprocessModule(Mod);

    // run Constructors from user program
    EE->runStaticConstructorsDestructors(false);

    // Construct the address -> symbolic name map using actual data allocated/initialized
    constructAddressMap(Mod);

    // Generate verilog for all rules
    generateRegion = ProcessVerilog;
    generateStructs(NULL, OutDirectory, generateModuleDef);

    // Generate cpp code for all rules
    generateRegion = ProcessCPP;
    generateStructs(fopen((OutDirectory + "/output.cpp").c_str(), "w"),
        "", generateClassBody); // generate class method bodies
    generateStructs(fopen((OutDirectory + "/output.h").c_str(), "w"),
        "", generateClassDef); // generate class definitions
    return false;
}
