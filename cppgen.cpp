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

std::list<std::string> extraMethods;

/*
 * Recursively generate element definitions for a class.
 */
static void generateClassElements(const StructType *STy, FILE *OStr)
{
    int Idx = 0;
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        const Type *element = *I;
        if (ClassMethodTable *table = classCreate[STy])
            if (const Type *newType = table->replaceType[Idx])
                element = newType;
        std::string fname = fieldName(STy, Idx);
        if (fname == "unused_data_to_force_inheritance")
            continue;
        if (fname != "") {
            if (const StructType *iSTy = dyn_cast<StructType>(element))
                if (inheritsModule(iSTy, "class.InterfaceClass"))
                    continue;
            fprintf(OStr, "%s", printType(element, false, fname, "  ", ";\n", false).c_str());
            if (dyn_cast<PointerType>(element)) {
                extraMethods.push_back("void set" + fname + "(" + printType(element, false, "v", "", "", false)
                    + ") { " + fname + " = v; }");
            }
        }
        else if (const StructType *inherit = dyn_cast<StructType>(element))
            generateClassElements(inherit, OStr);
    }
}

static int hasRun(const StructType *STy)
{
    if (STy && classCreate[STy]) {
        if (classCreate[STy]->rules.size())
            return 1;
        for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
            if (hasRun(dyn_cast<StructType>(*I)))
                return 1;
    }
    return 0;
}

/*
 * Generate string for class method declaration
 */
static std::string printFunctionSignature(const Function *F, std::string altname)
{
    std::string sep, statstr, tstr = altname + '(';
    FunctionType *FT = cast<FunctionType>(F->getFunctionType());
    ERRORIF (F->hasDLLImportStorageClass() || F->hasDLLExportStorageClass() || F->hasStructRetAttr() || FT->isVarArg());
    if (F->hasLocalLinkage()) statstr = "static ";
    if (F->isDeclaration()) {
        for (auto I = FT->param_begin()+1, E = FT->param_end(); I != E; ++I) {
            tstr += printType(*I, /*isSigned=*/false, "", sep, "", false);
            sep = ", ";
        }
    }
    else
        for (auto I = ++F->arg_begin(), E = F->arg_end(); I != E; ++I) {
            tstr += printType(I->getType(), /*isSigned=*/false, GetValueName(I), sep, "", false);
            sep = ", ";
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
    generateRegion = ProcessCPP;
    if (inheritsModule(STy, "class.ModuleStub") || inheritsModule(STy, "class.InterfaceClass")
     || STy->getName() == "class.Module" || STy->getName() == "class.InterfaceClass")
        return;
    ClassMethodTable *table = classCreate[STy];
    std::string name = getStructName(STy);
    FILE *OStr = fopen((oDir + "/" + name + ".h").c_str(), "w");
    fprintf(OStr, "#ifndef __%s_H__\n#define __%s_H__\n", name.c_str(), name.c_str());
    int Idx = 0;
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        const Type *element = *I;
        if (const Type *newType = table->replaceType[Idx])
            element = newType;
        if (fieldName(STy, Idx) != "") {
            if (const StructType *iSTy = dyn_cast<StructType>(element))
                if (!inheritsModule(iSTy, "class.InterfaceClass"))
                    fprintf(OStr, "#include \"%s.h\"\n", getStructName(iSTy).c_str());
            if (const PointerType *PTy = dyn_cast<PointerType>(element))
                if (const StructType *iSTy = dyn_cast<StructType>(PTy->getElementType()))
                    fprintf(OStr, "#include \"%s.h\"\n", getStructName(iSTy).c_str());
        }
    }
    fprintf(OStr, "class %s {\n", name.c_str());
    extraMethods.clear();
    generateClassElements(STy, OStr);
    fprintf(OStr, "public:\n");
    for (auto FI : table->method)
        fprintf(OStr, "  %s;\n", printFunctionSignature(FI.second, FI.first).c_str());
    if (hasRun(STy))
        fprintf(OStr, "  void run();\n");
    for (auto item: extraMethods)
        fprintf(OStr, "  %s\n", item.c_str());
    fprintf(OStr, "};\n#endif  // __%s_H__\n", name.c_str());
    fclose(OStr);

    OStr = fopen((oDir + "/" + name + ".cpp").c_str(), "w");
    fprintf(OStr, "#include \"%s.h\"\n", name.c_str());
    for (auto FI : table->method) {
        Function *func = FI.second;
        if (endswith(func->getName(), "EC2Ev"))
            continue;
        fprintf(OStr, "%s {\n", printFunctionSignature(func, name + "::" + FI.first).c_str());
        processFunction(func);
        for (auto info: storeList) {
            Value *cond = getCondition(info.second.cond, 0);
            if (cond)
                fprintf(OStr, "        if (%s)\n    ", printOperand(cond, false).c_str());
            fprintf(OStr, "        %s = %s;\n", info.first.c_str(), info.second.item.c_str());
        }
        for (auto item: functionList)
            fprintf(OStr, "        %s;\n", item.c_str());
        fprintf(OStr, "}\n");
    }
    if (hasRun(STy)) {
        fprintf(OStr, "void %s::run()\n{\n", name.c_str());
        for (auto item : table->rules)
            fprintf(OStr, "    if (%s__RDY()) %s();\n", item.c_str(), item.c_str());
        int Idx = 0;
        for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
            std::string fname = fieldName(STy, Idx);
            const PointerType *PTy = dyn_cast<PointerType>(*I);
            if (hasRun(!PTy ? dyn_cast<StructType>(*I)
                            : dyn_cast<StructType>(PTy->getElementType())))
                fprintf(OStr, "    %s%srun();\n", fname.c_str(), PTy ? "->" : ".");
        }
        fprintf(OStr, "}\n");
    }
}
