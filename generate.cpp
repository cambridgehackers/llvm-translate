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
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

#include "declarations.h"

static int trace_call;//=1;
int trace_translate ;//= 1;
static int trace_gep;//= 1;
static int trace_hoist;// = 1;
std::string globalName;
std::map<Function *, Function *> ruleRDYFunction;
std::map<const StructType *,ClassMethodTable *> classCreate;
static unsigned NextTypeID;
int generateRegion = ProcessNone;
Function *currentFunction;

static std::list<Function *> vtableWork;
static std::map<const Type *, int> structMap;
static DenseMap<const Value*, unsigned> AnonValueNumbers;
static unsigned NextAnonValueNumber;
static DenseMap<const StructType*, unsigned> UnnamedStructIDs;
static std::string processInstruction(Instruction &I);

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

int inheritsModule(const StructType *STy)
{
    if (STy) {
        std::string sname = STy->getName();
        if (sname == "class.Module")
            return 1;
        for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
            if (inheritsModule(dyn_cast<StructType>(*I)))
                return 1;
    }
    return 0;
}

// Preprocess the body rules, creating shadow variables and moving items to RDY() and ENA()
// Walk list of work items, cleaning up function references and adding to vtableWork
static void processHoist(Function *currentFunction)
{
    for (auto BI = currentFunction->begin(), BE = currentFunction->end(); BI != BE; ++BI) {
        for (auto II = BI->begin(), IE = BI->end(); II != IE;) {
            auto INEXT = std::next(BasicBlock::iterator(II));
            if (II->getOpcode() == Instruction::Call) {
                Instruction &I = *II;
                std::string vout, methodString, fname, methodName, prefix, rmethodString;
                Function *parentRDYName = ruleRDYFunction[currentFunction];
                CallInst &ICL = static_cast<CallInst&>(I);
                int RDYName = -1;
            
                std::string pcalledFunction = printOperand(ICL.getCalledValue(), false);
                if (pcalledFunction[0] == '(' && pcalledFunction[pcalledFunction.length()-1] == ')')
                    pcalledFunction = pcalledFunction.substr(1, pcalledFunction.length()-2);
                Function *func = ICL.getCalledFunction();
                ERRORIF(func && (Intrinsic::ID)func->getIntrinsicID());
                ERRORIF (ICL.hasStructRetAttr() || ICL.hasByValArgument() || ICL.isTailCall());
                if (!func)
                    func = EE->FindFunctionNamed(pcalledFunction.c_str());
                pushWork(func);
                if (trace_call)
                    printf("CALL: CALLER %d %s pRDY %p func %p pcalledFunction '%s'\n", generateRegion, globalName.c_str(), parentRDYName, func, pcalledFunction.c_str());
                if (!func) {
                    printf("%s: not an instantiable call!!!! %s\n", __FUNCTION__, pcalledFunction.c_str());
                    exit(-1);
                }
                PointerType  *PTy = cast<PointerType>(func->getType());
                FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
                unsigned len = FTy->getNumParams();
                ERRORIF(FTy->isVarArg() && !len);
                Instruction *oldOp = dyn_cast<Instruction>(I.getOperand(I.getNumOperands()-1));
                const StructType *STy = findThisArgumentType(func->getType());
                //printf("[%s:%d] %s -> %s %p oldOp %p\n", __FUNCTION__, __LINE__, globalName.c_str(), pcalledFunction.c_str(), func, oldOp);
                for (auto info : classCreate) {
                    if (const StructType *iSTy = info.first)
                    if (ClassMethodTable *table = info.second)
                    if (oldOp && derivedStruct(iSTy, STy)) {
                        if (oldOp->getOpcode() == Instruction::Load)
                        if (Instruction *gep = dyn_cast<Instruction>(oldOp->getOperand(0)))
                        if (gep->getNumOperands() >= 2)
                        if (const ConstantInt *CI = cast<ConstantInt>(gep->getOperand(1)))
                            pushWork(EE->FindFunctionNamed(
                                lookupMethodName(table, CI->getZExtValue()).c_str()));
                    }
                }
                if (oldOp) {
                    I.setOperand(I.getNumOperands()-1, func);
                    recursiveDelete(oldOp);
                }
                if (ClassMethodTable *table = classCreate[STy])
                    if ((rmethodString = getMethodName(func->getName())) != "")
                        RDYName = vtableFind(table, rmethodString + "__RDY");
                fname = func->getName();
                if (trace_hoist)
                    printf("HOIST:    CALL %p typeid %d fname %s\n", func, I.getType()->getTypeID(), fname.c_str());
                if (func->isDeclaration() && !strncmp(fname.c_str(), "_Z14PIPELINEMARKER", 18)) {
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
                    return;
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
            }
            II = INEXT;
        }
    }
}
static void call2runOnFunction(Function *currentFunction, Function &F)
{
    bool changed = false;
    Module *Mod = F.getParent();
    std::string fname = F.getName();
//printf("CallProcessPass2: %s\n", fname.c_str());
    for (auto BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
        for (auto I = BB->getFirstInsertionPt(), E = BB->end(); I != E;) {
            auto PI = std::next(BasicBlock::iterator(I));
            int opcode = I->getOpcode();
            switch (opcode) {
            case Instruction::Alloca: {
                Value *retv = (Value *)I;
                std::string name = I->getName();
                int ind = name.find("block");
//printf("       ALLOCA %s;", name.c_str());
                if (I->hasName() && ind == -1 && endswith(name, ".addr")) {
                    Value *newt = NULL;
                    auto PN = PI;
                    while (PN != E) {
                        auto PNN = std::next(BasicBlock::iterator(PN));
                        if (PN->getOpcode() == Instruction::Store && retv == PN->getOperand(1)) {
                            newt = PN->getOperand(0); // Remember value we were storing in temp
                            if (PI == PN)
                                PI = PNN;
                            PN->eraseFromParent(); // delete Store instruction
                        }
                        else if (PN->getOpcode() == Instruction::Load && retv == PN->getOperand(0)) {
                            PN->replaceAllUsesWith(newt); // replace with stored value
                            if (PI == PN)
                                PI = PNN;
                            PN->eraseFromParent(); // delete Load instruction
                        }
                        PN = PNN;
                    }
//printf("del1");
                    I->eraseFromParent(); // delete Alloca instruction
                    changed = true;
                }
//printf("\n");
                break;
                }
            };
            I = PI;
        }
    }
    for (auto BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
        for (auto II = BB->begin(), IE = BB->end(); II != IE; ) {
            BasicBlock::iterator PI = std::next(BasicBlock::iterator(II));
            if (CallInst *CI = dyn_cast<CallInst>(II)) {
                std::string pcalledFunction = printOperand(CI->getCalledValue(), false);
                if (pcalledFunction[0] == '(' && pcalledFunction[pcalledFunction.length()-1] == ')')
                    pcalledFunction = pcalledFunction.substr(1, pcalledFunction.length() - 2);
                Function *func = dyn_cast_or_null<Function>(Mod->getNamedValue(pcalledFunction));
                std::string cthisp = printOperand(II->getOperand(0), false);
                //printf("%s: %s CALLS %s func %p thisp %s\n", __FUNCTION__, fname.c_str(), pcalledFunction.c_str(), func, cthisp.c_str());
                if (func && cthisp == "this") {
                    fprintf(stdout,"callProcess: pcalledFunction %s single!!!!\n", pcalledFunction.c_str());
                    call2runOnFunction(currentFunction, *func);
                    II->setOperand(II->getNumOperands()-1, func);
                    InlineFunctionInfo IFI;
                    InlineFunction(CI, IFI, false);
                    changed = true;
                }
            }
            II = PI;
        }
    }
}

void pushWork(Function *func)
{
    if (!func || generateRegion != ProcessNone)
        return;
    if (ClassMethodTable *table = classCreate[findThisArgumentType(func->getType())])
        table->method[getMethodName(func->getName())] = func;
    vtableWork.push_back(func);
    // inline intra-class method call bodies
    call2runOnFunction(func, *func);
    processHoist(func);
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
    else {
        if (!UnnamedStructIDs[STy])
            UnnamedStructIDs[STy] = NextTypeID++;
        return "l_unnamed_" + utostr(UnnamedStructIDs[STy]);
    }
}

static std::string GetValueName(const Value *Operand)
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
    VarName.reserve(Name.capacity());
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
    if (generateRegion == ProcessVerilog && VarName != "this")
        return globalName + "_" + VarName;
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
                cbuffer += " reg";
        }
        else if (NumBits <= 16)
            cbuffer += sp + " VERILOG_short";
        else if (NumBits <= 32)
            //cbuffer += sp + " VERILOG_int";
            cbuffer += " reg" + verilogArrRange(Ty);
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

