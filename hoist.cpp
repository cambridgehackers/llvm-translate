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
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

#include "declarations.h"

static std::map<const Value *, Value *> cloneVmap;
int trace_clone;

/*
 * clone a DAG from one basic block to another
 */
static Instruction *cloneTree(const Instruction *I, Instruction *insertPoint)
{
    std::string NameSuffix = "foosuff";
    Instruction *NewInst = I->clone();

    if (I->hasName())
        NewInst->setName(I->getName()+NameSuffix);
    for (unsigned OI = 0, E = I->getNumOperands(); OI != E; ++OI) {
        const Value *oval = I->getOperand(OI);
        if (cloneVmap.find(oval) == cloneVmap.end()) {
            if (const Instruction *IC = dyn_cast<Instruction>(oval))
                cloneVmap[oval] = cloneTree(IC, insertPoint);
            else
                continue;
        }
        NewInst->setOperand(OI, cloneVmap[oval]);
    }
    NewInst->insertBefore(insertPoint);
    if (trace_clone)
        printf("[%s:%d] %s %d\n", __FUNCTION__, __LINE__, NewInst->getOpcodeName(), NewInst->getNumOperands());
    return NewInst;
}

static void prepareClone(Instruction *TI, const Instruction *I)
{
    cloneVmap.clear();
    Function *TargetF = TI->getParent()->getParent();
    const Function *SourceF = I->getParent()->getParent();
    Function::arg_iterator TargetA = TargetF->arg_begin();
    for (Function::const_arg_iterator AI = SourceF->arg_begin(),
             AE = SourceF->arg_end(); AI != AE; ++AI, ++TargetA)
        cloneVmap[AI] = TargetA;
}
static Instruction *copyFunction(Instruction *TI, const Instruction *I, int methodIndex, Type *returnType)
{
    prepareClone(TI, I);
    if (!returnType)
        return cloneTree(I, TI);
    Instruction *orig_thisp = dyn_cast<Instruction>(I->getOperand(0));
    Instruction *new_thisp = cloneTree(orig_thisp, TI);
    Type *Params[] = {new_thisp->getType()};
    Type *castType = PointerType::get(
             PointerType::get(
                 PointerType::get(
                     FunctionType::get(returnType,
                           ArrayRef<Type*>(Params, 1), false),
                     0), 0), 0);
    IRBuilder<> builder(TI->getParent());
    builder.SetInsertPoint(TI);
    Value *vtabbase = builder.CreateLoad(
             builder.CreateBitCast(new_thisp, castType));
    Value *newCall = builder.CreateCall(
             builder.CreateLoad(
                 builder.CreateConstInBoundsGEP1_32(nullptr,
                     vtabbase, methodIndex)), new_thisp);
    return dyn_cast<Instruction>(newCall);
}

void recursiveDelete(Value *V)
{
    Instruction *I = dyn_cast<Instruction>(V);
    if (!I)
        return;
    for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i) {
        Value *OpV = I->getOperand(i);
        I->setOperand(i, nullptr);
        if (OpV->use_empty())
            recursiveDelete(OpV);
    }
    I->eraseFromParent();
}

/*
 * Perform RDY(), ENA() hoisting.  Insert shadow variable access for store.
 */
