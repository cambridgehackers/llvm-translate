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
    std::string condition;
    std::string value;
} MUX_VALUE;

static int dontInlineValues;//=1;
static std::map<std::string, std::list<MUX_VALUE>> muxValueList;
static std::map<std::string, std::string> assignList;

std::string verilogArrRange(const Type *Ty)
{
    unsigned NumBits = cast<IntegerType>(Ty)->getBitWidth();
    if (NumBits > 8)
        return "[" + utostr(NumBits - 1) + ":0]";
    return "";
}
static bool findExact(std::string haystack, std::string needle)
{
    std::string::size_type sz = haystack.find(needle);
    if (sz == std::string::npos)
        return false;
    sz += needle.length();
    if (isalnum(haystack[sz]) || haystack[sz] == '_' || haystack[sz] == '$')
        return findExact(haystack.substr(sz), needle);
    return true;
}

/*
 * lookup/replace values for class variables that are assigned only 1 time.
 */
static std::string inlineValue(std::string wname, bool clear)
{
    if (dontInlineValues)
        return clear ? wname : "";
    std::string temp = assignList[wname];
    if (temp == "") {
        std::string exactMatch;
        int referenceCount = 0;
        for (auto item: assignList) {
            if (item.second == wname)
                exactMatch = item.first;
            if (findExact(item.second, wname))
                referenceCount++;
        }
        if (referenceCount == 1 && exactMatch != "") {
            if (clear)
                assignList[exactMatch] = "";
            return exactMatch;
        }
    }
    if (clear) {
        if (temp != "")
            assignList[wname] = "";
        else
            return wname;
    }
    return temp;
}

/*
 * Generate verilog module header for class definition or reference
 */
