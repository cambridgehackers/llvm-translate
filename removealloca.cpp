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
#include "llvm/Linker.h"
#include "llvm/PassManager.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

#include "declarations.h"

char RemoveAllocaPass::ID = 0;

/*
 * Remove alloca and calls to 'llvm.dbg.declare()' that were added
 * when compiling with '-g'
 */
bool RemoveAllocaPass::runOnFunction(Function &F)
{
    bool changed = false;
    for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
        Function::iterator BN = llvm::next(Function::iterator(BB));
    BasicBlock::iterator Start = BB->getFirstInsertionPt();
    BasicBlock::iterator E = BB->end();
    if (Start == E) return false;
    BasicBlock::iterator I = Start++;
    while(1) {
        BasicBlock::iterator PI = llvm::next(BasicBlock::iterator(I));
        int opcode = I->getOpcode();
        Value *retv = (Value *)I;
        switch (opcode) {
        case Instruction::Alloca:
            if (I->hasName() && endswith(I->getName().str().c_str(), ".addr")) {
                Value *newt = NULL;
                BasicBlock::iterator PN = PI;
                while (PN != E) {
                    BasicBlock::iterator PNN = llvm::next(BasicBlock::iterator(PN));
                    if (PN->getOpcode() == Instruction::Store && retv == PN->getOperand(1)) {
                        newt = PN->getOperand(0); // Remember value we were storing in temp
                        if (PI == PN)
                            PI = PNN;
                        PN->eraseFromParent(); // delete Store instruction
                    }
                    else if (PN->getOpcode() == Instruction::Load && retv == PN->getOperand(0)) {
                        PN->replaceAllUsesWith(newt); // replace with stored value
                        PN->eraseFromParent(); // delete Load instruction
                    }
                    PN = PNN;
                }
                I->eraseFromParent(); // delete Alloca instruction
                changed = true;
            }
            break;
        case Instruction::Call:
            if (CallInst *CI = dyn_cast<CallInst>(I)) {
                Value *Operand = CI->getCalledValue();
                if (Operand->hasName() && isa<Constant>(Operand)) {
                  const char *cp = Operand->getName().str().c_str();
                  if (!strcmp(cp, "llvm.dbg.declare") || !strcmp(cp, "atexit")) {
                      I->eraseFromParent(); // delete this instruction
                      break;
                      changed = true;
                  }
                }
                Instruction *nexti = PI;
                if (BB == F.begin() && BN == BE && I == BB->getFirstInsertionPt() && nexti == BB->getTerminator()) {
                    printf("[%s:%d] single!!!! %s\n", __FUNCTION__, __LINE__, F.getName().str().c_str());
                    InlineFunctionInfo IFI;
                    InlineFunction(CI, IFI, false);
                }
            }
            break;
        };
        if (PI == E)
            break;
        I = PI;
    }
    }
    return changed;
}