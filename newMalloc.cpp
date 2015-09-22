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

using namespace llvm;

#include "declarations.h"

/*
 * Map calls to 'new()' and 'malloc()' in constructors to call 'llvm_translate_malloc'.
 * This enables llvm-translate to easily maintain a list of valid memory regions
 * during processing.
 */
static bool callProcess_runOnInstruction(Instruction *II)
{
    Module *Mod = II->getParent()->getParent()->getParent();
    Value *called = II->getOperand(II->getNumOperands()-1);
    const char *cp = called->getName().str().c_str();
    const Function *CF = dyn_cast<Function>(called);
    if (!CF || !CF->isDeclaration())
        return false;
    if (!strcmp(cp, "_Znwm") || !strcmp(cp, "malloc")) {
        //printf("[%s:%d]CALL %d\n", __FUNCTION__, __LINE__, called->getValueID());
        Type *Params[] = {Type::getInt64Ty(Mod->getContext())};
        FunctionType *fty = FunctionType::get(
            Type::getInt8PtrTy(Mod->getContext()),
            ArrayRef<Type*>(Params, 1), false);
        Value *newmalloc = Mod->getOrInsertFunction("llvm_translate_malloc", fty);
        Function *F = dyn_cast<Function>(newmalloc);
        Function *cc = dyn_cast<Function>(called);
        F->setCallingConv(cc->getCallingConv());
        F->setDoesNotAlias(0);
        F->setAttributes(cc->getAttributes());
        called->replaceAllUsesWith(newmalloc);
        return true;
    }
    return false;
}

char CallProcessPass::ID = 0;
bool CallProcessPass::runOnBasicBlock(BasicBlock &BB)
{
    bool changed = false;
    for (BasicBlock::iterator II = BB.begin(), IE = BB.end(); II != IE; ) {
        BasicBlock::iterator PI = llvm::next(BasicBlock::iterator(II));
        if (II->getOpcode() == Instruction::Call)
            changed |= callProcess_runOnInstruction(II);
        II = PI;
    }
    return changed;
}
