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
        for (auto I = BB->getFirstInsertionPt(), E = BB->end(); I != E;) {
            auto PI = std::next(BasicBlock::iterator(I));
            int opcode = I->getOpcode();
            switch (opcode) {
            case Instruction::Alloca: {
                Value *retv = (Value *)I;
                std::string name = I->getName();
                int ind = name.find("block");
//printf("       ALLOCA %s;", name.c_str());
                if (I->hasName() && ind == -1 && endswith(name, ".addr")) {
                    Value *newt = NULL;
                    auto PN = PI;
                    while (PN != E) {
                        auto PNN = std::next(BasicBlock::iterator(PN));
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
    unsigned long tparam = 0;
    unsigned long styparam = (unsigned long)findThisArgumentType(II->getParent()->getParent()->getType());
    //printf("[%s:%d] %s calling %s styparam %lx\n", __FUNCTION__, __LINE__, II->getParent()->getParent()->getName().str().c_str(), CF->getName().str().c_str(), styparam);
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
                std::string pcalledFunction = printOperand(NULL, CI->getCalledValue(), false);
                if (pcalledFunction[0] == '(' && pcalledFunction[pcalledFunction.length()-1] == ')')
                    pcalledFunction = pcalledFunction.substr(1, pcalledFunction.length() - 2);
                Function *func = dyn_cast_or_null<Function>(Mod->getNamedValue(pcalledFunction));
                if (!strncmp(pcalledFunction.c_str(), "&0x", 3) && !func) {
                    if (void *tval = mapLookup(pcalledFunction.c_str()+1)) {
                        func = static_cast<Function *>(tval);
                        pcalledFunction = func->getName();
                    }
                }
                std::string cthisp = printOperand(NULL, II->getOperand(0), false);
                //printf("%s: %s CALLS %s func %p thisp %s\n", __FUNCTION__, fname.c_str(), pcalledFunction.c_str(), func, cthisp.c_str());
                if (func && cthisp == "this") {
                    fprintf(stdout,"callProcess: pcalledFunction %s single!!!!\n", pcalledFunction.c_str());
                    RemoveAllocaPass_runOnFunction(*func);
                    II->setOperand(II->getNumOperands()-1, func);
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

bool instRunOnFunction(Function &F)
{
    bool changed = false;
    Module *Mod = F.getParent();
    std::string fname = F.getName();
//printf("instRunOnFunction: %s\n", fname.c_str());
    for (auto BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
        for (auto II = BB->begin(), IE = BB->end(); II != IE; ) {
            BasicBlock::iterator PI = std::next(BasicBlock::iterator(II));
            if (II->getOpcode() == Instruction::Store && II->getOperand(0)->getValueID() == Value::ConstantExprVal) {
                if (const ConstantExpr *vinit = dyn_cast<ConstantExpr>((II->getOperand(0))))
                if (vinit->getOpcode() == Instruction::BitCast)
                if (const ConstantExpr *gep = dyn_cast<ConstantExpr>((vinit->getOperand(0))))
                if (gep->getOpcode() == Instruction::GetElementPtr)
                if (GlobalVariable *GV = dyn_cast<GlobalVariable>(gep->getOperand(0)))
                if (const StructType *STy = vtableMap[GV]) {
                    printf("[%s:%d] STy %s\n", __FUNCTION__, __LINE__, STy->getName().str().c_str());
                    Type *Params[] = {Type::getInt64Ty(Mod->getContext())};
                    FunctionType *fty = FunctionType::get(Type::getVoidTy(Mod->getContext()),
                        ArrayRef<Type*>(Params, 1), false);
                    Function *F = dyn_cast<Function>(Mod->getOrInsertFunction("accessVtable", fty));
                    F->setCallingConv(GlobalValue::LinkOnceODRLinkage);
                    F->setDoesNotAlias(0);
                    IRBuilder<> builder(II->getParent());
                    builder.SetInsertPoint(II);
                    builder.CreateCall(F, {builder.getInt64((unsigned long)STy)});
                    changed = true;
                }
            }
            II = PI;
        }
    }
    return changed;
}
