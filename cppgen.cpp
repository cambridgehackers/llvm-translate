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
#include "llvm/IR/Instructions.h"

using namespace llvm;

#include "declarations.h"

/*
 * Recursively generate element definitions for a class.
 */
static void generateClassElements(const StructType *STy, FILE *OStr)
{
    int Idx = 0;
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        Type *element = *I;
        if (ClassMethodTable *table = classCreate[STy])
            if (Type *newType = table->replaceType[Idx])
                element = newType;
        std::string fname = fieldName(STy, Idx);
        if (fname == "unused_data_to_force_inheritance")
            continue;
        if (fname != "") {
            if (const StructType *iSTy = dyn_cast<StructType>(element))
                if (inheritsModule(iSTy, "class.InterfaceClass"))
                    continue;
            fprintf(OStr, "%s", printType(element, false, fname, "  ", ";\n", false).c_str());
        }
        else if (const StructType *inherit = dyn_cast<StructType>(element))
            generateClassElements(inherit, OStr);
    }
}

/*
 * Generate string for class method declaration
 */
static std::string printFunctionSignature(const Function *F, std::string altname)
{
    std::string sep, statstr, tstr = altname + '(';
    FunctionType *FT = cast<FunctionType>(F->getFunctionType());
    ERRORIF (F->hasDLLImportStorageClass() || F->hasDLLExportStorageClass() || FT->isVarArg());
// || F->hasStructRetAttr()
    if (F->hasLocalLinkage()) statstr = "static ";
    if (F->isDeclaration()) {
        auto AI = FT->param_begin()+1, AE = FT->param_end();
        if (F->hasStructRetAttr())
            AI++;
        for (; AI != AE; ++AI) {
            tstr += printType(*AI, /*isSigned=*/false, "", sep, "", false);
            sep = ", ";
        }
    }
    else {
        auto AI = ++F->arg_begin(), AE = F->arg_end();
        if (F->hasStructRetAttr())
            AI++;
        for (; AI != AE; ++AI) {
            tstr += printType(AI->getType(), /*isSigned=*/false, GetValueName(AI), sep, "", false);
            sep = ", ";
        }
    }
    if (sep == "")
        tstr += "void";
    return printType(F->getReturnType(), /*isSigned=*/false, tstr + ')', statstr, "", false);
}

/*
 * Generate class definition into output file.  Class methods are
 * only generated as prototypes.
 */
void generateClassDef(const StructType *STy, std::string oDir)
{
    std::list<std::string> runLines;
    std::list<std::string> extraMethods;
    ClassMethodTable *table = classCreate[STy];
    std::string name = getStructName(STy);

    // first generate '.h' file
    FILE *OStr = fopen((oDir + "/" + name + ".h").c_str(), "w");
    fprintf(OStr, "#ifndef __%s_H__\n#define __%s_H__\n", name.c_str(), name.c_str());
    int Idx = 0;
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        Type *element = *I;
        if (Type *newType = table->replaceType[Idx])
            element = newType;
        std::string fname = fieldName(STy, Idx);
        if (fname != "") {
            if (const StructType *iSTy = dyn_cast<StructType>(element))
                if (!inheritsModule(iSTy, "class.InterfaceClass")) {
                    fprintf(OStr, "#include \"%s.h\"\n", getStructName(iSTy).c_str());
                    runLines.push_back(fname);
                }
            if (const PointerType *PTy = dyn_cast<PointerType>(element)) {
                if (const StructType *iSTy = dyn_cast<StructType>(PTy->getElementType()))
                    fprintf(OStr, "#include \"%s.h\"\n", getStructName(iSTy).c_str());
                extraMethods.push_back("void set" + fname + "(" + printType(element, false, "v", "", "", false)
                    + ") { " + fname + " = v; }");
            }
        }
    }
    if (name.substr(0,12) == "l_struct_OC_")
        fprintf(OStr, "typedef struct {\n");
    else
        fprintf(OStr, "class %s {\n", name.c_str());
    generateClassElements(STy, OStr);
    fprintf(OStr, "public:\n  void run();\n");
    for (auto FI : table->method)
        fprintf(OStr, "  %s;\n", printFunctionSignature(FI.second, FI.first).c_str());
    for (auto item: extraMethods)
        fprintf(OStr, "  %s\n", item.c_str());
    fprintf(OStr, "}%s;\n#endif  // __%s_H__\n", (name.substr(0,12) == "l_struct_OC_" ? name.c_str():""), name.c_str());
    fclose(OStr);
    // now generate '.cpp' file
    if (name.substr(0,12) != "l_struct_OC_") {
    OStr = fopen((oDir + "/" + name + ".cpp").c_str(), "w");
    fprintf(OStr, "#include \"%s.h\"\n", name.c_str());
    for (auto FI : table->method) {
        Function *func = FI.second;
        fprintf(OStr, "%s {\n", printFunctionSignature(func, name + "::" + FI.first).c_str());
        processFunction(func);
        for (auto info: declareList)
            fprintf(OStr, "        %s;\n", info.c_str());
        for (auto info: storeList) {
            if (Value *cond = getCondition(info.second.cond, 0))
                fprintf(OStr, "        if (%s)\n    ", printOperand(cond, false).c_str());
            fprintf(OStr, "        %s = %s;\n", info.first.c_str(), info.second.item.c_str());
        }
        for (auto info: functionList) {
            if (Value *cond = getCondition(info.cond, 0))
                fprintf(OStr, "        if (%s)\n    ", printOperand(cond, false).c_str());
            fprintf(OStr, "        %s;\n", info.item.c_str());
        }
        fprintf(OStr, "}\n");
    }
    fprintf(OStr, "void %s::run()\n{\n", name.c_str());
    for (auto item : table->rules)
        if (item.second)
            fprintf(OStr, "    if (%s__RDY()) %s();\n", item.first.c_str(), item.first.c_str());
    for (auto item : runLines)
        fprintf(OStr, "    %s.run();\n", item.c_str());
    fprintf(OStr, "}\n");
    fclose(OStr);
    }
}
