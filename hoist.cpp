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
                 builder.CreateConstInBoundsGEP1_32(
                     vtabbase, methodIndex)), new_thisp);
    return dyn_cast<Instruction>(newCall);
}

/*
 * Perform guard(), update() hoisting.  Insert shadow variable access for store.
 */
const char *calculateGuardUpdate(Function ***thisp, Instruction &I)
{
    int opcode = I.getOpcode();
    switch (opcode) {
    // Terminators
#if 0
    case Instruction::Br:
        if (isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
            const BranchInst &BI(cast<BranchInst>(I));
            prepareOperand(BI.getCondition());
            int cond_item = getLocalSlot(BI.getCondition());
            char temp[MAX_CHAR_BUFFER];
            sprintf(temp, "%s" SEPARATOR "%s_cond", globalName, I.getParent()->getName().str().c_str());
            if (slotarray[cond_item].name) {
                slotarray[cond_item].name = strdup(temp);
            }
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
        sprintf(temp, "%s" SEPARATOR "%s_phival", globalName, I.getParent()->getName().str().c_str());
        slotarray[operand_list[0].value].name = strdup(temp);
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
        const char *p = getOperand(thisp, Callee, false);
        int guardName = -1, updateName = -1, parentGuardName = -1, parentUpdateName = -1;
        Function *func = dyn_cast<Function>(I.getOperand(I.getNumOperands()-1));
        const char *cp = NULL;
printf("[%s:%d] thisp %p func %p Callee %p p %s\n", __FUNCTION__, __LINE__, thisp, func, Callee, p);
        if (!func) {
            void *pact = mapLookup(p);
            func = static_cast<Function *>(pact);
        }
        printf("[%s:%d] CallPTR %p thisp %p\n", __FUNCTION__, __LINE__, func, thisp);
        if (!func) {
            printf("[%s:%d] not an instantiable call!!!!\n", __FUNCTION__, __LINE__);
        }
        else {
            cp = func->getName().str().c_str();
            printf("[%s:%d] Call %s\n", __FUNCTION__, __LINE__, cp);
            if (trace_translate)
                printf("%s: CALL %d %s %p\n", __FUNCTION__, I.getType()->getTypeID(), cp, thisp);
            if (func->isDeclaration() && !strncmp(cp, "_Z14PIPELINEMARKER", 18)) {
                cloneVmap.clear();
                /* for now, just remove the Call.  Later we will push processing of I.getOperand(0) into another block */
                Function *F = I.getParent()->getParent();
                Module *Mod = F->getParent();
                std::string Fname = F->getName().str();
                std::string otherName = Fname.substr(0, Fname.length() - 8) + "2" + "6updateEv";
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
            int len = 0;
            int status;
            const char *demang = abi::__cxa_demangle(func->getName().str().c_str(), 0, 0, &status);
            if (demang) {
                const char *p = demang;
                while (*p && *p != '(')
                    p++;
                while (p != demang && *p != ':') {
                    len++;
                    p--;
                }
                const PointerType *PTy;
                const StructType *STy;
                if (func->arg_begin() != func->arg_end()
                 && (PTy = dyn_cast<PointerType>(func->arg_begin()->getType()))
                 && (STy = dyn_cast<StructType>(PTy->getPointerElementType()))) {
                    std::string tname = STy->getName().str();
                    if (len > 2) {
                        char tempname[1000];
                        memcpy(tempname, p+1, len-1);
                        strcpy(tempname+len-1, "__guard");
                        guardName = lookup_method(tname.c_str(), tempname);
                    }
                }
            }
        }
        const GlobalValue *g = NULL;
        if (thisp)
            g = EE->getGlobalValueAtAddress(thisp[0] - 2);
        printf("[%s:%d] guard %d update %d thisp %p g %p\n", __FUNCTION__, __LINE__, guardName, updateName, thisp, g);
        if (g) {
            char temp[MAX_CHAR_BUFFER];
            int status;
            const char *ret = abi::__cxa_demangle(g->getName().str().c_str(), 0, 0, &status);
            sprintf(temp, "class.%s", ret+11);
            parentGuardName = lookup_method(temp, "guard");
            //parentUpdateName = lookup_method(temp, "update");
        }
        printf("[%s:%d] pguard %d pupdate %d\n", __FUNCTION__, __LINE__, parentGuardName, parentUpdateName);
        if (guardName >= 0 && parentGuardName >= 0) {
            Function *peer_guard = thisp[0][parentGuardName];
            TerminatorInst *TI = peer_guard->begin()->getTerminator();
            Instruction *newI = copyFunction(TI, &I, guardName, Type::getInt1Ty(TI->getContext()));
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
        if (parentUpdateName >= 0) {
            Function *peer_update = thisp[0][parentUpdateName];
            TerminatorInst *TI = peer_update->begin()->getTerminator();
            if (updateName >= 0)
                copyFunction(TI, &I, updateName, Type::getVoidTy(TI->getContext()));
            else if (I.use_empty()) {
                copyFunction(TI, &I, 0, NULL); // Move this call to the 'update()' method
                I.eraseFromParent(); // delete "Call" instruction
            }
        }
        pushWork(func, NULL);
        break;
        }
    default:
        return processCInstruction(thisp, I);
        printf("HOther opcode %d.=%s\n", opcode, I.getOpcodeName());
        exit(1);
    case Instruction::Store: case Instruction::Ret:
        break;
    }
    return NULL;
}