std::string printFunctionSignature(const Function *F, std::string altname, bool Prototype, std::string postfix, int skip)
{
    std::string sep = "", statstr = "", tstr;
    FunctionType *FT = cast<FunctionType>(F->getFunctionType());
    ERRORIF (F->hasDLLImportStorageClass() || F->hasDLLExportStorageClass() || F->hasStructRetAttr() || FT->isVarArg());
    if (F->hasLocalLinkage()) statstr = "static ";
    if (altname != "")
        tstr += altname;
    else
        tstr += F->getName();
    tstr += '(';
    if (F->isDeclaration()) {
        for (auto I = FT->param_begin(), E = FT->param_end(); I != E; ++I) {
            if (!skip) {
                tstr += printType(*I, /*isSigned=*/false, "", sep, "", false);
                sep = ", ";
            }
            skip = 0;
        }
    } else if (!F->arg_empty()) {
        for (auto I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
            if (!skip) {
                std::string ArgName = (I->hasName() || !Prototype) ? GetValueName(I) : "";
                tstr += printType(I->getType(), /*isSigned=*/false, ArgName, sep, "", false);
                sep = ", ";
            }
            skip = 0;
        }
    }
    if (sep == "")
        tstr += "void"; // ret() -> ret(void) in C.
    return printType(F->getReturnType(), /*isSigned=*/false, tstr + ')', statstr, postfix, false);
}

