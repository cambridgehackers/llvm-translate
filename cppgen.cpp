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
#include <string.h>
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/InstIterator.h"

using namespace llvm;

#include "declarations.h"

/*
 * Output instructions
 */
const char *processCInstruction(Function ***thisp, Instruction &I)
{
    char vout[MAX_CHAR_BUFFER];
    int opcode = I.getOpcode();
    vout[0] = 0;
    switch(opcode) {
    // Terminators
    case Instruction::Ret:
        if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
          strcat(vout, "  return ");
          if (I.getNumOperands())
            strcat(vout, writeOperand(thisp, I.getOperand(0), false));
          strcat(vout, ";\n");
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
    case Instruction::And: case Instruction::Or: case Instruction::Xor: {
        assert(!I.getType()->isPointerTy());
        bool needsCast = false;
        if (I.getType() ==  Type::getInt8Ty(I.getContext())
         || I.getType() == Type::getInt16Ty(I.getContext())
         || I.getType() == Type::getFloatTy(I.getContext())) {
          needsCast = true;
          strcat(vout, printType(I.getType(), false, "", "((", ")("));
        }
        if (BinaryOperator::isNeg(&I)) {
          strcat(vout, "-(");
          strcat(vout, writeOperand(thisp, BinaryOperator::getNegArgument(cast<BinaryOperator>(&I)), false));
          strcat(vout, ")");
        } else if (BinaryOperator::isFNeg(&I)) {
          strcat(vout, "-(");
          strcat(vout, writeOperand(thisp, BinaryOperator::getFNegArgument(cast<BinaryOperator>(&I)), false));
          strcat(vout, ")");
        } else if (I.getOpcode() == Instruction::FRem) {
          if (I.getType() == Type::getFloatTy(I.getContext()))
            strcat(vout, "fmodf(");
          else if (I.getType() == Type::getDoubleTy(I.getContext()))
            strcat(vout, "fmod(");
          else  // all 3 flavors of long double
            strcat(vout, "fmodl(");
          strcat(vout, writeOperand(thisp, I.getOperand(0), false));
          strcat(vout, ", ");
          strcat(vout, writeOperand(thisp, I.getOperand(1), false));
          strcat(vout, ")");
        } else {
          strcat(vout, writeOperandWithCast(thisp, I.getOperand(0), I.getOpcode()));
          strcat(vout, " ");
          strcat(vout, intmapLookup(opcodeMap, I.getOpcode()));
          strcat(vout, " ");
          strcat(vout, writeOperandWithCast(thisp, I.getOperand(1), I.getOpcode()));
          strcat(vout, writeInstructionCast(I));
        }
        if (needsCast) {
          strcat(vout, "))");
        }
        }
        break;

    // Memory instructions...
    case Instruction::Store: {
        StoreInst &IS = static_cast<StoreInst&>(I);
        ERRORIF (IS.isVolatile());
        const char *pdest = writeOperand(thisp, IS.getPointerOperand(), true);
        Value *Operand = I.getOperand(0);
        Constant *BitMask = 0;
        IntegerType* ITy = dyn_cast<IntegerType>(Operand->getType());
        if (ITy && !ITy->isPowerOf2ByteWidth())
            BitMask = ConstantInt::get(ITy, ITy->getBitMask());
        const char *sval = writeOperand(thisp, Operand, false);
        if (!strncmp(pdest, "*((0x", 5)) {
            char *endptr = NULL;
            void *pint = (void *)strtol(pdest+5, &endptr, 16);
            const char *pname = mapAddress(pint, "", NULL);
            if (strncmp(pname, "0x", 2) && !strcmp(endptr, "))"))
                pdest = pname;
        }
        strcat(vout, pdest);
        strcat(vout, " = ");
        if (BitMask)
          strcat(vout, "((");
        std::map<std::string, void *>::iterator NI = nameMap.find(sval);
//printf("[%s:%d] storeval %s found %d\n", __FUNCTION__, __LINE__, sval, (NI != nameMap.end()));
        if (NI != nameMap.end() && NI->second) {
            sval = mapAddress(NI->second, "", NULL);
//printf("[%s:%d] second %p pname %s\n", __FUNCTION__, __LINE__, NI->second, sval);
        }
        strcat(vout, sval);
        if (BitMask) {
          strcat(vout, printConstant(thisp, ") & ", BitMask));
          strcat(vout, ")");
        }
        }
        break;

    // Convert instructions...
    case Instruction::SExt:
    case Instruction::FPTrunc: case Instruction::FPExt:
    case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::UIToFP: case Instruction::SIToFP:
    case Instruction::IntToPtr: case Instruction::PtrToInt:
    case Instruction::AddrSpaceCast:
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::BitCast: {
        Type *DstTy = I.getType();
        Type *SrcTy = I.getOperand(0)->getType();
        const char *p = getOperand(thisp, I.getOperand(0), false);
        char ptemp[1000];
        //printf("[%s:%d] RRRRRR %s %s thisp %p\n", __FUNCTION__, __LINE__, I.getOpcodeName(), p, thisp);
        if (!strcmp(p, "Vthis") && thisp) {
            sprintf(ptemp, "0x%lx", (unsigned long)thisp);
            p = ptemp;
        }
        if (p[0] == '&') {
            void *tval = mapLookup(p+1);
            if (tval) {
                strcat(vout, p);
                break;
            }
        }
        if (!strncmp(p, "0x", 2)) {
            strcat(vout, p);
            break;
        }
        strcat(vout, "(");
        strcat(vout, printCast(opcode, SrcTy, DstTy));
        if (SrcTy == Type::getInt1Ty(I.getContext()) && opcode == Instruction::SExt)
          strcat(vout, "0-");
        strcat(vout, p);
        if (DstTy == Type::getInt1Ty(I.getContext()) &&
            (opcode == Instruction::Trunc || opcode == Instruction::FPToUI ||
             opcode == Instruction::FPToSI || opcode == Instruction::PtrToInt)) {
          strcat(vout, "&1u");
        }
        strcat(vout, ")");
        }
        break;

    // Other instructions...
    case Instruction::ICmp: case Instruction::FCmp: {
        ICmpInst &CI = static_cast<ICmpInst&>(I);
        bool shouldCast = CI.isRelational();
        bool typeIsSigned = CI.isSigned();
        strcat(vout, writeOperandWithCastICmp(thisp, I.getOperand(0), shouldCast, typeIsSigned));
        strcat(vout, " ");
        strcat(vout, intmapLookup(predText, CI.getPredicate()));
        strcat(vout, " ");
        strcat(vout, writeOperandWithCastICmp(thisp, I.getOperand(1), shouldCast, typeIsSigned));
        strcat(vout, writeInstructionCast(I));
        if (shouldCast)
          strcat(vout, "))");
        }
        break;
    case Instruction::Call: {
        CallInst &ICL = static_cast<CallInst&>(I);
        unsigned ArgNo = 0;
        const char *sep = "";
        Function *func = ICL.getCalledFunction();
        ERRORIF(func && (Intrinsic::ID)func->getIntrinsicID());
        ERRORIF (ICL.hasStructRetAttr() || ICL.hasByValArgument() || ICL.isTailCall());
        Value *Callee = ICL.getCalledValue();
        ConstantExpr *CE = dyn_cast<ConstantExpr>(Callee);
        Function *RF = NULL;
        CallSite CS(&I);
        CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
        const char *cthisp = getOperand(thisp, *AI, false);
        Function ***called_thisp = NULL;
        if (!strncmp(cthisp, "0x", 2))
            called_thisp = (Function ***)mapLookup(cthisp);
        if (CE && CE->isCast() && (RF = dyn_cast<Function>(CE->getOperand(0)))) {
            Callee = RF;
            strcat(vout, printType(ICL.getCalledValue()->getType(), false, "", "((", ")(void*)"));
        }
        const char *p = writeOperand(thisp, Callee, false);
printf("[%s:%d] p %s func %p thisp %p called_thisp %p\n", __FUNCTION__, __LINE__, p, func, thisp, called_thisp);
        if (!strncmp(p, "&0x", 3) && !func) {
            void *tval = mapLookup(p+1);
            if (tval) {
                func = static_cast<Function *>(tval);
                if (func)
                    p = func->getName().str().c_str();
                printf("[%s:%d] tval %p pnew %s\n", __FUNCTION__, __LINE__, tval, p);
            }
        }
        pushWork(func, called_thisp, 2);
        int skip = regen_methods;
        std::map<Function *,ClassMethodTable *>::iterator NI = functionIndex.find(func);
        if (NI != functionIndex.end()) {
            p = writeOperand(thisp, *AI, false);
            if (p[0] == '&')
                p++;
            strcat(vout, p);
            strcat(vout, ".");
            strcat(vout, NI->second->method[func].c_str());
            skip = 1;
        }
        else
            strcat(vout, p);
        if (RF)
            strcat(vout, ")");
        PointerType  *PTy = (func) ? cast<PointerType>(func->getType()) : cast<PointerType>(Callee->getType());
        FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
        ERRORIF(FTy->isVarArg() && !FTy->getNumParams());
        unsigned len = FTy->getNumParams();
        strcat(vout, "(");
        for (; AI != AE; ++AI, ++ArgNo) {
            if (!skip) {
            strcat(vout, sep);
            if (ArgNo < len && (*AI)->getType() != FTy->getParamType(ArgNo))
                strcat(vout, printType(FTy->getParamType(ArgNo), /*isSigned=*/false, "", "(", ")"));
            const char *p = writeOperand(thisp, *AI, false);
            strcat(vout, p);
            sep = ", ";
            }
            skip = 0;
        }
        strcat(vout, ")");
        }
        break;
    default:
        printf("Other opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
        break;
    }
    return strdup(vout);
}

void generateClassDef(const StructType *STy, FILE *OStr, int generate)
{
    std::string name = getStructName(STy);
    fprintf(OStr, "class %s {\npublic:\n", name.c_str());
    unsigned Idx = 0;
    for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
        fprintf(OStr, "%s", printType(*I, false, fieldName(STy, Idx++), "  ", ";\n"));
    ClassMethodTable *table = findClass(name);
    if (table)
        for (std::map<Function *, std::string>::iterator FI = table->method.begin(); FI != table->method.end(); FI++) {
            VTABLE_WORK foo(FI->first, NULL);
            regen_methods = 1;
            processFunction(foo, 2, FI->second.c_str(), OStr);
            regen_methods = 0;
        }
    fprintf(OStr, "};\n\n");
}

void generateCppData(FILE *OStr, Module &Mod)
{
    ArrayType *ATy;
    const ConstantExpr *CE;
    std::list<std::string> ruleList;
    NextTypeID = 1;
    fprintf(OStr, "\n\n/* Global Variable Definitions and Initialization */\n");
    for (Module::global_iterator I = Mod.global_begin(), E = Mod.global_end(); I != E; ++I) {
        ERRORIF (I->hasWeakLinkage() || I->hasDLLImportLinkage() || I->hasDLLExportLinkage()
          || I->isThreadLocal() || I->hasHiddenVisibility() || I->hasExternalWeakLinkage());
        if (processVar(I)) {
          Type *Ty = I->getType()->getElementType();
          if (!(Ty->getTypeID() == Type::ArrayTyID && (ATy = cast<ArrayType>(Ty))
              && ATy->getElementType()->getTypeID() == Type::PointerTyID)
           && I->getInitializer()->isNullValue()) {
              if (I->hasLocalLinkage())
                fprintf(OStr, "static ");
              fprintf(OStr, "%s", printType(Ty, false, GetValueName(I), "", ""));
              if (!I->getInitializer()->isNullValue())
                fprintf(OStr, " = %s", writeOperand(NULL, I->getInitializer(), false));
              fprintf(OStr, ";\n");
          }
        }
    }
    fprintf(OStr, "\n\n//******************** vtables for Classes *******************\n");
    for (Module::global_iterator I = Mod.global_begin(), E = Mod.global_end(); I != E; ++I)
        if (processVar(I)) {
          Type *Ty = I->getType()->getElementType();
          if (Ty->getTypeID() == Type::ArrayTyID && (ATy = cast<ArrayType>(Ty))
           && ATy->getElementType()->getTypeID() == Type::PointerTyID) {
              if (I->hasLocalLinkage())
                fprintf(OStr, "static ");
              std::string strName = GetValueName(I);
              fprintf(OStr, "%s", printType(Ty, false, strName, "", ""));
              if (!I->getInitializer()->isNullValue()) {
                fprintf(OStr, " = " );
                Constant* CPV = dyn_cast<Constant>(I->getInitializer());
                if (ConstantArray *CA = dyn_cast<ConstantArray>(CPV)) {
                    Type *ETy = CA->getType()->getElementType();
                    ERRORIF (ETy == Type::getInt8Ty(CA->getContext()) || ETy == Type::getInt8Ty(CA->getContext()));
                    fprintf(OStr, "{");
                    const char *sep = " ";
                    if ((CE = dyn_cast<ConstantExpr>(CA->getOperand(3))) && CE->getOpcode() == Instruction::BitCast
                     && checkIfRule(CE->getOperand(0)->getType())) {
                        ruleList.push_back(strName);
                        for (unsigned i = 2, e = CA->getNumOperands(); i != e; ++i) {
                          Constant* V = dyn_cast<Constant>(CA->getOperand(i));
                          fprintf(OStr, "%s", printConstant(NULL, sep, V));
                          sep = ", ";
                        }
                    }
                    else
                        fprintf(OStr, "0");
                    fprintf(OStr, " }");
                }
              }
              fprintf(OStr, ";\n");
          }
    }
    fprintf(OStr, "typedef unsigned char **RuleVTab;//Rules:\nconst RuleVTab ruleList[] = {\n    ");
    while (ruleList.begin() != ruleList.end()) {
        fprintf(OStr, "%s, ", ruleList.begin()->c_str());
        ruleList.pop_front();
    }
    fprintf(OStr, "NULL};\n");
}
void generateCppHeader(Module &Mod, FILE *OStr)
{
    fprintf(OStr, "\n/* External Global Variable Declarations */\n");
    for (Module::global_iterator I = Mod.global_begin(), E = Mod.global_end(); I != E; ++I)
        if (I->hasExternalLinkage() || I->hasCommonLinkage())
          fprintf(OStr, "%s", printType(I->getType()->getElementType(), false, GetValueName(I), "extern ", ";\n"));
    fprintf(OStr, "\n/* Function Declarations */\n");
    for (Module::iterator I = Mod.begin(), E = Mod.end(); I != E; ++I) {
        ERRORIF(I->hasExternalWeakLinkage() || I->hasHiddenVisibility() || (I->hasName() && I->getName()[0] == 1));
        std::string name = I->getName().str();
        if (!(I->isIntrinsic() || name == "main" || name == "atexit"
         || name == "printf" || name == "__cxa_pure_virtual"
         || name == "setjmp" || name == "longjmp" || name == "_setjmp"))
            fprintf(OStr, "%s", printFunctionSignature(I, NULL, true, ";\n", 0));
    }
    UnnamedStructIDs.clear();
}