void generateModuleSignature(FILE *OStr, const StructType *STy, std::string instance)
{
    ClassMethodTable *table = classCreate[STy];
    std::string name = getStructName(STy);
    std::string inp = "input ", outp = "output ", instPrefix, inpClk = "input ";
    std::list<std::string> paramList;
    if (instance != "") {
        instPrefix = instance + MODULE_SEPARATOR;
        inp = instPrefix;
        outp = instPrefix;
        inpClk = "";
        for (auto FI : table->method) {
            Function *func = FI.second;
            std::string mname = FI.first;
            const Type *retTy = func->getReturnType();
            std::string arrRange;
            int isAction = (retTy == Type::getVoidTy(func->getContext()));
            std::string wparam = inp + mname + (isAction ? "__ENA" : "");
            if (!isAction)
                arrRange = verilogArrRange(retTy);
            if (inlineValue(wparam, false) == "")
                fprintf(OStr, "    wire %s%s;\n", arrRange.c_str(), wparam.c_str());
            for (auto AI = ++func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
                wparam = instPrefix + AI->getName().str();
                if (inlineValue(wparam, false) == "")
                    fprintf(OStr, "    wire %s%s;\n", verilogArrRange(AI->getType()).c_str(), wparam.c_str());
            }
        }
    }
    paramList.push_back(inpClk + "CLK");
    paramList.push_back(inpClk + "nRST");
    for (auto FI : table->method) {
        Function *func = FI.second;
        std::string mname = FI.first;
        const Type *retTy = func->getReturnType();
        int isAction = (retTy == Type::getVoidTy(func->getContext()));
        std::string wparam = inp + mname + (isAction ? "__ENA" : "");
        if (instance != "")
            wparam = inlineValue(wparam, true);
        else if (!isAction)
            wparam = outp + verilogArrRange(retTy) + mname;
        paramList.push_back(wparam);
        for (auto AI = ++func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
            if (instance != "")
                wparam = inlineValue(instPrefix + AI->getName().str(), true);
            else
                wparam = inp + verilogArrRange(AI->getType()) + AI->getName().str();
            paramList.push_back(wparam);
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
            if (const StructType *STy = dyn_cast<StructType>(element)) { // calling indications to C++ from hardware
                if (ClassMethodTable *table = classCreate[STy]) {
                for (auto FI : table->method) {
                    Function *func = FI.second;
                    std::string wparam, mname = fname + MODULE_SEPARATOR + FI.first;
                    const Type *retTy = func->getReturnType();
                    int isAction = (retTy == Type::getVoidTy(func->getContext()));
                    if (isAction)
                        wparam = outp + mname + "__ENA";
                    else
                        wparam = inp + (instance == "" ? verilogArrRange(retTy):"") + mname;
                    paramList.push_back(wparam);
                    for (auto AI = ++func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI) {
                        wparam = outp + (instance == "" ? verilogArrRange(AI->getType()):"") + mname + "_" + AI->getName().str();
                        paramList.push_back(wparam);
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

/*
 * Generate BSV file for generated class definition.
 */
void generateBsvWrapper(const StructType *STy, std::string oDir)
{
    std::string name = getStructName(STy);
    ClassMethodTable *table = classCreate[STy];
    /*
     * Now generate importBVI for use of module from Bluespec
     */
    FILE *BStr = fopen((oDir + "/" + ucName(name) + ".bsv").c_str(), "w");
    fprintf(BStr, "interface %s;\n", ucName(name).c_str());
    for (auto FI : table->method) {
        Function *func = FI.second;
        std::string mname = FI.first;
        int isAction = (func->getReturnType() == Type::getVoidTy(func->getContext()));
        if (endswith(mname, "__RDY"))
            continue;
        fprintf(BStr, "    method %s %s(", !isAction ? "Bit#(32)" : "Action", mname.c_str());
        for (auto AI = ++func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI)
            fprintf(BStr, "%s %s", "Bit#(32)", AI->getName().str().c_str());
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
        int isAction = (func->getReturnType() == Type::getVoidTy(func->getContext()));
        if (isAction)
            fprintf(BStr, "    method %s(", mname.c_str());
        else
            fprintf(BStr, "    method %s %s(", mname.c_str(), mname.c_str());
        for (auto AI = ++func->arg_begin(), AE = func->arg_end(); AI != AE; ++AI)
            fprintf(BStr, "%s", AI->getName().str().c_str());
        if (isAction)
            fprintf(BStr, ") enable(%s__ENA", mname.c_str());
        fprintf(BStr, ") ready(%s__RDY);\n", mname.c_str());
        sched += sep + mname;
        sep = ", ";
    }
    fprintf(BStr, "    schedule (%s) CF (%s);\n", sched.c_str(), sched.c_str());
    fprintf(BStr, "endmodule\n");
    fclose(BStr);
}

static std::string globalCondition;
// 'Or' together ENA lines from all invocations of a method from this class
void muxEnable(BasicBlock *bb, std::string signal)
{
     std::string tempCond = globalCondition;
     if (Value *cond = getCondition(bb, 0))
         tempCond += " & " + printOperand(cond, false);
     if (assignList[signal] != "")
         assignList[signal] += " || ";
     assignList[signal] += tempCond;
}
// 'Mux' together parameter settings from all invocations of a method from this class
void muxValue(BasicBlock *bb, std::string signal, std::string value)
{
     std::string tempCond = globalCondition;
     if (Value *cond = getCondition(bb, 0))
         tempCond += " & " + printOperand(cond, false);
     muxValueList[signal].push_back(MUX_VALUE{tempCond, value});
}

void generateModuleDef(const StructType *STy, std::string oDir)
{
    std::list<std::string> metaList;
    std::string name = getStructName(STy);
    ClassMethodTable *table = classCreate[STy];
    std::list<std::string> alwaysLines;

    muxValueList.clear();
    assignList.clear();
    FILE *OStr = fopen((oDir + "/" + name + ".v").c_str(), "w");
    generateModuleSignature(OStr, STy, "");
    // generate wires for internal methods RDY/ENA.  Collect state element assignments
    // from each method
    for (auto FI : table->method) {
        Function *func = FI.second;
        std::string mname = FI.first;
        int isAction = (func->getReturnType() == Type::getVoidTy(func->getContext()));
        globalCondition = mname + "__ENA_internal";
        processFunction(func);
        if (!isAction) {
            std::string temp;
            for (auto info: functionList) {
                ERRORIF(getCondition(info.cond, 0));
                temp += info.item;
            }
            assignList[mname] = temp;  // collect the text of the return value into a single 'assign'
            if (endswith(mname, "__RDY"))
                metaList.push_back("//METAGUARD; " + mname + "; " + temp + ";");
        }
        else {
            // generate RDY_internal wire so that we can reference RDY expression inside module
            // generate ENA_internal wire that is and'ed with RDY_internal, so that we can
            // never be enabled unless we were actually ready.
            fprintf(OStr, "    wire %s__RDY_internal;\n", mname.c_str());
            fprintf(OStr, "    wire %s__ENA_internal = %s__ENA && %s__RDY_internal;\n", mname.c_str(), mname.c_str(), mname.c_str());
            fprintf(OStr, "    assign %s__RDY = %s__RDY_internal;\n", mname.c_str(), mname.c_str());
            if (functionList.size() > 0) {
                printf("%s: non-store lines in Action\n", __FUNCTION__);
                for (auto info: functionList) {
                    if (Value *cond = getCondition(info.cond, 0))
                        printf("%s\n", ("    if (" + printOperand(cond, false) + ")").c_str());
                    printf("%s\n", info.item.c_str());
                }
                exit(-1);
            }
        }
        if (storeList.size() > 0) {
            alwaysLines.push_back("if (" + mname + "__ENA_internal) begin");
            for (auto info: storeList) {
                if (Value *cond = getCondition(info.second.cond, 0))
                    alwaysLines.push_back("    if (" + printOperand(cond, false) + ")");
                alwaysLines.push_back("    " + info.first + " <= " + info.second.item + ";");
            }
            alwaysLines.push_back("end; // End of " + mname);
        }
        // gather up metadata generated by processFunction
        if (!endswith(mname, "__RDY")) {
            std::list<ReferenceType> tempRead; // to ensure that 'printOperand' doesn't add more things!
            std::string temp;
            for (auto item: readList)
                tempRead.push_back(item);
            for (auto item: tempRead)
                temp += ":" + printOperand(getCondition(item.cond, 0),false) + ";" + item.item;
            if (temp != "")
                metaList.push_back("//METAREAD; " + mname + "; " + temp + ";");
            temp = "";
            for (auto item: writeList)
                temp += ":" + printOperand(getCondition(item.cond, 0),false) + ";" + item.item;
            if (temp != "")
                metaList.push_back("//METAWRITE; " + mname + "; " + temp + ";");
            temp = "";
            for (auto item: invokeList)
                temp += ":" + printOperand(getCondition(item.cond, 0),false) + ";" + item.item;
            if (temp != "")
                metaList.push_back("//METAINVOKE; " + mname + "; " + temp + ";");
        }
    }
    // combine mux'ed assignments into a single 'assign' statement
    // Context: before local state declarations, to allow inlining
    for (auto item: muxValueList) {
        int remain = item.second.size();
        std::string temp;
        for (auto element: item.second) {
            if (--remain)
                temp += element.condition + " ? ";
            temp += element.value;
            if (remain)
                temp += " : ";
        }
        assignList[item.first] = temp;
    }
    // generate local state element declarations
    int Idx = 0;
    std::list<std::string> resetList;
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        const Type *element = *I;
        if (const Type *newType = table->replaceType[Idx])
            element = newType;
        std::string fname = fieldName(STy, Idx);
        if (fname != "") {
            if (const PointerType *PTy = dyn_cast<PointerType>(element)) {
                if (const StructType *STy = dyn_cast<StructType>(PTy->getElementType()))
                    metaList.push_back("//METAEXTERNAL; " + fname + "; " + getStructName(STy) + ";");
            }
            else if (const StructType *STy = dyn_cast<StructType>(element)) {
                if (!inheritsModule(STy, "class.InterfaceClass")) {
                    generateModuleSignature(OStr, STy, fname);
                    metaList.push_back("//METAINTERNAL; " + fname + "; " + getStructName(STy) + ";");
                }
            }
            else {
                fprintf(OStr, "    %s;\n", printType(element, false, fname, "", "", false).c_str());
                resetList.push_back(fname);
            }
        }
    }
    // generate 'assign' items
    for (auto item: assignList)
        if (item.second != "") {
            std::string temp = item.first.c_str();
            if (endswith(temp, "__RDY"))
                temp += "_internal";
            fprintf(OStr, "    assign %s = %s;\n", temp.c_str(), item.second.c_str());
        }
    // generate clocked updates to state elements
    if (resetList.size() > 0 || alwaysLines.size() > 0) {
        fprintf(OStr, "\n    always @( posedge CLK) begin\n      if (!nRST) begin\n");
        for (auto item: resetList)
            fprintf(OStr, "        %s <= 0;\n", item.c_str());
        fprintf(OStr, "      end // nRST\n");
        if (alwaysLines.size() > 0) {
            fprintf(OStr, "      else begin\n");
            for (auto info: alwaysLines)
                fprintf(OStr, "        %s\n", info.c_str());
            fprintf(OStr, "      end\n");
        }
        fprintf(OStr, "    end // always @ (posedge CLK)\n");
    }
    fprintf(OStr, "endmodule \n\n");
    // write out metadata comments at end of the file
    for (auto item : metaList)
        fprintf(OStr, "%s\n", item.c_str());
    fclose(OStr);
    generateBsvWrapper(STy, oDir);
}