/*
 * GEP and Load instructions interpreter functions
 * (just execute using the memory areas allocated by the constructors)
 */
static std::string printGEPExpression(Value *Ptr, gep_type_iterator I, gep_type_iterator E)
{
    std::string cbuffer = "(", sep = " ", amper = "&";
    PointerType *PTy;
    ConstantDataArray *CPA;
    void *tval = NULL;
    uint64_t Total = 0;
    VectorType *LastIndexIsVector = 0;
    const DataLayout *TD = EE->getDataLayout();
    Constant *FirstOp = dyn_cast<Constant>(I.getOperand());
    bool expose = isAddressExposed(Ptr);
    std::string referstr = printOperand(Ptr, false);
    if (referstr[0] == '(' && referstr[referstr.length()-1] == ')')
       referstr = referstr.substr(1, referstr.length() - 2).c_str();

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
        std::string lname = lookupMethodName(table, Total/sizeof(void *));
        std::string name = getMethodName(lname);
        if (trace_gep)
            printf("%s: Method invocation referstr %s name %s lname %s tval %p\n", __FUNCTION__,
                referstr.c_str(), name.c_str(), lname.c_str(), tval);
        referstr = generateRegion == ProcessNone ? lname : name;
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
            if (expose)
                cbuffer += referstr + ".";
            else {
                referstr += "->";
                if (referstr == "this->")
                    referstr = "";
                cbuffer += referstr;
            }
            cbuffer += fieldName(STy, cast<ConstantInt>(I.getOperand())->getZExtValue());
        }
        else if ((*I)->isArrayTy() || !(*I)->isVectorTy()) {
            cbuffer += referstr;
            cbuffer += "[" + printOperand(I.getOperand(), false) + "]";
        }
        else {
            cbuffer += referstr;
            if (!isa<Constant>(I.getOperand()) || !cast<Constant>(I.getOperand())->isNullValue())
                cbuffer += ")+(" + printOperand(I.getOperand(), false);
            cbuffer += "))";
        }
        referstr = "";
    }
    cbuffer += referstr + ")";
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
        prefix = "(&";  // Global variables are referenced as their addresses by llvm
    if (I && isInlinableInst(*I)) {
        std::string p = processInstruction(*I);
        if (p[0] == '(' && p[p.length()-1] == ')')
            p = p.substr(1,p.length()-2);
        if (prefix == "*" && p[0] == '&') {
            prefix = "";
            p = p.substr(1);
        }
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
    if (isAddressImplicit)
        cbuffer += ")";
    return cbuffer;
}

