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
 * Generate Verilog output for Store and Call instructions
 */
const char *generateVerilog(Function ***thisp, Instruction &I)
{
    static char vout[MAX_CHAR_BUFFER];
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
#if 0
    case Instruction::Br:
        {
        if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
          char temp[MAX_CHAR_BUFFER];
          const BranchInst &BI(cast<BranchInst>(I));
          prepareOperand(BI.getCondition());
          int cond_item = getLocalSlot(BI.getCondition());
          sprintf(temp, "%s" SEPARATOR "%s_cond", globalName, I.getParent()->getName().str().c_str());
          if (slotarray[cond_item].name) {
              sprintf(vout, "%s = %s\n", temp, slotarray[cond_item].name);
              slotarray[cond_item].name = strdup(temp);
          }
          prepareOperand(BI.getSuccessor(0));
          prepareOperand(BI.getSuccessor(1));
        } else if (isa<IndirectBrInst>(I)) {
          for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i) {
            prepareOperand(I.getOperand(i));
          }
        }
        dump_operands = 1;
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
#endif

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

    case Instruction::Call: {
        CallInst &ICL = static_cast<CallInst&>(I);
        unsigned ArgNo = 0;
        const char *sep = "";
        Function *func = ICL.getCalledFunction();
        ERRORIF(func && (Intrinsic::ID)func->getIntrinsicID());
        ERRORIF (ICL.hasStructRetAttr() || ICL.hasByValArgument() || ICL.isTailCall());
        Value *Callee = ICL.getCalledValue();
        PointerType  *PTy = cast<PointerType>(Callee->getType());
        FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
        ConstantExpr *CE = dyn_cast<ConstantExpr>(Callee);
        Function *RF = NULL;
        ERRORIF(FTy->isVarArg() && !FTy->getNumParams());
        unsigned len = FTy->getNumParams();
        CallSite CS(&I);
        CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
        if (CE && CE->isCast() && (RF = dyn_cast<Function>(CE->getOperand(0)))) {
            Callee = RF;
            strcat(vout, printType(ICL.getCalledValue()->getType(), false, "", "((", ")(void*)"));
        }
        strcat(vout, writeOperand(thisp, Callee, false));
        if (RF)
            strcat(vout, ")");
        strcat(vout, "(");
        for (; AI != AE; ++AI, ++ArgNo) {
            strcat(vout, sep);
            if (ArgNo < len && (*AI)->getType() != FTy->getParamType(ArgNo))
                strcat(vout, printType(FTy->getParamType(ArgNo), /*isSigned=*/false, "", "(", ")"));
            const char *p = writeOperand(thisp, *AI, false);
            strcat(vout, p);
            sep = ", ";
        }
        strcat(vout, ")");
        pushWork(func, NULL, 1);
        }
        break;

#if 0
    case Instruction::PHI:
        {
        char temp[MAX_CHAR_BUFFER];
        const PHINode *PN = dyn_cast<PHINode>(&I);
        if (!PN) {
            printf("[%s:%d]\n", __FUNCTION__, __LINE__);
            exit(1);
        }
        I.getType()->dump();
        sprintf(temp, "%s" SEPARATOR "%s_phival", globalName, I.getParent()->getName().str().c_str());
        sprintf(vout, "%s = ", temp);
        slotarray[operand_list[0].value].name = strdup(temp);
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
              strcat(vout, temp);
            }
            if (slotarray[valuein].name)
                sprintf(temp, "%s %s", slotarray[valuein].name, trailch);
            else
                sprintf(temp, "%lld %s", (long long)slotarray[valuein].offset, trailch);
            strcat(vout, temp);
        }
        dump_operands = 1;
        }
        break;
#endif
    default:
        printf("Other opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
        break;
    }
    if (vout[0])
        return vout;
    return NULL;
}

static std::map<const Type *, int> structMap;
static void printContainedStructs(const Type *Ty, FILE *OStr)
{
    std::map<const Type *, int>::iterator FI = structMap.find(Ty);
    const PointerType *PTy = dyn_cast<PointerType>(Ty);
    if (PTy) {
        const StructType *subSTy = dyn_cast<StructType>(PTy->getElementType());
        if (subSTy) { /* Not recursion!  These are generated afterword, if we didn't generate before */
            std::map<const Type *, int>::iterator FI = structMap.find(subSTy);
            if (FI != structMap.end())
                structWork.push_back(subSTy);
        }
    }
    else if (FI == structMap.end() && !Ty->isPrimitiveType() && !Ty->isIntegerTy()) {
        generateModuleDef(Ty, OStr);
    }
}

void generateModuleDef(const Type *Ty, FILE *OStr)
{
    const StructType *STy = dyn_cast<StructType>(Ty);
        std::string name;
        structMap[Ty] = 1;
        if (STy) {
            name = getStructName(STy);
            //fprintf(OStr, "class %s;\n", name.c_str());
        }
        for (Type::subtype_iterator I = Ty->subtype_begin(), E = Ty->subtype_end(); I != E; ++I)
            printContainedStructs(*I, OStr);
        if (STy) {
            for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
                printContainedStructs(*I, OStr);
            ClassMethodTable *table = findClass(name);
            if (table) {
            fprintf(OStr, "module %s (\n", name.c_str());
            unsigned Idx = 0;
            for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
              fprintf(OStr, "%s", printType(*I, false, fieldName(STy, Idx++), "  ", ";\n"));
                for (std::map<Function *, std::string>::iterator FI = table->method.begin(); FI != table->method.end(); FI++) {
                    VTABLE_WORK foo(FI->first, NULL);
                    regen_methods = 1;
                    processFunction(foo, 1, FI->second.c_str(), OStr);
                    regen_methods = 0;
                }
            fprintf(OStr, "endmodule \n\n");
            }
        }
}
void generateVerilogHeader(Module &Mod, FILE *OStr, FILE *ONull)
{
    structWork_run = 1;
    while (structWork.begin() != structWork.end()) {
        printContainedStructs(*structWork.begin(), OStr);
        structWork.pop_front();
    }
    structWork_run = 0;
}
