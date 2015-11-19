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
 * Remove alloca and calls to 'llvm.dbg.declare()' that were added
 * when compiling with '-g'
 */
bool RemoveAllocaPass_runOnFunction(Function &F)
{
    bool changed = false;
//printf("RemoveAllocaPass: %s\n", F.getName().str().c_str());
    for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
        BasicBlock::iterator Start = BB->getFirstInsertionPt();
        BasicBlock::iterator E = BB->end();
        if (Start == E) return false;
        BasicBlock::iterator I = Start++;
        while(1) {
            Value *retv = (Value *)I;
            BasicBlock::iterator PI = std::next(BasicBlock::iterator(I));
            int opcode = I->getOpcode();
            switch (opcode) {
            case Instruction::Alloca: {
               std::string name = I->getName();
               int ind = name.find("block");
//printf("       ALLOCA %s;", name.c_str());
                if (I->hasName() && ind == -1 && endswith(name.c_str(), ".addr")) {
                    Value *newt = NULL;
                    BasicBlock::iterator PN = PI;
                    while (PN != E) {
                        BasicBlock::iterator PNN = std::next(BasicBlock::iterator(PN));
                        if (PN->getOpcode() == Instruction::Store && retv == PN->getOperand(1)) {
                            newt = PN->getOperand(0); // Remember value we were storing in temp
                            if (PI == PN)
                                PI = PNN;
                            PN->eraseFromParent(); // delete Store instruction
                        }
                        else if (PN->getOpcode() == Instruction::Load && retv == PN->getOperand(0)) {
                            PN->replaceAllUsesWith(newt); // replace with stored value
                            if (PI == PN)
                                PI = PNN;
                            PN->eraseFromParent(); // delete Load instruction
                        }
                        PN = PNN;
                    }
//printf("del1");
                    I->eraseFromParent(); // delete Alloca instruction
                    changed = true;
                }
//printf("\n");
                break;
                }
            };
            if (PI == E)
                break;
            I = PI;
        }
    }
    return changed;
}