static std::string printCall(Instruction &I)
{
    std::string vout, methodString, fname, methodName, prefix, rmethodString;
    Function *parentRDYName = ruleRDYFunction[currentFunction];
    CallInst &ICL = static_cast<CallInst&>(I);
    unsigned ArgNo = 0;
    const char *sep = "";
    CallSite CS(&I);
    CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
    int skip = generateRegion != ProcessNone;

    std::string pcalledFunction = printOperand(ICL.getCalledValue(), false);
    if (pcalledFunction[0] == '(' && pcalledFunction[pcalledFunction.length()-1] == ')')
        pcalledFunction = pcalledFunction.substr(1, pcalledFunction.length()-2);
    Function *func = ICL.getCalledFunction();
    ERRORIF(func && (Intrinsic::ID)func->getIntrinsicID());
    ERRORIF (ICL.hasStructRetAttr() || ICL.hasByValArgument() || ICL.isTailCall());
    if (!func)
        func = EE->FindFunctionNamed(pcalledFunction.c_str());
    pushWork(func);
    if (trace_call)
        printf("CALL: CALLER %d %s pRDY %p func %p pcalledFunction '%s'\n", generateRegion, globalName.c_str(), parentRDYName, func, pcalledFunction.c_str());
    if (!func) {
        printf("%s: not an instantiable call!!!! %s\n", __FUNCTION__, pcalledFunction.c_str());
        exit(-1);
    }
    PointerType  *PTy = cast<PointerType>(func->getType());
    FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
    unsigned len = FTy->getNumParams();
    ERRORIF(FTy->isVarArg() && !len);
    ClassMethodTable *CMT = classCreate[findThisArgumentType(func->getType())];
    if (CMT)
        pcalledFunction = printOperand(*AI, false);
    else {
        printf("[%s:%d]\n", __FUNCTION__, __LINE__);
        exit(-1);
    }
    if (generateRegion == ProcessVerilog) {
        if (pcalledFunction.substr(0,1) == "(" && pcalledFunction[pcalledFunction.length()-1] == ')')
            pcalledFunction = pcalledFunction.substr(1,pcalledFunction.length()-2);
        if (pcalledFunction[0] == '&')
            pcalledFunction = pcalledFunction.substr(1);
        prefix = pcalledFunction + "_" + getMethodName(func->getName());
        vout += prefix;
        skip = 1;
        if (func->getReturnType() == Type::getVoidTy(func->getContext()))
            vout += "__ENA = 1";
        if (prefix == "")
            vout += "(";
        Function::const_arg_iterator FAI = func->arg_begin();
        for (; AI != AE; ++AI, ++ArgNo, FAI++) {
            if (!skip) {
                vout += sep;
                std::string p = printOperand(*AI, false);
                if (prefix != "")
                    vout += (";\n            " + prefix + "_" + FAI->getName().str() + " = ");
                else
                    sep = ", ";
                vout += p;
            }
            skip = 0;
        }
    }
    else {
        std::string poststr;
        if (!inheritsModule(findThisArgumentType(currentFunction->getType()))) {
            //printf("[%s:%d] %s -> %s %p skip %d\n", __FUNCTION__, __LINE__, globalName.c_str(), pcalledFunction.c_str(), func, skip);
            skip = 0;
        }
        skip = 1;
        poststr = "->";
        if (pcalledFunction.substr(0,2) == "(&" && pcalledFunction[pcalledFunction.length()-1]) {
            pcalledFunction = pcalledFunction.substr(2,pcalledFunction.length()-3);
            poststr = ".";
        }
        poststr += getMethodName(func->getName());
        vout += pcalledFunction + poststr + "(";
        if (len && FTy->getParamType(0)->getTypeID() != Type::PointerTyID) {
            printf("[%s:%d] clear skip\n", __FUNCTION__, __LINE__);
            skip = 0;
        }
        for (; AI != AE; ++AI, ++ArgNo) {
            if (!skip) {
                vout += sep + printOperand(*AI, false);
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
static std::string processInstruction(Instruction &I)
{
    std::string vout;
    int opcode = I.getOpcode();
//printf("[%s:%d] op %s\n", __FUNCTION__, __LINE__, I.getOpcodeName());
    switch(opcode) {
    case Instruction::GetElementPtr: {
        GetElementPtrInst &IG = static_cast<GetElementPtrInst&>(I);
        return printGEPExpression(IG.getPointerOperand(), gep_type_begin(IG), gep_type_end(IG));
        }
    case Instruction::Load: {
        LoadInst &IL = static_cast<LoadInst&>(I);
        ERRORIF (IL.isVolatile());
        return printOperand(I.getOperand(0), true);
        }
    // Terminators
    case Instruction::Ret:
        if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
            if (generateRegion == ProcessVerilog)
                vout += "    " + globalName + " = ";
            else
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
            vout += printOperand(I.getOperand(0), false)
                 + " " + intmapLookup(opcodeMap, I.getOpcode()) + " "
                 + printOperand(I.getOperand(1), false);
        break;

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
        if (pdest.length() > 2 && pdest[0] == '(' && pdest[pdest.length()-1] == ')')
            pdest = pdest.substr(1, pdest.length() -2);
        vout += pdest + ((generateRegion == ProcessVerilog) ? " <= " : " = ");
        if (BitMask)
            vout += "((";
        vout += sval;
        if (BitMask)
            vout += ") & " + printOperand(BitMask, false) + ")";
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
        vout += printOperand(I.getOperand(0), false);
        break;

    // Other instructions...
    case Instruction::ICmp: case Instruction::FCmp: {
        ICmpInst &CI = static_cast<ICmpInst&>(I);
        vout += printOperand(I.getOperand(0), false)
             + " " + intmapLookup(predText, CI.getPredicate()) + " "
             + printOperand(I.getOperand(1), false);
        break;
        }
    case Instruction::Call:
        vout += printCall(I);
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
void processFunction(Function *func, FILE *OStr)
{
    std::string fname = func->getName();
    globalName = getMethodName(fname);
    if (globalName == "")
        globalName = fname;
    currentFunction = func;

    NextAnonValueNumber = 0;
    if (trace_call)
        printf("PROCESSING %s\n", globalName.c_str());
#if 0
    /* connect up argument formal param names with actual values */
    for (auto AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
        int slotindex = getLocalSlot(AI);
        ERRORIF (AI->hasByValAttr());
        (AI->getName().str().c_str());
    }
#endif
    /* Generate Verilog for all instructions.  Record function calls for post processing */
    for (auto BI = func->begin(), BE = func->end(); BI != BE; ++BI) {
        if (trace_translate && BI->hasName())         // Print out the label if it exists...
            printf("LLLLL: %s\n", BI->getName().str().c_str());
        for (auto II = BI->begin(), IE = BI->end(); II != IE;) {
            auto INEXT = std::next(BasicBlock::iterator(II));
            if (INEXT == IE || !isInlinableInst(*II)) {
                if (trace_translate && generateRegion == ProcessCPP)
                    printf("/*before %p opcode %d.=%s*/\n", &*II, II->getOpcode(), II->getOpcodeName());
                std::string vout = processInstruction(*II);
                bool save_val = (!isDirectAlloca(&*II) && II->use_begin() != II->use_end()
                            && II->getType() != Type::getVoidTy(BI->getContext()));
                if (vout != "" && OStr) {
                    fprintf(OStr, "        ");
                    if (save_val) {
                        std::string resname = GetValueName(&*II);
                        if (generateRegion == ProcessCPP)
                            resname = printType(II->getType(), false, resname, "", "", false);
                        fprintf(OStr, "%s = ", resname.c_str());
                    }
                    fprintf(OStr, "%s;\n", vout.c_str());
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
        if (const StructType *STy = dyn_cast<StructType>(Ty)) {
            ClassMethodTable *table = classCreate[STy];
            int Idx = 0;
            for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
                const Type *element = *I;
                if (table && table->replaceType[Idx])
                    element = table->replaceType[Idx];
                printContainedStructs(element, OStr, ODir, cb);
            }
            if (classCreate[STy] && inheritsModule(STy))
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
    // remove dwarf info, if it was compiled in
    const char *delete_names[] = { "llvm.dbg.declare", "llvm.dbg.value", "atexit", NULL};
    const char **p = delete_names;
    while(*p)
        if (Function *Declare = Mod->getFunction(*p++)) {
            while (!Declare->use_empty()) {
                CallInst *CI = cast<CallInst>(Declare->user_back());
                CI->eraseFromParent();
            }
            Declare->eraseFromParent();
        }

    // Construct the vtable map for classes
    constructVtableMap(Mod);

    // before running constructors, remap all calls to 'malloc' and 'new' to our runtime.
    const char *malloc_names[] = { "_Znwm", "malloc", NULL};
    p = malloc_names;
    while(*p)
        if (Function *Declare = Mod->getFunction(*p++))
            for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; I++)
                callMemrunOnFunction(cast<CallInst>(*I));

    // Add StructType parameter to exportSymbol calls
    if (Function *Declare = Mod->getFunction("exportSymbol"))
        for(auto I = Declare->user_begin(), E = Declare->user_end(); I != E; I++) {
            CallInst *II = cast<CallInst>(*I);
            II->setOperand(2, ConstantInt::get(Type::getInt64Ty(II->getContext()),
                (unsigned long)findThisArgumentType(II->getParent()->getParent()->getType())));
        }

    // run Constructors
    EE->runStaticConstructorsDestructors(false);

    // Construct the address -> symbolic name map using actual data allocated/initialized
    constructAddressMap(Mod);

    // Generate verilog for all rules
    generateRegion = ProcessVerilog;
    generateStructs(NULL, OutDirectory, generateModuleDef);

    // Generate cpp code for all rules
    generateRegion = ProcessCPP;
    FILE *Out = fopen((OutDirectory + "/output.cpp").c_str(), "w");
    generateStructs(Out, "", generateClassBody); // generate class method bodies
    FILE *OutHeader = fopen((OutDirectory + "/output.h").c_str(), "w");
    generateStructs(OutHeader, "", generateClassDef); // generate class definitions
    return false;
}