std::string calculateGuardUpdate(Function ***thisp, Instruction &I)
{
    int opcode = I.getOpcode();
//printf("[%s:%d] op %s thisp %p\n", __FUNCTION__, __LINE__, I.getOpcodeName(), thisp);
    switch (opcode) {
    // Terminators
    case Instruction::Ret:
        break;
#if 0
    case Instruction::Br:
        if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
            const BranchInst &BI(cast<BranchInst>(I));
            prepareOperand(BI.getCondition());
            int cond_item = getLocalSlot(BI.getCondition());
            char temp[MAX_CHAR_BUFFER];
            sprintf(temp, "%s" SEPARATOR "%s_cond", globalName.c_str(), I.getParent()->getName().str().c_str());
            prepareOperand(BI.getSuccessor(0));
            prepareOperand(BI.getSuccessor(1));
        } else if (isa<IndirectBrInst>(I)) {
            for (unsigned i = 0, e = I.getNumOperands(); i != e; ++i)
                prepareOperand(I.getOperand(i));
        }
        break;
#endif

    // Other instructions...
#if 0
    case Instruction::PHI: {
        char temp[MAX_CHAR_BUFFER];
        const PHINode *PN = dyn_cast<PHINode>(&I);
        I.getType()->dump();
        sprintf(temp, "%s" SEPARATOR "%s_phival", globalName.c_str(), I.getParent()->getName().str().c_str());
        for (unsigned op = 0, Eop = PN->getNumIncomingValues(); op < Eop; ++op) {
            int valuein = getLocalSlot(PN->getIncomingValue(op));
            prepareOperand(PN->getIncomingValue(op));
            prepareOperand(PN->getIncomingBlock(op));
            TerminatorInst *TI = PN->getIncomingBlock(op)->getTerminator();
            //printf("[%s:%d] terminator\n", __FUNCTION__, __LINE__);
            //TI->dump();
            const BranchInst *BI = dyn_cast<BranchInst>(TI);
            const char *trailch = "";
            if (isa<BranchInst>(TI) && cast<BranchInst>(TI)->isConditional()) {
              prepareOperand(BI->getCondition());
              int cond_item = getLocalSlot(BI->getCondition());
              sprintf(temp, "%s ?", slotarray[cond_item].name);
              trailch = ":";
              //prepareOperand(BI->getSuccessor(0));
              //prepareOperand(BI->getSuccessor(1));
            }
            if (slotarray[valuein].name)
                sprintf(temp, "%s %s", slotarray[valuein].name, trailch);
            else
                sprintf(temp, "%lld %s", (long long)slotarray[valuein].offset, trailch);
        }
        }
        break;
#endif
    case Instruction::Call: {
        CallInst &ICL = static_cast<CallInst&>(I);
        Value *Callee = ICL.getCalledValue();
        ConstantExpr *CE = dyn_cast<ConstantExpr>(Callee);
        ERRORIF (CE && CE->isCast() && (dyn_cast<Function>(CE->getOperand(0))));
        std::string p = fetchOperand(thisp, Callee, false);
        int RDYName = -1, ENAName = -1, parentRDYName = -1, parentENAName = -1;
        Function *func = dyn_cast<Function>(I.getOperand(I.getNumOperands()-1));
        std::string fname;
        const StructType *STy;
        const char *className, *methodName;
        const GlobalValue *g = NULL;
printf("[%s:%d] thisp %p func %p Callee %p p %s\n", __FUNCTION__, __LINE__, thisp, func, Callee, p.c_str());
        if (!func) {
            void *pact = mapLookup(p.c_str());
            func = static_cast<Function *>(pact);
        }
        if (!func) {
            printf("[%s:%d] not an instantiable call!!!!\n", __FUNCTION__, __LINE__);
            break;
        }
        std::string cthisp = fetchOperand(thisp, I.getOperand(0), false);
        Function ***called_thisp = NULL;
        if (!strncmp(cthisp.c_str(), "0x", 2))
            called_thisp = (Function ***)mapLookup(cthisp.c_str());
        fname = func->getName();
        printf("[%s:%d] CallPTR %p %s thisp %p\n", __FUNCTION__, __LINE__, func, fname.c_str(), thisp);
        pushWork(func, called_thisp, 0);
        printf("[%s:%d] Call %s\n", __FUNCTION__, __LINE__, fname.c_str());
        if (trace_translate)
            printf("%s: CALL %d %s %p\n", __FUNCTION__, I.getType()->getTypeID(), fname.c_str(), thisp);
        if (func->isDeclaration() && !strncmp(fname.c_str(), "_Z14PIPELINEMARKER", 18)) {
            cloneVmap.clear();
            /* for now, just remove the Call.  Later we will push processing of I.getOperand(0) into another block */
            Function *F = I.getParent()->getParent();
            Module *Mod = F->getParent();
            std::string Fname = F->getName().str();
            std::string otherName = Fname.substr(0, Fname.length() - 8) + "2" + "3ENAEv";
            Function *otherBody = Mod->getFunction(otherName);
            TerminatorInst *TI = otherBody->begin()->getTerminator();
            prepareClone(TI, &I);
            Instruction *IT = dyn_cast<Instruction>(I.getOperand(1));
            Instruction *IC = dyn_cast<Instruction>(I.getOperand(0));
            Instruction *newIC = cloneTree(IC, TI);
            Instruction *newIT = cloneTree(IT, TI);
            printf("[%s:%d] other %s %p\n", __FUNCTION__, __LINE__, otherName.c_str(), otherBody);
            IRBuilder<> builder(TI->getParent());
            builder.SetInsertPoint(TI);
            builder.CreateStore(newIC, newIT);
            IRBuilder<> oldbuilder(I.getParent());
            oldbuilder.SetInsertPoint(&I);
            Value *newLoad = oldbuilder.CreateLoad(IT);
            I.replaceAllUsesWith(newLoad);
            I.eraseFromParent();
            break;
        }
        if ((STy = findThisArgument(func))
         && getClassName(fname.c_str(), &className, &methodName)) {
            std::string tname = STy->getName();
            char tempname[1000];
            strcpy(tempname, methodName);
            strcat(tempname, "__RDY");
            RDYName = lookup_method(tname.c_str(), tempname);
        }
        if (thisp)
            g = EE->getGlobalValueAtAddress(thisp[0] - 2);
        printf("[%s:%d] RDY %d ENA %d thisp %p g %p\n", __FUNCTION__, __LINE__, RDYName, ENAName, thisp, g);
        if (g) {
            char temp[MAX_CHAR_BUFFER];
            int status;
            const char *ret = abi::__cxa_demangle(g->getName().str().c_str(), 0, 0, &status);
            sprintf(temp, "class.%s", ret+11);
            parentRDYName = lookup_method(temp, "RDY");
            //parentENAName = lookup_method(temp, "ENA");
        }
        printf("[%s:%d] pRDY %d pENA %d\n", __FUNCTION__, __LINE__, parentRDYName, parentENAName);
        if (RDYName >= 0 && parentRDYName >= 0) {
            Function *peer_RDY = thisp[0][parentRDYName];
            TerminatorInst *TI = peer_RDY->begin()->getTerminator();
            Instruction *newI = copyFunction(TI, &I, RDYName, Type::getInt1Ty(TI->getContext()));
            if (CallInst *nc = dyn_cast<CallInst>(newI))
                nc->addAttribute(AttributeSet::ReturnIndex, Attribute::ZExt);
            Value *cond = TI->getOperand(0);
            const ConstantInt *CI = dyn_cast<ConstantInt>(cond);
            if (CI && CI->getType()->isIntegerTy(1) && CI->getZExtValue())
                TI->setOperand(0, newI);
            else {
                // 'And' return value into condition
                Instruction *newBool = BinaryOperator::Create(Instruction::And, newI, newI, "newand", TI);
                cond->replaceAllUsesWith(newBool);
                // we must set this after the 'replaceAllUsesWith'
                newBool->setOperand(0, cond);
            }
        }
        if (parentENAName >= 0) {
            Function *peer_ENA = thisp[0][parentENAName];
            TerminatorInst *TI = peer_ENA->begin()->getTerminator();
            if (ENAName >= 0)
                copyFunction(TI, &I, ENAName, Type::getVoidTy(TI->getContext()));
            else if (I.use_empty()) {
                copyFunction(TI, &I, 0, NULL); // Move this call to the 'ENA()' method
                I.eraseFromParent(); // delete "Call" instruction
            }
        }
        if (cthisp == "Vthis") {
            printf("[%s:%d] single!!!! %s\n", __FUNCTION__, __LINE__, func->getName().str().c_str());
fprintf(stderr, "[%s:%d] thisp %p func %p Callee %p p %s\n", __FUNCTION__, __LINE__, thisp, func, Callee, p.c_str());
ICL.dump();
I.dump();
I.getOperand(0)->dump();
#if 1
            InlineFunctionInfo IFI;
            InlineFunction(&ICL, IFI, false);
#endif
        }
        break;
        }
    default:
        return processCInstruction(thisp, I);
        printf("HOther opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
    case Instruction::Store:
        break;
    }
    return "";
}
