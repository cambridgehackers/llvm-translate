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

typedef struct {
    Function *func;
    std::string name;
} READY_INFO;

std::string verilogArrRange(const Type *Ty)
{
    unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
    if (NumBits > 8)
        return "[" + utostr(NumBits - 1) + ":0]";
    return "";
}
void generateModuleSignature(FILE *OStr, const StructType *STy, std::string instance)
{
    ClassMethodTable *table = classCreate[STy];
    std::string name = getStructName(STy);
    std::string inp = "input ", outp = "output ";
    std::list<std::string> paramList;
    if (instance != "") {
        inp = instance + MODULE_SEPARATOR;
        outp = instance + MODULE_SEPARATOR;
    }
    paramList.push_back(inp + "CLK");
    paramList.push_back(inp + "nRST");
    for (auto FI : table->method) {
        Function *func = FI.second;
        std::string mname = FI.first;
        const Type *retTy = func->getReturnType();
        int hasRet = (retTy != Type::getVoidTy(func->getContext()));
        if (hasRet) {
            paramList.push_back(outp + (instance == "" ? verilogArrRange(retTy):"") + mname);
        }
        else
            paramList.push_back(inp + mname + "__ENA");
        int skip = 1;
        for (auto AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
            if (!skip)
                paramList.push_back(inp + (instance == "" ? verilogArrRange(AI->getType()):"") + mname + MODULE_SEPARATOR + AI->getName().str());
            skip = 0;
        }
    }
    int Idx = 0;
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        const Type *element = *I;
        if (const Type *newType = table->replaceType[Idx])
            element = newType;
        std::string fname = fieldName(STy, Idx);
        if (fname != "")
        if (const PointerType *PTy = dyn_cast<PointerType>(element)) {
            element = PTy->getElementType();
            if (const StructType *STy = dyn_cast<StructType>(element)) {
                if (ClassMethodTable *table = classCreate[STy]) {
                int Idx = 0;
                for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
                    const Type *element = *I;
                        if (const Type *newType = table->replaceType[Idx])
                            element = newType;
                    std::string ename = fieldName(STy, Idx);
                    if (ename != "")
                        paramList.push_back(outp + printType(element, false, fname + MODULE_SEPARATOR + ename, "  ", "", false));
                }
                for (auto FI : table->method) {
                    Function *func = FI.second;
                    std::string mname = fname + MODULE_SEPARATOR + FI.first;
                    const Type *retTy = func->getReturnType();
                    int hasRet = (retTy != Type::getVoidTy(func->getContext()));
                    if (hasRet)
                        paramList.push_back(inp + (instance == "" ? verilogArrRange(retTy):"") + mname);
                    else
                        paramList.push_back(outp + mname + "__ENA");
                    int skip = 1;
                    for (auto AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
                        if (!skip)
                            paramList.push_back(outp + (instance == "" ? verilogArrRange(AI->getType()):"") + mname + MODULE_SEPARATOR + AI->getName().str());
                        skip = 0;
                    }
                }
                }
            }
            else
                paramList.push_back(outp + printType(element, false, fname, "  ", "", false));
        }
    }
    if (instance != "")
        fprintf(OStr, "    %s %s (\n", name.c_str(), instance.c_str());
    else
        fprintf(OStr, "module %s (\n", name.c_str());
    for (auto PI = paramList.begin(); PI != paramList.end();) {
        if (instance != "")
            fprintf(OStr, "    ");
        fprintf(OStr, "    %s", PI->c_str());
        PI++;
        if (PI != paramList.end())
            fprintf(OStr, ",\n");
    }
    fprintf(OStr, ");\n");
}

