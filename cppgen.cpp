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
#include <cxxabi.h> // abi::__cxa_demangle
#include "llvm/IR/Instructions.h"

using namespace llvm;

#include "declarations.h"

int trace_cppstruct;// = 1;

static int processVar(const GlobalVariable *GV)
{
    if (GV->isDeclaration() || GV->getSection() == std::string("llvm.metadata")
     || (GV->hasAppendingLinkage() && GV->use_empty()
      && (GV->getName() == "llvm.global_ctors" || GV->getName() == "llvm.global_dtors")))
        return 0;
    return 1;
}

static int hasRun(const StructType *STy, int recurse)
{
    if (STy) {
        std::string sname = STy->getName();
        if (ruleFunctionNames[sname].size())
            return 1;
        for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
            if (recurse && hasRun(dyn_cast<StructType>(*I), recurse))
                return 1;
    }
    return 0;
}
int inheritsModule(const StructType *STy)
{
    if (STy) {
        std::string sname = STy->getName();
        if (sname == "class.Module")
            return 1;
        for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I)
            if (inheritsModule(dyn_cast<StructType>(*I)))
                return 1;
    }
    return 0;
}
static void generateClassElements(const StructType *STy, FILE *OStr)
{
    int Idx = 0;
    for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
        const Type *element = *I;
        std::string fname = fieldName(STy, Idx);
        if (fname != "") {
            const Type *newType = replaceType[EREPLACE_INFO{STy, Idx}];
            if (newType)
                element = newType;
            fprintf(OStr, "%s", printType(element, false, fname, "  ", ";\n").c_str());
        }
        else if (const StructType *inherit = dyn_cast<StructType>(element)) {
            //printf("[%s:%d]inherit %p\n", __FUNCTION__, __LINE__, inherit);
            generateClassElements(inherit, OStr);
        }
    }
}

void generateClassDef(const StructType *STy, FILE *OStr)
{
    std::string name = getStructName(STy);
    fprintf(OStr, "class %s {\npublic:\n", name.c_str());
    if (trace_cppstruct)
        printf("ELEMENT: TOP %s\n", name.c_str());
    generateClassElements(STy, OStr);
    if (ClassMethodTable *table = classCreate[name])
        for (auto FI : table->method) {
            Function *func = FI.first;
            std::string fname = func->getName();
            const char *className, *methodName;
            getClassName(fname.c_str(), &className, &methodName);
            fprintf(OStr, "  %s", printFunctionSignature(func, methodName, false, ";\n", 1).c_str());
        }
    if (hasRun(STy, 1))
        fprintf(OStr, "  void run();\n");
    fprintf(OStr, "};\n\n");
}

void generateClassBody(const StructType *STy, FILE *OStr)
{
    std::string name = getStructName(STy);
    if (ClassMethodTable *table = classCreate[name])
        for (auto FI : table->method) {
            VTABLE_WORK workItem(FI.first, NULL, 1);
            regen_methods = 3;
            processFunction(workItem, OStr, name);
            regen_methods = 0;
        }
    if (hasRun(STy, 1)) {
        fprintf(OStr, "void %s::run()\n{\n", name.c_str());
        //fprintf(OStr, "    printf(\" %s::run()\\n\");\n", name.c_str());
        if (hasRun(STy, 0)) {
             for (auto item : ruleFunctionNames[STy->getName()])
                 fprintf(OStr, "    if (%s__RDY()) %s();\n", item.c_str(), item.c_str());
        }
        int Idx = 0;
        for (auto I = STy->element_begin(), E = STy->element_end(); I != E; ++I, Idx++) {
            std::string fname = fieldName(STy, Idx);
            const StructType *STy = dyn_cast<StructType>(*I);
            if (const PointerType *PTy = dyn_cast<PointerType>(*I)) {
                STy = dyn_cast<StructType>(PTy->getElementType());
                if (fname != "module" && hasRun(STy, 1))
                    fprintf(OStr, "    %s->run();\n", fname.c_str());
            }
            else if (hasRun(STy, 1))
                fprintf(OStr, "    %s.run();\n", fname.c_str());
        }
        fprintf(OStr, "}\n");
    }
}

void generateCppData(FILE *OStr, Module &Mod)
{
    NextTypeID = 1;
    fprintf(OStr, "\n\n/* Global Variable Definitions and Initialization */\n");
    for (auto I = Mod.global_begin(), E = Mod.global_end(); I != E; ++I) {
        ERRORIF (I->hasWeakLinkage() || I->hasDLLImportStorageClass() || I->hasDLLExportStorageClass()
          || I->isThreadLocal() || I->hasHiddenVisibility() || I->hasExternalWeakLinkage());
        if (processVar(I)) {
          ArrayType *ATy;
          Type *Ty = I->getType()->getElementType();
          if (!(Ty->getTypeID() == Type::ArrayTyID && (ATy = cast<ArrayType>(Ty))
              && ATy->getElementType()->getTypeID() == Type::PointerTyID)
           && I->getInitializer()->isNullValue()) {
              if (I->hasLocalLinkage())
                fprintf(OStr, "static ");
              fprintf(OStr, "%s", printType(Ty, false, GetValueName(I), "", "").c_str());
              if (!I->getInitializer()->isNullValue())
                fprintf(OStr, " = %s", printOperand(NULL, I->getInitializer(), false).c_str());
              fprintf(OStr, ";\n");
          }
        }
    }
}
