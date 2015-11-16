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
#include "llvm/ADT/STLExtras.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

#include "declarations.h"

static bool call2Process_runOnInstruction(Instruction *I)
{
    Module *Mod = I->getParent()->getParent()->getParent();
    CallInst *CI = dyn_cast<CallInst>(I);
    Value *called = I->getOperand(I->getNumOperands()-1);
    std::string cp = called->getName();
    if (cp != "printf" && CI) {
        std::string lname = fetchOperand(NULL, CI->getCalledValue(), false);
        if (lname[0] == '(' && lname[lname.length()-1] == ')')
            lname = lname.substr(1, lname.length() - 2);
        Value *val = Mod->getNamedValue(lname);
        std::string cthisp = fetchOperand(NULL, I->getOperand(0), false);
        if (val && cthisp == "Vthis") {
            //fprintf(stdout,"callProcess: lname %s single!!!! cthisp %s\n", lname.c_str(), cthisp.c_str());
            RemoveAllocaPass_runOnFunction(*dyn_cast<Function>(val));
            I->setOperand(I->getNumOperands()-1, val);
            InlineFunctionInfo IFI;
            InlineFunction(CI, IFI, false);
            return true;
        }
    }
    return false;
}

bool callMemrunOnFunction(Function &F)
{
    bool changed = false;
    std::string fname = F.getName();
//printf("CallProcessPass: %s\n", fname.c_str());
    if (fname.length() > 5 && fname.substr(0,5) == "_ZNSt") {
        printf("SKIPPING %s\n", fname.c_str());
        return changed;
    }
    changed = RemoveAllocaPass_runOnFunction(F);
    /*
     * Map calls to 'new()' and 'malloc()' in constructors to call 'llvm_translate_malloc'.
     * This enables llvm-translate to easily maintain a list of valid memory regions
     * during processing.
     */
    for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
        for (BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE; ) {
            BasicBlock::iterator PI = std::next(BasicBlock::iterator(II));
            if (II->getOpcode() == Instruction::Call) {
                Module *Mod = II->getParent()->getParent()->getParent();
                Value *called = II->getOperand(II->getNumOperands()-1);
                std::string cp = called->getName();
                const Function *CF = dyn_cast<Function>(called);
                if ((cp == "_Znwm" || cp == "malloc")
                    && CF && CF->isDeclaration()) {
                    IRBuilder<> builder(II->getParent());
                    builder.SetInsertPoint(II);
                    unsigned long tparam = 0;
                    printf("[%s:%d]CALL %d\n", __FUNCTION__, __LINE__, called->getValueID());
                    if (PI->getOpcode() == Instruction::BitCast && &*II == PI->getOperand(0))
                        tparam = (unsigned long)PI->getType();
                    Type *Params[] = {Type::getInt64Ty(Mod->getContext()),
                        Type::getInt64Ty(Mod->getContext())};
                    FunctionType *fty = FunctionType::get(
                        Type::getInt8PtrTy(Mod->getContext()),
                        ArrayRef<Type*>(Params, 2), false);
                    Value *newmalloc = Mod->getOrInsertFunction("llvm_translate_malloc", fty);
                    Function *F = dyn_cast<Function>(newmalloc);
                    F->setCallingConv(CF->getCallingConv());
                    F->setDoesNotAlias(0);
                    F->setAttributes(CF->getAttributes());
                    II->replaceAllUsesWith(builder.CreateCall(
                        F, {II->getOperand(0), builder.getInt64(tparam)}, "llvm_translate_malloc"));
                    II->eraseFromParent();
                    changed = true;
                }
            }
            II = PI;
        }
    }
//printf("CallProcessPass: end %s changed %d\n", fname.c_str(), changed);
    return changed;
}

bool call2runOnFunction(Function &F)
{
    bool changed = false;
    std::string fname = F.getName();
//printf("CallProcessPass2: %s\n", fname.c_str());
    if (fname.length() > 5 && fname.substr(0,5) == "_ZNSt") {
        printf("SKIPPING %s\n", fname.c_str());
        return changed;
    }
    changed = RemoveAllocaPass_runOnFunction(F);
    for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
        for (BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE; ) {
            BasicBlock::iterator PI = std::next(BasicBlock::iterator(II));
            if (II->getOpcode() == Instruction::Call)
                changed |= call2Process_runOnInstruction(II);
            II = PI;
        }
    }
//printf("CallProcessPass2: end %s changed %d\n", fname.c_str(), changed);
    return changed;
}