void generateModuleDef(const StructType *STy, FILE *aOStr, std::string oDir)
{
    std::string name = getStructName(STy);
    ClassMethodTable *table = classCreate[STy];
    std::list<READY_INFO> rdyList;
    std::list<std::string> readWriteList;

    FILE *OStr = fopen((oDir + "/" + name + ".v").c_str(), "w");
    generateModuleSignature(OStr, STy, "");
    for (auto FI : table->method) {
        Function *func = FI.second;
        std::string mname = FI.first;
        int hasRet = (func->getReturnType() != Type::getVoidTy(func->getContext()));
        if (hasRet) {
            if (endswith(mname, "__RDY"))
                rdyList.push_back(READY_INFO{func, mname});
        }
    }
    int Idx = 0;
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        const Type *element = *I;
        if (const Type *newType = table->replaceType[Idx])
            element = newType;
        std::string fname = fieldName(STy, Idx);
        if (fname != "") {
            if (const PointerType *PTy = dyn_cast<PointerType>(element)) {
                if (const StructType *STy = dyn_cast<StructType>(PTy->getElementType()))
                    readWriteList.push_back("//METAEXTERNAL " + fname + "; " + getStructName(STy) + ";");
            }
            else if (const StructType *STy = dyn_cast<StructType>(element)) {
                generateModuleSignature(OStr, STy, fname);
                readWriteList.push_back("//METAINTERNAL " + fname + "; " + getStructName(STy) + ";");
            }
            else
                fprintf(OStr, "%s", printType(element, false, fname, "  ", ";\n", false).c_str());
        }
    }
    fprintf(OStr, "    always @( posedge CLK) begin\n      if (!nRST) begin\n      end\n      else begin\n");
    for (auto FI : table->method) {
        Function *func = FI.second;
        std::string mname = FI.first;
        int isAction = (func->getReturnType() == Type::getVoidTy(func->getContext()));
        fprintf(OStr, "        // Method: %s\n", mname.c_str());
        if (isAction)
            fprintf(OStr, "        if (%s__ENA) begin\n", mname.c_str());
        else
            fprintf(OStr, "             %s = ", mname.c_str());
        processFunction(func, OStr);
        std::string condition;
        if (!endswith(mname, "__RDY")) {
            std::string temp;
            for (auto item: readList)
                temp += ":" + item;
            if (temp != "")
                readWriteList.push_back("//METAREAD " + mname + "; " + "(" + condition + ")" + temp + ";");
            temp = "";
            for (auto item: writeList)
                temp += ":" + item;
            if (temp != "")
                readWriteList.push_back("//METAWRITE " + mname + "; " + "(" + condition + ")" + temp + ";");
            temp = "";
            for (auto item: invokeList)
                temp += ":" + item;
            if (temp != "")
                readWriteList.push_back("//METAINVOKE " + mname + "; " + "(" + condition + ")" + temp + ";");
        }
        if (isAction)
            fprintf(OStr, "        end; // End of %s\n", mname.c_str());
        fprintf(OStr, "\n");
    }
    fprintf(OStr, "      end; // nRST\n    end; // always @ (posedge CLK)\nendmodule \n\n");
    for (auto PI = rdyList.begin(); PI != rdyList.end(); PI++) {
        fprintf(OStr, "//METAGUARD %s; ", PI->name.c_str());
        processFunction(PI->func, OStr);
    }
    for (auto item : readWriteList)
        fprintf(OStr, "%s\n", item.c_str());
    fclose(OStr);

    /*
     * Now generate importBVI for use of module from Bluespec
     */
    FILE *BStr = fopen((oDir + "/" + ucName(name) + ".bsv").c_str(), "w");
    fprintf(BStr, "interface %s;\n", ucName(name).c_str());
    for (auto FI : table->method) {
        Function *func = FI.second;
        std::string mname = FI.first;
        int hasRet = (func->getReturnType() != Type::getVoidTy(func->getContext()));
        if (endswith(mname, "__RDY"))
            continue;
        fprintf(BStr, "    method %s %s(", hasRet ? "Bit#(32)" : "Action", mname.c_str());
        int skip = 1;
        for (auto AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
            if (!skip) {
                //const Type *Ty = AI->getType();
                //paramList.push_back(inp + verilogArrRange(Ty) + 
                fprintf(BStr, "%s %s", "Bit#(32)", AI->getName().str().c_str());
            }
            skip = 0;
        }
        fprintf(BStr, ");\n");
    }
    fprintf(BStr, "endinterface\nimport \"BVI\" %s =\nmodule mk%s(%s);\n", name.c_str(), ucName(name).c_str(), ucName(name).c_str());
    fprintf(BStr, "    default_reset rst(nRST);\n    default_clock clk(CLK);\n");
    std::string sched = "", sep = "";
    for (auto FI : table->method) {
        Function *func = FI.second;
        std::string mname = FI.first;
        if (endswith(mname, "__RDY"))
            continue;
        int hasRet = (func->getReturnType() != Type::getVoidTy(func->getContext()));
        if (hasRet)
            fprintf(BStr, "    method %s %s(", mname.c_str(), mname.c_str());
        else
            fprintf(BStr, "    method %s(", mname.c_str());
        int skip = 1;
        for (auto AI = func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
            if (!skip) {
                //const Type *Ty = AI->getType();
                //paramList.push_back(inp + verilogArrRange(Ty) + 
                fprintf(BStr, "%s", (mname + MODULE_SEPARATOR + AI->getName().str()).c_str());
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
