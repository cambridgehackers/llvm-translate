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
        if (fname != "")
            fprintf(OStr, "%s", printType(element, false, fname, "  ", ";\n", false).c_str());
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
void generateClassDef(const StructType *STy, FILE *OStr, std::string ODir)
{
    ClassMethodTable *table = classCreate[STy];
    std::string name = getStructName(STy);
    fprintf(OStr, "class %s {\npublic:\n", name.c_str());
    generateClassElements(STy, OStr);
    for (auto FI : table->method)
        fprintf(OStr, "  %s", printFunctionSignature(FI.second, FI.first, false, ";\n", 1).c_str());
    if (hasRun(STy))
        fprintf(OStr, "  void run();\n");
    fprintf(OStr, "};\n\n");
}

void generateClassBody(const StructType *STy, FILE *OStr, std::string ODir)
{
    ClassMethodTable *table = classCreate[STy];
    std::string name = getStructName(STy);
    for (auto FI : table->method) {
        Function *func = FI.second;
        fprintf(OStr, "%s", printFunctionSignature(func, name + "::" + FI.first, false, " {\n", 1).c_str());
        processFunction(func, NULL, OStr);
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
