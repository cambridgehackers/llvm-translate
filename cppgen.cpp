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
#include "llvm/IR/Instructions.h"

using namespace llvm;

#include "declarations.h"

/*
 * Output instructions
 */
std::string processCInstruction(Function ***thisp, Instruction &I)
{
    std::string vout;
    int opcode = I.getOpcode();
    switch(opcode) {
    // Terminators
    case Instruction::Ret:
        if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
            vout += "  return ";
            if (I.getNumOperands())
                vout += writeOperand(thisp, I.getOperand(0), false);
            vout += ";\n";
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
            vout += "-(" + writeOperand(thisp, BinaryOperator::getNegArgument(cast<BinaryOperator>(&I)), false) + ")";
        else if (BinaryOperator::isFNeg(&I))
            vout += "-(" + writeOperand(thisp, BinaryOperator::getFNegArgument(cast<BinaryOperator>(&I)), false) + ")";
        else if (I.getOpcode() == Instruction::FRem) {
            if (I.getType() == Type::getFloatTy(I.getContext()))
                vout += "fmodf(";
            else if (I.getType() == Type::getDoubleTy(I.getContext()))
                vout += "fmod(";
            else  // all 3 flavors of long double
                vout += "fmodl(";
            vout += writeOperand(thisp, I.getOperand(0), false) + ", "
                 + writeOperand(thisp, I.getOperand(1), false) + ")";
        } else
            vout += writeOperand(thisp, I.getOperand(0), false)
                 + " " + intmapLookup(opcodeMap, I.getOpcode()) + " "
                 + writeOperand(thisp, I.getOperand(1), false);
        break;

    // Memory instructions...
    case Instruction::Store: {
        StoreInst &IS = static_cast<StoreInst&>(I);
        ERRORIF (IS.isVolatile());
        std::string pdest = writeOperand(thisp, IS.getPointerOperand(), true);
        Value *Operand = I.getOperand(0);
        Constant *BitMask = 0;
        IntegerType* ITy = dyn_cast<IntegerType>(Operand->getType());
        if (ITy && !ITy->isPowerOf2ByteWidth())
            BitMask = ConstantInt::get(ITy, ITy->getBitMask());
        std::string sval = writeOperand(thisp, Operand, false);
        if (!strncmp(pdest.c_str(), "*((0x", 5)) {
            char *endptr = NULL;
            void *pint = (void *)strtol(pdest.c_str()+5, &endptr, 16);
            std::string pname = mapAddress(pint, "", NULL);
            if (strncmp(pname.c_str(), "0x", 2) && !strcmp(endptr, "))"))
                pdest = pname;
        }
        vout += pdest + " = ";
        if (BitMask)
          vout += "((";
        std::map<std::string, void *>::iterator NI = nameMap.find(sval.c_str());
//printf("[%s:%d] storeval %s found %d\n", __FUNCTION__, __LINE__, sval.c_str(), (NI != nameMap.end()));
        if (NI != nameMap.end() && NI->second)
            sval = mapAddress(NI->second, "", NULL);
        vout += sval;
        if (BitMask) {
          vout += ") & " + writeOperand(thisp, BitMask, false) + ")";
        }
        break;
        }

    // Convert instructions...
    case Instruction::SExt:
    case Instruction::FPTrunc: case Instruction::FPExt:
    case Instruction::FPToUI: case Instruction::FPToSI:
    case Instruction::UIToFP: case Instruction::SIToFP:
    case Instruction::IntToPtr: case Instruction::PtrToInt:
    case Instruction::AddrSpaceCast:
    case Instruction::Trunc: case Instruction::ZExt: case Instruction::BitCast: {
        std::string p = getOperand(thisp, I.getOperand(0), false);
        if (p == "Vthis" && thisp) {
            char ptemp[1000];
            sprintf(ptemp, "0x%lx", (unsigned long)thisp);
            p = ptemp;
        }
        vout += p;
        break;
        }

    // Other instructions...
    case Instruction::ICmp: case Instruction::FCmp: {
        ICmpInst &CI = static_cast<ICmpInst&>(I);
        vout += writeOperand(thisp, I.getOperand(0), false)
             + " " + intmapLookup(predText, CI.getPredicate()) + " "
             + writeOperand(thisp, I.getOperand(1), false);
        break;
        }
    case Instruction::Call: {
        CallInst &ICL = static_cast<CallInst&>(I);
        unsigned ArgNo = 0;
        const char *sep = "";
        Function *func = ICL.getCalledFunction();
        ERRORIF(func && (Intrinsic::ID)func->getIntrinsicID());
        ERRORIF (ICL.hasStructRetAttr() || ICL.hasByValArgument() || ICL.isTailCall());
        Value *Callee = ICL.getCalledValue();
        CallSite CS(&I);
        CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
        std::string cthisp = getOperand(thisp, *AI, false);
        Function ***called_thisp = NULL;
        if (!strncmp(cthisp.c_str(), "0x", 2))
            called_thisp = (Function ***)mapLookup(cthisp.c_str());
        std::string p = writeOperand(thisp, Callee, false);
printf("[%s:%d] p %s func %p thisp %p called_thisp %p\n", __FUNCTION__, __LINE__, p.c_str(), func, thisp, called_thisp);
        if (!strncmp(p.c_str(), "&0x", 3) && !func) {
            void *tval = mapLookup(p.c_str()+1);
            if (tval) {
                func = static_cast<Function *>(tval);
                if (func)
                    p = func->getName();
                printf("[%s:%d] tval %p pnew %s\n", __FUNCTION__, __LINE__, tval, p.c_str());
            }
        }
        pushWork(func, called_thisp);
        int skip = regen_methods;
        std::map<Function *,ClassMethodTable *>::iterator NI = functionIndex.find(func);
        if (NI != functionIndex.end()) {
            p = writeOperand(thisp, *AI, false);
            if (p[0] == '&')
                p = p.substr(1);
            vout += p + ("." + NI->second->method[func]);
            skip = 1;
        }
        else
            vout += p;
        PointerType  *PTy = (func) ? cast<PointerType>(func->getType()) : cast<PointerType>(Callee->getType());
        FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
        ERRORIF(FTy->isVarArg() && !FTy->getNumParams());
        unsigned len = FTy->getNumParams();
        vout += "(";
        for (; AI != AE; ++AI, ++ArgNo) {
            if (!skip) {
                ERRORIF (ArgNo < len && (*AI)->getType() != FTy->getParamType(ArgNo));
                vout += sep + writeOperand(thisp, *AI, false);
                sep = ", ";
            }
            skip = 0;
        }
        vout += ")";
        break;
        }
    default:
        printf("Other opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
        break;
    }
    return vout;
}

void generateClassDef(const StructType *STy, FILE *OStr)
{
    std::string name = getStructName(STy);
    fprintf(OStr, "class %s {\npublic:\n", name.c_str());
    unsigned Idx = 0;
    for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
        fprintf(OStr, "%s", printType(*I, false, fieldName(STy, Idx++), "  ", ";\n").c_str());
    ClassMethodTable *table = findClass(name);
    if (table)
        for (std::map<Function *, std::string>::iterator FI = table->method.begin(); FI != table->method.end(); FI++) {
            VTABLE_WORK foo(FI->first, NULL);
            regen_methods = 2;
            processFunction(foo, OStr);
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
              fprintf(OStr, "%s", printType(Ty, false, GetValueName(I), "", "").c_str());
              if (!I->getInitializer()->isNullValue())
                fprintf(OStr, " = %s", writeOperand(NULL, I->getInitializer(), false).c_str());
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
              fprintf(OStr, "%s", printType(Ty, false, strName, "", "").c_str());
              if (!I->getInitializer()->isNullValue()) {
                fprintf(OStr, " = " );
                Constant* CPV = dyn_cast<Constant>(I->getInitializer());
                if (ConstantArray *CA = dyn_cast<ConstantArray>(CPV)) {
                    Type *ETy = CA->getType()->getElementType();
                    ERRORIF (ETy == Type::getInt8Ty(CA->getContext()) || ETy == Type::getInt8Ty(CA->getContext()));
                    fprintf(OStr, "{");
                    const char *sep = "";
                    if ((CE = dyn_cast<ConstantExpr>(CA->getOperand(3))) && CE->getOpcode() == Instruction::BitCast
                     && checkIfRule(CE->getOperand(0)->getType())) {
                        ruleList.push_back(strName);
                        for (unsigned i = 2, e = CA->getNumOperands(); i != e; ++i) {
                            Constant* V = dyn_cast<Constant>(CA->getOperand(i));
                            //char *p = writeOperand(NULL, V, false);
                            assert (dyn_cast<PointerType>(V->getType()));
                            std::string p = writeOperand(NULL, V->getOperand(0), false);
                            fprintf(OStr, "%s (unsigned char *)%s", sep, p.c_str());
                            sep = ",";
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
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    fprintf(OStr, "\n/* External Global Variable Declarations */\n");
    for (Module::global_iterator I = Mod.global_begin(), E = Mod.global_end(); I != E; ++I)
        if (I->hasExternalLinkage() || I->hasCommonLinkage())
          fprintf(OStr, "%s", printType(I->getType()->getElementType(), false, GetValueName(I), "extern ", ";\n").c_str());
    fprintf(OStr, "\n/* Function Declarations */\n");
    for (Module::iterator I = Mod.begin(), E = Mod.end(); I != E; ++I) {
        ERRORIF(I->hasExternalWeakLinkage() || I->hasHiddenVisibility() || (I->hasName() && I->getName()[0] == 1));
        std::string name = I->getName().str();
        if (!(I->isIntrinsic() || name == "main" || name == "atexit"
         || name == "printf" || name == "__cxa_pure_virtual"
         || name == "setjmp" || name == "longjmp" || name == "_setjmp"))
            fprintf(OStr, "%s", printFunctionSignature(I, "", true, ";\n", 0).c_str());
    }
    UnnamedStructIDs.clear();
}
