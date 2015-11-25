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
#include "llvm/ADT/StringExtras.h"

using namespace llvm;

#include "declarations.h"

static std::map<std::string,Type *> referencedItems;

/*
 * Generate Verilog output for Store and Call instructions
 */
std::string generateVerilog(Function ***thisp, Instruction &I)
{
    std::string vout;
    int opcode = I.getOpcode();
    switch(opcode) {
    // Terminators
    case Instruction::Ret:
        if (I.getNumOperands() != 0 || I.getParent()->getParent()->size() != 1) {
            vout += "    " + globalName + " = ";
            if (I.getNumOperands())
                vout += printOperand(thisp, I.getOperand(0), false);
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
#endif

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
        vout += pdest + " <= ";
        if (BitMask)
          vout += "((";
//printf("[%s:%d] storeval %s found %p\n", __FUNCTION__, __LINE__, sval, nameMap[sval]);
        if (void *temp = nameMap[sval]) {
            sval = mapAddress(temp);
//printf("[%s:%d] second %p pname %s\n", __FUNCTION__, __LINE__, temp, sval);
        }
        vout += sval;
        if (BitMask)
          vout += ") & " + printOperand(thisp, BitMask, false) + ")";
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
        std::string cthisp = fetchOperand(thisp, *AI, false);
        Function ***called_thisp = (Function ***)mapLookup(cthisp.c_str());
        std::string p = printOperand(thisp, Callee, false);
        //printf("[%s:%d] p %s func %p thisp %p called_thisp %p\n", __FUNCTION__, __LINE__, p.c_str(), func, thisp, called_thisp);
        if (p == "printf")
            break;
        if (!strncmp(p.c_str(), "&0x", 3) && !func) {
            void *tval = mapLookup(p.c_str()+1);
            if (tval) {
                func = static_cast<Function *>(tval);
                if (func)
                    p = func->getName();
                //printf("[%s:%d] tval %p pnew %s\n", __FUNCTION__, __LINE__, tval, p.c_str());
            }
        }
        pushWork(func, called_thisp, 0);
        int hasRet = !func || (func->getReturnType() != Type::getVoidTy(func->getContext()));
        int skip = regen_methods;
        std::string prefix;
        if (ClassMethodTable *CMT = functionIndex[func]) {
            p = printOperand(thisp, *AI, false);
            if (p[0] == '&')
                p = p.substr(1);
            std::string pnew = p;
            referencedItems[pnew] = func->getType();
            prefix = p + CMT->method[func];
            vout += prefix;
            if (!hasRet)
                vout += "__ENA = 1";
            skip = 1;
        }
        else {
            vout += p;
            if (regen_methods) {
                break;
            }
        }
        PointerType  *PTy = (func) ? cast<PointerType>(func->getType()) : cast<PointerType>(Callee->getType());
        FunctionType *FTy = cast<FunctionType>(PTy->getElementType());
        ERRORIF(FTy->isVarArg() && !FTy->getNumParams());
        unsigned len = FTy->getNumParams();
        if (prefix == "")
            vout += "(";
        if (func) {
        Function::const_arg_iterator FAI = func->arg_begin();
        for (; AI != AE; ++AI, ++ArgNo, FAI++) {
            if (!skip) {
                vout += sep;
                std::string p = printOperand(thisp, *AI, false);
                if (prefix != "")
                    vout += (";\n            " + prefix + "_" + FAI->getName().str() + " = ");
                else {
                    ERRORIF (ArgNo < len && (*AI)->getType() != FTy->getParamType(ArgNo));
                    sep = ", ";
                }
                vout += p;
            }
            skip = 0;
        }
        }
        if (prefix == "")
            vout += ")";
        break;
        }

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
        return processCInstruction(thisp, I);
        printf("Verilog Other opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
        break;
    }
    return vout;
}

std::string verilogArrRange(const Type *Ty)
{
unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
    if (NumBits > 8)
        return "[" + utostr(NumBits - 1) + ":0]";
    return "";
}
static void generateModuleSignature(std::string name, FILE *OStr, ClassMethodTable *table, const char *instance)
{
    std::list<std::string> paramList;
    std::string inp = "input ";
    std::string outp = "output ";
    if (instance) {
        inp = instance;
        outp = instance;
        fprintf(OStr, "%s %s (\n", name.c_str(), instance);
    }
    else
        fprintf(OStr, "module %s (\n", name.c_str());
    paramList.push_back(inp + "CLK");
    paramList.push_back(inp + "nRST");
    for (std::map<Function *, std::string>::iterator FI = table->method.begin(); FI != table->method.end(); FI++) {
        Function *func = FI->first;
        std::string mname = FI->second;
        const Type *retTy = func->getReturnType();
        int hasRet = (retTy != Type::getVoidTy(func->getContext()));
        if (hasRet)
            paramList.push_back(outp + verilogArrRange(retTy) + mname);
        else
            paramList.push_back(inp + mname + "__ENA");
        int skip = 1;
        for (Function::const_arg_iterator AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
            if (!skip) {
                const Type *Ty = AI->getType();
                paramList.push_back(inp + verilogArrRange(Ty) + mname + "_" + AI->getName().str());
            }
            skip = 0;
        }
    }
    for (std::list<std::string>::iterator PI = paramList.begin(); PI != paramList.end();) {
        fprintf(OStr, "    %s", PI->c_str());
        PI++;
        if (PI != paramList.end())
            fprintf(OStr, ",");
        else
            fprintf(OStr, ");");
        fprintf(OStr, "\n");
    }
    fprintf(OStr, "\n");
}

void generateModuleDef(const StructType *STy, std::string oDir)
{
    std::string name = getStructName(STy);
    ClassMethodTable *table = classCreate[name];

printf("[%s:%d] name %s table %p\n", __FUNCTION__, __LINE__, name.c_str(), table);
    if (!table || !inheritsModule(STy)
     || table->method.begin() == table->method.end())
        return;
    FILE *OStr = fopen((oDir + "/" + name + ".v").c_str(), "w");
    generateModuleSignature(name, OStr, table, NULL);
    int Idx = 0;
    for (StructType::element_iterator I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        std::string fname = fieldName(STy, Idx);
        if (fname != "")
            fprintf(OStr, "%s", printType(*I, false, fname, "  ", ";\n").c_str());
    }
    fprintf(OStr, "  always @( posedge CLK) begin\n    if (!nRST) begin\n    end\n    else begin\n");
    for (std::map<Function *, std::string>::iterator FI = table->method.begin(); FI != table->method.end(); FI++) {
        Function *func = FI->first;
        std::string mname = FI->second;
        int hasRet = (func->getReturnType() != Type::getVoidTy(func->getContext()));
        fprintf(OStr, "        // Method: %s\n", mname.c_str());
        if (!hasRet)
            fprintf(OStr, "        if (%s__ENA) begin\n", mname.c_str());
        regen_methods = 1;
        VTABLE_WORK foo(func, NULL, 1);
        processFunction(foo, OStr, "");
        regen_methods = 0;
        if (!hasRet)
            fprintf(OStr, "        end; // End of %s\n", mname.c_str());
        fprintf(OStr, "\n");
    }
    fprintf(OStr, "    end; // nRST\n");
    fprintf(OStr, "  end; // always @ (posedge CLK)\nendmodule \n\n");
    fclose(OStr);
    /*
     * Now generate importBVI for use of module from Bluespec
     */
    FILE *BStr = fopen((oDir + "/" + ucName(name) + ".bsv").c_str(), "w");
    fprintf(BStr, "interface %s;\n", ucName(name).c_str());
    for (std::map<Function *, std::string>::iterator FI = table->method.begin(); FI != table->method.end(); FI++) {
        Function *func = FI->first;
        std::string mname = FI->second;
        int hasRet = (func->getReturnType() != Type::getVoidTy(func->getContext()));
        if (mname.length() > 5 && mname.substr(mname.length() - 5, 5) == "__RDY")
            continue;
        fprintf(BStr, "    method %s %s(", hasRet ? "Bit#(32)" : "Action", mname.c_str());
        int skip = 1;
        for (Function::const_arg_iterator AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
            if (!skip) {
                //const Type *Ty = AI->getType();
                //paramList.push_back(inp + verilogArrRange(Ty) + 
                fprintf(BStr, "%s %s", "Bit#(32)", AI->getName().str().c_str());
            }
            skip = 0;
        }
        fprintf(BStr, ");\n");
    }
    fprintf(BStr, "endinterface\n");
    fprintf(BStr, "import \"BVI\" %s =\nmodule mk%s(%s);\n", name.c_str(), ucName(name).c_str(), ucName(name).c_str());
    fprintf(BStr, "    default_reset rst(nRST);\n    default_clock clk(CLK);\n");
    std::string sched = "";
    std::string sep = "";
    for (std::map<Function *, std::string>::iterator FI = table->method.begin(); FI != table->method.end(); FI++) {
        Function *func = FI->first;
        std::string mname = FI->second;
        if (mname.length() > 5 && mname.substr(mname.length() - 5, 5) == "__RDY")
            continue;
        int hasRet = (func->getReturnType() != Type::getVoidTy(func->getContext()));
        if (hasRet)
            fprintf(BStr, "    method %s %s(", mname.c_str(), mname.c_str());
        else
            fprintf(BStr, "    method %s(", mname.c_str());
        int skip = 1;
        for (Function::const_arg_iterator AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
            if (!skip) {
                //const Type *Ty = AI->getType();
                //paramList.push_back(inp + verilogArrRange(Ty) + 
                fprintf(BStr, "%s", (mname + "_" + AI->getName().str()).c_str());
            }
            skip = 0;
        }
        if (!hasRet)
            fprintf(BStr, ") enable(%s__ENA", mname.c_str());
        fprintf(BStr, ") ready(%s__RDY);\n", mname.c_str());
        sched += sep + mname;
        sep = ", ";
    }
    fprintf(BStr, "    schedule (%s) CF (%s);\n", sched.c_str(), sched.c_str());
    fprintf(BStr, "endmodule\n");
    fclose(BStr);
}
void generateVerilogHeader(Module &Mod, FILE *OStr, FILE *ONull)
{
    for (std::map<std::string,Type *>::iterator RI = referencedItems.begin(); RI != referencedItems.end(); RI++) {
        const StructType *STy;
        if ((STy = findThisArgumentType(dyn_cast<PointerType>(RI->second)))) {
            std::string name = getStructName(STy);
            ClassMethodTable *table = classCreate[name];
printf("[%s:%d] name %s type %p table %p\n", __FUNCTION__, __LINE__, name.c_str(), RI->second, table);
            if (table && RI->first != "Vthis")
                generateModuleSignature(name, OStr, table, RI->first.c_str());
        }
    }
}
