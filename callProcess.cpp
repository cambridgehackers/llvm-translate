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
#include "llvm/ADT/STLExtras.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

#include "declarations.h"

/*
 * Remove alloca and calls to 'llvm.dbg.declare()' that were added
 * when compiling with '-g'
 */
static bool RemoveAllocaPass_runOnFunction(Function &F)
{
    bool changed = false;
    for (auto BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
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
                if (I->hasName() && ind == -1 && endswith(name, ".addr")) {
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

/*
 * Map calls to 'new()' and 'malloc()' in constructors to call 'llvm_translate_malloc'.
 * This enables llvm-translate to easily maintain a list of valid memory regions
 * during processing.
 */
void callMemrunOnFunction(CallInst *II)
{
    Module *Mod = II->getParent()->getParent()->getParent();
    Value *called = II->getOperand(II->getNumOperands()-1);
    const Function *CF = dyn_cast<Function>(called);
    Instruction *PI = II->user_back();
    unsigned long tparam = 0, styparam = (unsigned long)findThisArgumentType(CF->getType());
    if (PI->getOpcode() == Instruction::BitCast && &*II == PI->getOperand(0))
        tparam = (unsigned long)PI->getType();
    Type *Params[] = {Type::getInt64Ty(Mod->getContext()),
        Type::getInt64Ty(Mod->getContext()), Type::getInt64Ty(Mod->getContext())};
    FunctionType *fty = FunctionType::get(Type::getInt8PtrTy(Mod->getContext()),
        ArrayRef<Type*>(Params, 3), false);
    Function *F = dyn_cast<Function>(Mod->getOrInsertFunction("llvm_translate_malloc", fty));
    F->setCallingConv(CF->getCallingConv());
    F->setDoesNotAlias(0);
    F->setAttributes(CF->getAttributes());
    IRBuilder<> builder(II->getParent());
    builder.SetInsertPoint(II);
    II->replaceAllUsesWith(builder.CreateCall(F,
       {II->getOperand(0), builder.getInt64(tparam), builder.getInt64(styparam)},
       "llvm_translate_malloc"));
    II->eraseFromParent();
}

bool call2runOnFunction(Function &F)
{
    bool changed = false;
    Module *Mod = F.getParent();
    std::string fname = F.getName();
//printf("CallProcessPass2: %s\n", fname.c_str());
    if (fname.length() > 5 && fname.substr(0,5) == "_ZNSt") {
        printf("SKIPPING %s\n", fname.c_str());
        return changed;
    }
    changed = RemoveAllocaPass_runOnFunction(F);
    for (auto BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
        for (auto II = BB->begin(), IE = BB->end(); II != IE; ) {
            BasicBlock::iterator PI = std::next(BasicBlock::iterator(II));
            if (CallInst *CI = dyn_cast<CallInst>(II)) {
                std::string lname = fetchOperand(NULL, CI->getCalledValue(), false);
                if (lname[0] == '(' && lname[lname.length()-1] == ')')
                    lname = lname.substr(1, lname.length() - 2);
                Value *val = Mod->getNamedValue(lname);
                std::string cthisp = fetchOperand(NULL, II->getOperand(0), false);
                if (val && cthisp == "Vthis") {
                    //fprintf(stdout,"callProcess: lname %s single!!!! cthisp %s\n", lname.c_str(), cthisp.c_str());
                    RemoveAllocaPass_runOnFunction(*dyn_cast<Function>(val));
                    II->setOperand(II->getNumOperands()-1, val);
                    InlineFunctionInfo IFI;
                    InlineFunction(CI, IFI, false);
                    changed = true;
                }
            }
            II = PI;
        }
    }
    return changed;
}
