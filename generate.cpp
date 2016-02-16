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
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

#include "declarations.h"

static int trace_call;//=1;
static int trace_gep;//=1;
std::map<Function *, Function *> ruleRDYFunction;
std::map<const StructType *,ClassMethodTable *> classCreate;
static unsigned NextTypeID;
static int generateRegion = ProcessNone;

static std::map<const Type *, int> structMap;
static std::map<const Value *, std::string> allocaMap;
static DenseMap<const Value*, unsigned> AnonValueNumbers;
static unsigned NextAnonValueNumber;
static DenseMap<const StructType*, unsigned> UnnamedStructIDs;
std::list<ReferenceType> readList, writeList, invokeList, functionList;
std::map<std::string, ReferenceType> storeList;
std::map<std::string, std::string> declareList;

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
    if (I.getType() == Type::getVoidTy(I.getContext())
// || !I.hasOneUse()
      || isa<TerminatorInst>(I)
      || isa<VAArgInst>(I) || isa<InsertElementInst>(I)
      || isa<InsertValueInst>(I) || isa<AllocaInst>(I))
        return false;
    if (I.hasOneUse()) {
        const Instruction &User = cast<Instruction>(*I.user_back());
        if (isa<ExtractElementInst>(User) || isa<ShuffleVectorInst>(User))
            return false;
        // doesn't seem to work for PHI references isa<PHINode>(I)
        //ERRORIF (I.getParent() != cast<Instruction>(I.user_back())->getParent());
    }
    return true;
}
static const AllocaInst *isDirectAlloca(const Value *V)
{
    const AllocaInst *AA = dyn_cast<AllocaInst>(V);
    if (!AA || AA->isArrayAllocation()
     || AA->getParent() != &AA->getParent()->getParent()->getEntryBlock())
        return 0;
    return AA;
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
    if (generateRegion == ProcessVerilog)
        VarName = allocaMap[Operand];
    if (VarName == "")
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

static const StructType *findThisArgumentType(const PointerType *PTy, int ind)
{
    if (PTy)
    if (const FunctionType *func = dyn_cast<FunctionType>(PTy->getElementType()))
    if (func->getNumParams() > 0)
    if ((PTy = dyn_cast<PointerType>(func->getParamType(ind))))
    if (const StructType *STy = dyn_cast<StructType>(PTy->getElementType())) {
        getStructName(STy);
        return STy;
    }
    return NULL;
}
const StructType *findThisArgument(Function *func)
{
    return findThisArgumentType(func->getType(), func->hasStructRetAttr());
}

/*
 * Output type declarations.  Note that each case in the switch statement
 * is different for verilog and cpp.
 */
std::string printType(Type *Ty, bool isSigned, std::string NameSoFar, std::string prefix, std::string postfix, bool ptr)
{
    std::string sep, cbuffer = prefix, sp = (isSigned?"signed":"unsigned");
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
        FunctionType *FTy = cast<FunctionType>(Ty);
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
    case Type::StructTyID: {
        const StructType *STy = cast<StructType>(Ty);
        if (inheritsModule(STy, "class.BitsClass")) {
            ClassMethodTable *table = classCreate[STy];
            cbuffer += "BITS" + table->instance + " " + NameSoFar;
        }
        else
            cbuffer += getStructName(STy) + " " + NameSoFar;
        break;
        }
    case Type::ArrayTyID: {
        ArrayType *ATy = cast<ArrayType>(Ty);
        unsigned len = ATy->getNumElements();
        if (len == 0) len = 1;
        if (generateRegion == ProcessVerilog) {
        }
        cbuffer += printType(ATy->getElementType(), false, "", "", "", false) + NameSoFar + "[" + utostr(len) + "]";
        break;
        }
    case Type::PointerTyID: {
        PointerType *PTy = cast<PointerType>(Ty);
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
int64_t getGEPOffset(VectorType **LastIndexIsVector, gep_type_iterator I, gep_type_iterator E)
{
    uint64_t Total = 0;
    const DataLayout *TD = EE->getDataLayout();

    for (auto TmpI = I; TmpI != E; ++TmpI) {
        *LastIndexIsVector = dyn_cast<VectorType>(*TmpI);
        if (const ConstantInt *CI = dyn_cast<ConstantInt>(TmpI.getOperand())) {
            if (StructType *STy = dyn_cast<StructType>(*TmpI))
                Total += TD->getStructLayout(STy)->getElementOffset(CI->getZExtValue());
            else {
                ERRORIF(isa<GlobalValue>(TmpI.getOperand()));
                Total += TD->getTypeAllocSize(cast<SequentialType>(*TmpI)->getElementType()) * CI->getZExtValue();
            }
        }
        else
            return -1;
    }
    return Total;
}

/*
 * Generate a string for the value represented by a GEP DAG
 */
static std::string printGEPExpression(Value *Ptr, gep_type_iterator I, gep_type_iterator E)
{
    std::string cbuffer, sep = " ", amper = "&";
    ClassMethodTable *table;
    PointerType *PTy;
    ConstantDataArray *CPA;
    int64_t Total = 0;
    VectorType *LastIndexIsVector = 0;
    Constant *FirstOp = dyn_cast<Constant>(I.getOperand());
    bool expose = isAddressExposed(Ptr);
    std::string referstr = printOperand(Ptr, false);

    Total = getGEPOffset(&LastIndexIsVector, I, E);
    if (LastIndexIsVector)
        cbuffer += printType(PointerType::getUnqual(LastIndexIsVector->getElementType()), false, "", "((", ")(", false);
    if (trace_gep)
        printf("[%s:%d] referstr %s Total %ld\n", __FUNCTION__, __LINE__, referstr.c_str(), (unsigned long)Total);
    if (Total == -1) {
        printf("[%s:%d] non-constant offset referstr %s Total %ld\n", __FUNCTION__, __LINE__, referstr.c_str(), (unsigned long)Total);
    }
    if (I == E)
        return referstr;
    if ((PTy = dyn_cast<PointerType>(Ptr->getType()))
     && (PTy = dyn_cast<PointerType>(PTy->getElementType()))
     && (table = classCreate[findThisArgumentType(PTy, 0)])) {
        // Lookup method index in vtable
        referstr = lookupMethodName(table, Total/sizeof(void *));
        //if (trace_gep)
            printf("%s: Method invocation referstr %s\n", __FUNCTION__, referstr.c_str());
        I = E; // skip post processing
    }
    else if (FirstOp && FirstOp->isNullValue()) {
        ++I;  // Skip the zero index.
        if (I == E) {
            // HACK HACK HACK HACK for 'fifo0'
            printf("[%s:%d] amper %s expose %d referstr %s\n", __FUNCTION__, __LINE__, amper.c_str(), expose, referstr.c_str());
            (*I)->dump();
            amper = "";
            referstr += "0";
        } else
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
            else
                arrow = MODULE_DOT;
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
            else {
                referstr = "(" + referstr + ")";
                referstr += arrow;
            }
            cbuffer += referstr + fname;
        }
        else {
            if (trace_gep)
                printf("[%s:%d] expose %d referstr %s cbuffer %s\n", __FUNCTION__, __LINE__, expose, referstr.c_str(), cbuffer.c_str());
            if ((*I)->isArrayTy() || !(*I)->isVectorTy()) {
                if (referstr[0] == '&')
                    referstr = referstr.substr(1);
                cbuffer += referstr;
                //cbuffer += "[" + printOperand(I.getOperand(), false) + "]";
                cbuffer += printOperand(I.getOperand(), false);
            }
            else {
                cbuffer += referstr;
                if (!isa<Constant>(I.getOperand()) || !cast<Constant>(I.getOperand())->isNullValue())
                    cbuffer += ")+(" + printOperand(I.getOperand(), false);
                cbuffer += "))";
            }
        }
        referstr = "";
    }
    cbuffer += referstr;
    if (trace_gep || Total == -1)
        printf("%s: return %s\n", __FUNCTION__, cbuffer.c_str());
    return cbuffer;
}

/*
 * Generate a string for a function/method call
 */
static std::string printCall(Instruction &I)
{
    Function *callingFunction = I.getParent()->getParent();
    std::string callingName = callingFunction->getName();
    std::string vout, sep, structRet;
    CallInst &ICL = static_cast<CallInst&>(I);
    Function *func = ICL.getCalledFunction();
    Type *retType = func->getReturnType();
    Value *structRetTemp = NULL;
    auto FAI = func->arg_begin();
    if (func->hasStructRetAttr()) {
        retType = cast<PointerType>(FAI->getType())->getElementType();
        FAI++;
    }
    CallSite CS(&I);
    CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
    if (func->hasStructRetAttr()) {
        structRetTemp = *AI;
        structRet = printOperand(*AI, dyn_cast<Argument>(*AI) == NULL); // get structure return area
        if (declareList[structRet] == "")
            declareList[structRet] = printType(cast<PointerType>((*AI)->getType())
                ->getElementType(), false, structRet, "", "", false);
        AI++;
    }
    std::string pcalledFunction = printOperand(*AI++, false); // skips 'this' param
    std::string prefix = MODULE_ARROW;

    if (!func) {
        printf("%s: not an instantiable call!!!! %s\n", __FUNCTION__, pcalledFunction.c_str());
        exit(-1);
    }
    std::string cgetName = func->getName();
    std::string fname = pushSeen[func];
    if (trace_call)
        printf("CALL: CALLER %s func %s[%p] pcalledFunction '%s' fname %s\n", callingName.c_str(), cgetName.c_str(), func, pcalledFunction.c_str(), fname.c_str());
    if (pcalledFunction[0] == '&') {
        pcalledFunction = pcalledFunction.substr(1);
        bool thisInterface = inheritsModule(findThisArgument(func), "class.InterfaceClass");
        prefix = thisInterface ? "" : ".";
    }
    if (generateRegion == ProcessVerilog)
        prefix = pcalledFunction + prefix;
    std::string mname = prefix + fname;
    Argument *calledRet = callingFunction->arg_begin();
    if (Instruction *dest = dyn_cast<Instruction>(I.getOperand(0)))
    if (dest->getOpcode() == Instruction::BitCast) {
    if (cgetName == "llvm.memcpy.p0i8.p0i8.i64") {
        if (Instruction *src = dyn_cast<Instruction>(I.getOperand(1)))
        if (src->getOpcode() == Instruction::BitCast) {
            std::string sval = printOperand(src->getOperand(0), dyn_cast<Argument>(src->getOperand(0)) == NULL);
            if (dyn_cast<Argument>(dest->getOperand(0)) == calledRet) {
                if (generateRegion == ProcessCPP)
                    vout += "return ";
                vout += sval;
            }
            else
                storeList[printOperand(dest->getOperand(0), true)] = ReferenceType{I.getParent(), sval};
            return vout;
        }
    }
    else if (cgetName == "llvm.memset.p0i8.i64") {
        if (dyn_cast<Argument>(dest->getOperand(0)) == calledRet) {
            if (generateRegion == ProcessCPP)
                return "return {0}";
            return "0";
        }
        else
            printf("[%s:%d] NOTARG memset\n", __FUNCTION__, __LINE__);
    }
    else
        printf("[%s:%d] not memcpy/memset %s\n", __FUNCTION__, __LINE__, cgetName.c_str());
    }
    if (generateRegion == ProcessVerilog) {
        if (retType == Type::getVoidTy(func->getContext()))
            muxEnable(I.getParent(), mname + "__ENA");
        else
            vout += mname;
        invokeList.push_back(ReferenceType{I.getParent(), mname});
    }
    else {
        vout += pcalledFunction + mname + "(";
    }
    for (FAI++; AI != AE; ++AI, FAI++) { // first param processed as pcalledFunction
        bool indirect = dyn_cast<PointerType>((*AI)->getType()) != NULL;
        if (auto *ins = dyn_cast<Instruction>(*AI)) {
            if (ins->getOpcode() == Instruction::GetElementPtr)
                indirect = true;
        }
        if (dyn_cast<Argument>(*AI))
            indirect = false;
        std::string parg = printOperand(*AI, indirect);
        if (generateRegion == ProcessVerilog)
            muxValue(I.getParent(), mname + "_" + FAI->getName().str(), parg);
        else
            vout += sep + parg;
        sep = ", ";
    }
    if (generateRegion != ProcessVerilog)
        vout += ")";
    if (structRet != "") {
        if (generateRegion == ProcessCPP)
            vout = structRet + " = " + vout;
        else {
            allocaMap[structRetTemp] = vout;
            vout = "";
        }
    }
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
 * Generate a string for any valid instruction DAG.
 */
static Function *processIFunction;
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
            readList.push_back(ReferenceType{I.getParent(), p});
        return p;
        }

    // Memory instructions...
    case Instruction::Store: {
        StoreInst &IS = static_cast<StoreInst&>(I);
        ERRORIF (IS.isVolatile());
        std::string pdest = printOperand(IS.getPointerOperand(), true);
        if (pdest[0] == '&')
            pdest = pdest.substr(1);
        Value *Operand = I.getOperand(0);
        Constant *BitMask = 0;
        IntegerType* ITy = dyn_cast<IntegerType>(Operand->getType());
        if (ITy && !ITy->isPowerOf2ByteWidth())
            BitMask = ConstantInt::get(ITy, ITy->getBitMask());
        std::string sval = printOperand(Operand, false);
        writeList.push_back(ReferenceType{I.getParent(), pdest});
        if (BitMask)
            sval = "((" + sval + ") & " + parenOperand(BitMask) + ")";
        storeList[pdest] = ReferenceType{I.getParent(), sval};
        return "";
        }

    // Terminators
    case Instruction::Ret:
        if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
            if (generateRegion == ProcessCPP)
                vout += "return ";
            if (I.getNumOperands())
                vout += printOperand(I.getOperand(0), false);
            if (generateRegion == ProcessCPP && processIFunction && processIFunction->hasStructRetAttr())
                vout += GetValueName(processIFunction->arg_begin());
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
    case Instruction::PHI: {
        const PHINode *PN = dyn_cast<PHINode>(&I);
        Value *prevCond = NULL;
        for (unsigned opIndex = 0, Eop = PN->getNumIncomingValues(); opIndex < Eop; opIndex++) {
            BasicBlock *inBlock = PN->getIncomingBlock(opIndex);
            Value *opCond = getCondition(inBlock, 0);
            if (opIndex != Eop - 1 || getCondition(inBlock, 1) != prevCond) {
                std::string cStr = printOperand(opCond, false);
                if (cStr != "")
                    vout += cStr + " ? ";
            }
            vout += printOperand(PN->getIncomingValue(opIndex), false);
            if (opIndex != Eop - 1)
                vout += ":";
            prevCond = opCond;
        }
        break;
        }
    case Instruction::Switch: {
        SwitchInst* SI = cast<SwitchInst>(&I);
        Value *switchIndex = SI->getCondition();
        //BasicBlock *defaultBB = SI->getDefaultDest();
        for (SwitchInst::CaseIt CI = SI->case_begin(), CE = SI->case_end(); CI != CE; ++CI) {
            BasicBlock *caseBB = CI.getCaseSuccessor();
            int64_t val = CI.getCaseValue()->getZExtValue();
            if (!getCondition(caseBB, 0)) { // 'true' condition
printf("[%s:%d] [%ld] = %s\n", __FUNCTION__, __LINE__, val, caseBB->getName().str().c_str());
                IRBuilder<> cbuilder(caseBB);
                setCondition(caseBB, 0,
                    cbuilder.CreateICmp(ICmpInst::ICMP_EQ, switchIndex,
                        ConstantInt::get(switchIndex->getType(), val)));
            }
        }
        break;
        }
    case Instruction::Br:
        break;
    case Instruction::Alloca: {
        std::string resname = GetValueName(&I);
        if (auto *PTy = dyn_cast<PointerType>(I.getType()))
            declareList[resname] = printType(PTy->getElementType(), false, resname, "", "", false);
        //printf("[%s:%d] ALLOCAA %s -> %s\n", __FUNCTION__, __LINE__, processIFunction->getName().str().c_str(), allocaMap[&I].c_str());
        break;
        }
#if 0
    case Instruction::ExtractValue: {
        ExtractValueInst *EI = cast<ExtractValueInst>(&I);
        //ExecutionContext &SF = ECStack.back();
        Value *Agg = EI->getAggregateOperand();
        GenericValue Dest, Src;// = getOperandValue(Agg, SF);
        ExtractValueInst::idx_iterator IdxBegin = EI->idx_begin();
        unsigned Num = EI->getNumIndices();
printf("[%s:%d] ExtractValue %d\n", __FUNCTION__, __LINE__, Num);
#if 0
        GenericValue *pSrc = &Src;
        for (unsigned i = 0 ; i < Num; ++i) {
            pSrc = &pSrc->AggregateVal[*IdxBegin];
            ++IdxBegin;
        }
        Type *IndexedType = ExtractValueInst::getIndexedType(Agg->getType(), EI->getIndices());
        switch (IndexedType->getTypeID()) {
        default:
            llvm_unreachable("Unhandled dest type for extractelement instruction");
            break;
        case Type::IntegerTyID:
            Dest.IntVal = pSrc->IntVal;
            break;
        case Type::FloatTyID:
            Dest.FloatVal = pSrc->FloatVal;
            break;
        case Type::DoubleTyID:
            Dest.DoubleVal = pSrc->DoubleVal;
            break;
        case Type::ArrayTyID:
        case Type::StructTyID:
        case Type::VectorTyID:
            Dest.AggregateVal = pSrc->AggregateVal;
            break;
        case Type::PointerTyID:
            Dest.PointerVal = pSrc->PointerVal;
            break;
        }
        //SetValue(EI->, Dest, SF);
#endif
        break;
        }
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
 * Generate a string for the value generated by an Instruction DAG
 */
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
        if (prefix == "*" && p[0] == '&') {
            prefix = "";
            p = p.substr(1);
        }
        if (prefix == "")
            cbuffer += p;
        else
            cbuffer += prefix + "(" + p + ")";
    }
    else if (I && I->getOpcode() == Instruction::Alloca)
        cbuffer += GetValueName(Operand);
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

/*
 * Walk all BasicBlocks for a Function, generating strings for Instructions
 * that are not 'isInlinableInst'.
 */
void processFunction(Function *func)
{
    NextAnonValueNumber = 0;
    readList.clear();
    writeList.clear();
    invokeList.clear();
    storeList.clear();
    functionList.clear();
    declareList.clear();
    if (trace_call)
        printf("PROCESSING %s\n", func->getName().str().c_str());
if (func->getName() == "zz_ZN7IVector3sayE10FixedPointILi6EES0_ILi4EE") {
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
func->dump();
}
    /* Generate cpp/Verilog for all instructions.  Record function calls for post processing */
    processIFunction = func;
    if (func->hasStructRetAttr())
    if (auto PTy = dyn_cast<PointerType>(func->arg_begin()->getType())) {
        std::string sname = GetValueName(func->arg_begin());
        declareList[sname] = printType(PTy->getElementType(), false,
            sname, "", "", false);
    }
    for (auto BI = func->begin(), BE = func->end(); BI != BE; ++BI) {
        for (auto II = BI->begin(), IE = BI->end(); II != IE;II++) {
            if (!isInlinableInst(*II)) {
                std::string vout = processInstruction(*II);
                if (vout != "") {
                    if (!isDirectAlloca(&*II) && II->use_begin() != II->use_end()
                         && II->getType() != Type::getVoidTy(BI->getContext())) {
                        std::string resname = GetValueName(&*II);
                        declareList[resname] = printType(II->getType(), false, resname, "", "", false);
                        storeList[resname] = ReferenceType{II->getParent(), vout};
                    }
                    else
                        functionList.push_back(ReferenceType{II->getParent(), vout});
                }
            }
        }
    }
    processIFunction = NULL;
}

/*
 * recursively walk a datatype and all subtypes it references, calling
 * a specified callback function to write the type definitions into
 * cpp and verilog output files
 */
static void generateContainedStructs(const Type *Ty, std::string ODir)
{
    if (!Ty)
        return;
    if (const PointerType *PTy = dyn_cast<PointerType>(Ty))
        generateContainedStructs(dyn_cast<StructType>(PTy->getElementType()), ODir);
    else if (!structMap[Ty]) {
        structMap[Ty] = 1;
        if (const StructType *STy = dyn_cast<StructType>(Ty))
        if (STy->hasName() && !inheritsModule(STy, "class.InterfaceClass")
         && !inheritsModule(STy, "class.BitsClass")
         && strncmp(STy->getName().str().c_str(), "class.std::", 11) // don't generate anything for std classes
         && strncmp(STy->getName().str().c_str(), "struct.std::", 12)) {
            ClassMethodTable *table = classCreate[STy];
            int Idx = 0;
            for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
                generateContainedStructs(*I, ODir);
                if (table && table->replaceType[Idx])
                    generateContainedStructs(table->replaceType[Idx], ODir);
            }
            if (STy->getName() != "class.Module") {
                // Only generate verilog for modules derived from Module
                generateRegion = ProcessVerilog;
                if (inheritsModule(STy, "class.Module"))
                    generateModuleDef(STy, ODir);
                // Generate cpp for all modules except class.ModuleExternal
                generateRegion = ProcessCPP;
                if (!inheritsModule(STy, "class.ModuleExternal"))
                    generateClassDef(STy, ODir);
            }
        }
    }
}

/*
 * Top level processing done after all module object files are loaded
 */
bool GenerateRunOnModule(Module *Mod, std::string OutDirectory)
{
    // Before running constructors, clean up and rewrite IR
    preprocessModule(Mod);

    // run Constructors from user program
    EE->runStaticConstructorsDestructors(false);

    // Construct the address -> symbolic name map using actual data allocated/initialized.
    // Pointer datatypes allocated by a class are hoisted and instantiated statically
    // in the generated class.  (in cpp, only pointers can be overridden with derived
    // class instances)
    constructAddressMap(Mod);

    // Walk the list of all classes referenced in the IR image,
    // recursively generating cpp class and verilog module definitions
    for (auto current : classCreate)
        generateContainedStructs(current.first, OutDirectory);
    return false;
}
