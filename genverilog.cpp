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

std::map<std::string,const Type *> referencedItems;

/*
 * Generate Verilog output for Store and Call instructions
 */
const char *generateVerilog(Function ***thisp, Instruction &I)
{
    char vout[MAX_CHAR_BUFFER];
    int opcode = I.getOpcode();
    vout[0] = 0;
    switch(opcode) {
    // Terminators
    case Instruction::Ret:
        if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
            sprintf(vout, "%s = ", globalName);
            if (I.getNumOperands())
                strcat(vout, writeOperand(thisp, I.getOperand(0), false));
            strcat(vout, ";");
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
          strcat(vout, ") & ");
          strcat(vout, writeOperand(thisp, BitMask, false));
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
        pushWork(func, called_thisp);
        int hasRet = !func || (func->getReturnType() != Type::getVoidTy(func->getContext()));
        int skip = regen_methods;
        std::string prefix;
        std::map<Function *,ClassMethodTable *>::iterator NI = functionIndex.find(func);
        if (NI != functionIndex.end()) {
            p = writeOperand(thisp, *AI, false);
            if (p[0] == '&')
                p++;
            std::string pnew = p;
            referencedItems[pnew] = func->getType();
            prefix = p + NI->second->method[func];
            strcat(vout, prefix.c_str());
            if (!hasRet)
                strcat(vout, "_ENA = 1");
            skip = 1;
        }
        else {
            strcat(vout, p);
            if (regen_methods) {
                strcat(vout, ";");
                break;
            }
        }
        if (RF)
            strcat(vout, ")");
        PointerType  *PTy = (func) ? cast<PointerType>(func->getType()) : cast<PointerType>(Callee->getType());
        FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
        ERRORIF(FTy->isVarArg() && !FTy->getNumParams());
        unsigned len = FTy->getNumParams();
        if (prefix == "")
            strcat(vout, "(");
        Function::const_arg_iterator FAI = func->arg_begin(), FAE = func->arg_end();
        for (; AI != AE; ++AI, ++ArgNo, FAI++) {
            if (!skip) {
            strcat(vout, sep);
            const char *p = writeOperand(thisp, *AI, false);
            if (prefix != "") {
                strcat(vout, ";\n            ");
                strcat(vout, prefix.c_str());
                strcat(vout, "_");
                strcat(vout, FAI->getName().str().c_str());
                strcat(vout, " = ");
            }
            else {
            if (ArgNo < len && (*AI)->getType() != FTy->getParamType(ArgNo))
                strcat(vout, printType(FTy->getParamType(ArgNo), /*isSigned=*/false, "", "(", ")"));
            sep = ", ";
            }
            strcat(vout, p);
            }
            skip = 0;
        }
        if (prefix == "")
            strcat(vout, ")");
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
        }
        break;
#endif
    default:
        return processCInstruction(thisp, I);
        printf("Verilog Other opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
        break;
    }
    if (vout[0])
        return strdup(vout);
    return NULL;
}
static void generateModuleSignature(std::string name, FILE *OStr, ClassMethodTable *table, const char *instance)
{
    const char *inp = "input ";
    const char *outp = "output ";
    if (instance) {
        inp = instance;
        outp = instance;
        fprintf(OStr, "%s %s (\n", name.c_str(), instance);
    }
    else
        fprintf(OStr, "module %s (\n", name.c_str());
    fprintf(OStr, "    %sCLK,\n", inp);
    fprintf(OStr, "    %sRST,\n", inp);
    for (std::map<Function *, std::string>::iterator FI = table->method.begin(); FI != table->method.end(); FI++) {
        Function *func = FI->first;
        std::string mname = FI->second;
        int hasRet = (func->getReturnType() != Type::getVoidTy(func->getContext()));
        if (hasRet)
            fprintf(OStr, "    %s%s,\n", outp, mname.c_str());
        else
            fprintf(OStr, "    %s%s_ENA,\n", inp, mname.c_str());
        int skip = 1;
        for (Function::const_arg_iterator AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
            if (!skip)
                fprintf(OStr, "    %s%s_%s,\n", inp, mname.c_str(), AI->getName().str().c_str());
            skip = 0;
        }
    }
    fprintf(OStr, ")\n\n");
}

void generateModuleDef(const StructType *STy, FILE *OStr)
{
    std::string name = getStructName(STy);
    unsigned Idx = 0;
    ClassMethodTable *table = findClass(name);

printf("[%s:%d] name %s table %p\n", __FUNCTION__, __LINE__, name.c_str(), table);
    if (!table)
        return;
    generateModuleSignature(name, OStr, table, NULL);
    for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
        fprintf(OStr, "%s", printType(*I, false, fieldName(STy, Idx++), "  ", ";\n"));
    fprintf(OStr, "  always @( posedge CLK) begin\n    if RST then begin\n    end\n    else begin\n");
    for (std::map<Function *, std::string>::iterator FI = table->method.begin(); FI != table->method.end(); FI++) {
        Function *func = FI->first;
        std::string mname = FI->second;
        int hasRet = (func->getReturnType() != Type::getVoidTy(func->getContext()));
        fprintf(OStr, "        // Start of %s\n", mname.c_str());
        if (!hasRet)
            fprintf(OStr, "        if (%s_ENA) begin\n", mname.c_str());
        regen_methods = 1;
        VTABLE_WORK foo(func, NULL);
        processFunction(foo, mname.c_str(), OStr);
        regen_methods = 0;
        if (!hasRet)
            fprintf(OStr, "        end; // End of %s\n", mname.c_str());
        fprintf(OStr, "\n");
    }
    fprintf(OStr, "  end // always @ (posedge CLK)\nendmodule \n\n");
}
void generateVerilogHeader(Module &Mod, FILE *OStr, FILE *ONull)
{
    for (std::map<std::string,const Type *>::iterator RI = referencedItems.begin(); RI != referencedItems.end(); RI++) {
        const FunctionType *FTy;
        const PointerType *PTy;
        const StructType *STy;
        if ((PTy = dyn_cast<PointerType>(RI->second))
         && (FTy = dyn_cast<FunctionType>(PTy->getPointerElementType()))
         && (PTy = dyn_cast<PointerType>(FTy->getParamType(0)))
         && (STy = dyn_cast<StructType>(PTy->getElementType()))) {
            std::string name = getStructName(STy);
            ClassMethodTable *table = findClass(name);
printf("[%s:%d] name %s type %p table %p\n", __FUNCTION__, __LINE__, name.c_str(), RI->second, table);
            if (table && RI->first != "Vthis")
                generateModuleSignature(name, OStr, table, RI->first.c_str());
        }
    }
}
